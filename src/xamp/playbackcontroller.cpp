#include "playbackcontroller.h"
#include "sharedmodeplayback.h"
#include <QFileInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>
#include <stdexcept>
#include <thememanager.h>
#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/jsonsettings.h>
#include <widget/imagecache.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/util/ui_util.h>
#include <stream/api.h>
#include <stream/avlibfilestream.h>
#include <base/bitperfect.h>
#include <stream/idspmanager.h>
#include <stream/mqafilestream.h>
#include <output_device/audiodevicemanager.h>

namespace {
    uint32_t resamplerTargetSampleRate(const QString& type) {
        QVariantMap settings;
        if (type == kSoxr || type.isEmpty()) {
            const auto setting_name = qAppSettings.valueAsString(kAppSettingSoxrSettingName);
            settings = qJsonSettings.valueAs(kSoxr).toMap()[setting_name].toMap();
        }
        else {
            settings = qJsonSettings.valueAsMap(type);
        }
        return settings[kResampleSampleRate].toUInt();
    }

    std::optional<EqSettings> storedParametricEqSettings() {
        if (!qAppSettings.contains(kAppSettingEQName)) {
            return std::nullopt;
        }
        const auto app_settings = qAppSettings.eqSettings();
        const auto preset_settings = qAppSettings.eqPreset().value(app_settings.name);
        if (app_settings.name != QStringLiteral("Manual") && !preset_settings.bands.empty()) {
            auto settings = preset_settings;
            settings.preamp = app_settings.settings.preamp;
            return settings;
        }
        return app_settings.settings;
    }

}

PlaybackController::PlaybackController(std::shared_ptr<IAudioPlayer> player, QObject* parent)
    : QObject(parent), player_(std::move(player)), adapter_(std::make_shared<UIPlayerStateAdapter>()) {
    player_->setStateAdapter(adapter_);
    // Stamp events at emission, not delivery, so queued callbacks from old tracks cannot change the new one.
    connect(adapter_.get(), &UIPlayerStateAdapter::stateChanged, this, [this](PlayerState state) {
        if (!accepting_events_) return;
        const auto generation = generation_.load();
        QMetaObject::invokeMethod(this, [this, generation, state] {
            if (!shutting_down_ && accepting_events_ && generation == generation_) acceptState(state);
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);
    connect(adapter_.get(), &UIPlayerStateAdapter::sampleTimeChanged, this, [this](double seconds) {
        if (!accepting_events_) return;
        const auto generation = generation_.load();
        QMetaObject::invokeMethod(this, [this, generation, seconds] {
            if (!shutting_down_ && accepting_events_ && generation == generation_ && snapshot_.hasTrack()) {
                snapshot_.position = std::clamp(seconds, 0.0, snapshot_.track.duration);
                emit positionChanged(snapshot_.position);
            }
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);
    connect(adapter_.get(), &UIPlayerStateAdapter::playbackError, this, [this](const QString& message) {
        if (!accepting_events_) return;
        const auto generation = generation_.load();
        QMetaObject::invokeMethod(this, [this, generation, message] {
            if (shutting_down_ || !accepting_events_ || generation != generation_) return;
            stop();
            emit failed(std::make_exception_ptr(std::runtime_error(message.toStdString())));
        }, Qt::QueuedConnection);
    }, Qt::DirectConnection);
    // SQL flags are a compatibility projection, never restored as live playback on startup.
    QSqlQuery query(qGuiDb.database());
    query.prepare("UPDATE playlistMusics SET playing = 0"_str);
    if (!query.exec()) throw std::runtime_error(query.lastError().text().toStdString());
}

bool PlaybackController::open(const PlayListEntity& requested, double position) {
    if (!device_) {
        emit failed(std::make_exception_ptr(std::runtime_error("No audio output device selected")));
        return false;
    }
    const auto& file_name = requested.file_path;
    const auto* entity = &requested;
    auto file_sample_rate = 44100;
    auto file_duration = 0.0;
    QPixmap display_cover = qTheme.unknownCover();
    TrackInfo track_info;
    auto has_display_cover = false;

    if (entity != nullptr) {
        const auto cover_id = entity->validCoverId();
        if (!cover_id.isEmpty() && cover_id != qImageCache.unknownCoverId()) {
            auto cache_cover = qImageCache.getOrDefault(cover_id);
            if (!cache_cover.isNull()) {
                display_cover = cache_cover;
                has_display_cover = true;
            }
        }
    }

    try {
        auto metadata_reader = makeMetadataReader();
        metadata_reader->open(file_name.toStdWString());

        auto metadata_opt = metadata_reader->extract();
        if (metadata_opt.has_value()) {
            track_info = metadata_opt.value();
            file_sample_rate = track_info.sample_rate;
            file_duration = track_info.duration;
            if (!has_display_cover) {
                auto buffer = metadata_reader->readEmbeddedCover();
                if (buffer.has_value() && buffer.value().size() > 0) {
                    const auto& cover_buffer = buffer.value();
                    QPixmap embedded_cover;
                    if (embedded_cover.loadFromData(
                        reinterpret_cast<const uchar*>(cover_buffer.data()),
                        static_cast<uint>(cover_buffer.size()))) {
                        display_cover = embedded_cover;
                        has_display_cover = true;
                    }
                }
            }
        }
        else {
            throw std::runtime_error("Unable to read track metadata");
        }
    }
    catch (...) {
        emit failed(std::current_exception());
        return false;
    }


    accepting_events_ = false;
    ++generation_;
    try {
        player_->stop();
    }
    catch (...) {
        clear();
        emit failed(std::current_exception());
        return false;
    }

    try {
    const auto is_shared_device = player_->getAudioDeviceManager()->isSharedDevice(
        device_.value().device_type_id);
    const auto is_asio_device = player_->getAudioDeviceManager()->isASIODevice(
        device_.value().device_type_id);
    auto playback_plan = resolvePlaybackPlan(device_.value(),
        file_sample_rate,
        isDsdFile(file_name.toStdWString()),
        is_shared_device,
        is_asio_device);    

    const bool bitperfect = qAppSettings.valueAsBool(QStringLiteral("bitPerfectEnabled"));
    if (bitperfect) {
        playback_plan.use_mqa_decode = false;
        playback_plan.needs_resample = false;
        playback_plan.output_mode = DsdModes::DSD_MODE_PCM;
        playback_plan.target_sample_rate = file_sample_rate;
    }
    auto& dsp_manager = player_->getDspManager();
    dsp_manager->setBitPerfect(bitperfect);
    dsp_manager->removeSampleRateConverter();
    if (playback_plan.needs_resample) {
        dsp_manager->addPreDSP(makeSrcSampleRateConverter());
    }
    else if (!bitperfect && qAppSettings.valueAsBool(kAppSettingResamplerEnable)) {
        const auto type = qAppSettings.valueAsString(kAppSettingResamplerType);
        const auto target_sample_rate = resamplerTargetSampleRate(type);
        if (target_sample_rate != 0) {
            playback_plan.target_sample_rate = target_sample_rate;
        }

        if (type == kSoxr || type.isEmpty()) {
            dsp_manager->addPreDSP(makeSoxrSampleRateConverter(target_sample_rate));
        }
        else if (type == kR8Brain) {
#ifdef XAMP_OS_WIN
            dsp_manager->addPreDSP(makeR8BrainSampleRateConverter());
#else
            dsp_manager->addPreDSP(makeSrcSampleRateConverter());
#endif
        }
        else {
            dsp_manager->addPreDSP(makeSrcSampleRateConverter());
        }
    }

    if (!bitperfect && qAppSettings.valueAsBool(kAppSettingEnableEQ)) {
        if (const auto eq_settings = storedParametricEqSettings()) {
            player_->getDspConfig().create(DspConfig::kEQSettings, *eq_settings);
            player_->getDspManager()->addParametricEq();
        }
    }
    else {
        player_->getDspManager()->removeParametricEq();
    }

        ScopedPtr<FileStream> file_stream;
        if (bitperfect) {
            auto source_owner = makeAlign<FileStream, AvLibFileStream>();
            auto* source = static_cast<AvLibFileStream*>(source_owner.get());
            source->setIntegerPcm(true);
            source->openFile(file_name.toStdWString());
            const auto format = source->integerPcmFormat();
            if (!format || !xamp::bitperfect::supported(format->valid_bits, format->channels, format->sample_rate))
                throw std::runtime_error("BitPerfect requires stereo 16/24/32-bit integer PCM WAV or FLAC");
            playback_plan.byte_format = ByteFormat::SINT32;
            playback_plan.target_sample_rate = format->sample_rate;
            file_stream = std::move(source_owner);
        } else {
            file_stream = StreamFactory::makeFileStream(file_name.toStdWString(),
                playback_plan.output_mode, playback_plan.use_mqa_decode);
        }

        auto use_mqa_decode = dynamic_cast<MqaFileStream*>(file_stream.get()) != nullptr;
        if (!use_mqa_decode) {
            playback_plan.use_mqa_decode = false;
        }
        const auto byte_format = bitperfect ? playback_plan.byte_format : resolvePreparedPlaybackByteFormat(
            playback_plan,
            use_mqa_decode);
        if (use_mqa_decode) {
            player_->getDspManager()->removeParametricEq();
        }

        player_->open(std::move(file_stream),
            device_.value(),
            playback_plan.target_sample_rate,
            playback_plan.output_mode);

        player_->getDspManager()->setSampleWriter();
        player_->prepareToPlay(byte_format);
        player_->bufferStream(position, 0, std::nullopt);
        player_->play();
    }
    catch (...) {
        const auto error = std::current_exception();
        try { player_->stop(true, false, true); } catch (...) {}
        clear();
        emit failed(error);
        return false;
    }


    auto playback_format = getPlaybackFormat(player_.get());
    PlayListEntity playing_entity = entity ? *entity : PlayListEntity{};
    const QFileInfo file_info(file_name);
    if (playing_entity.file_path.isEmpty()) {
        playing_entity.file_path = file_name;
    }
    if (playing_entity.parent_path.isEmpty()) {
        playing_entity.parent_path = file_info.absolutePath();
    }
    if (playing_entity.file_name.isEmpty()) {
        playing_entity.file_name = file_info.completeBaseName();
    }
    if (playing_entity.title.isEmpty()) {
        playing_entity.title = toQString(track_info.title);
    }
    if (playing_entity.artist.isEmpty()) {
        playing_entity.artist = toQString(track_info.artist);
    }
    if (playing_entity.album.isEmpty()) {
        playing_entity.album = toQString(track_info.album);
    }
    if (playing_entity.duration <= 0) {
        playing_entity.duration = file_duration;
    }
    if (playing_entity.sample_rate == 0) {
        playing_entity.sample_rate = file_sample_rate;
    }
    if (playing_entity.bit_rate == 0) {
        playing_entity.bit_rate = track_info.bit_rate;
    }
    if (playing_entity.track == 0) {
        playing_entity.track = track_info.track;
    }
    if (playing_entity.file_extension.isEmpty()) {
        playing_entity.file_extension = file_info.suffix();
    }

    snapshot_.track = playing_entity;
    snapshot_.track_revision = generation_.load();
    snapshot_.cover = display_cover;
    restored_ = false;
    snapshot_.position = position;
    snapshot_.status = PlaybackStatus::Playing;
    const auto ext = track_info.file_ext() ? toQString(track_info.file_ext().value()) : file_info.suffix();
    snapshot_.bitperfect = player_->getDspManager()->isBitPerfect();
    snapshot_.format = format2String(playback_format, ext, fromStdStringView(device_->desc));
    accepting_events_ = true;
    return true;
}

void PlaybackController::play(const PlayListEntity& track, int playlist, PlaybackSource source,
    const QList<PlayListEntity>& queue) {
    if (!open(track)) return;
    snapshot_.playlist_id = playlist;
    snapshot_.source = source;
    queue_.reset(queue, snapshot_.track);
    publish();
}

void PlaybackController::next(int direction) {
    if (queue_.empty()) { emit startRequested(); return; }
    const auto order = qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder);
    const auto previous_queue = queue_;
    const auto track = queue_.next(order, direction);
    if (track && open(*track)) publish();
    else { queue_ = previous_queue; if (!snapshot_.hasTrack()) clear(); }
}

void PlaybackController::playQueued(int id) {
    const auto previous_queue = queue_;
    const auto track = queue_.select(id);
    if (track && open(*track)) publish();
    else queue_ = previous_queue;
}

void PlaybackController::togglePause() {
    try {
        if (snapshot_.status == PlaybackStatus::Playing) {
            player_->pause(); snapshot_.status = PlaybackStatus::Paused; publish();
        } else if (snapshot_.status == PlaybackStatus::Paused) {
            if (restored_) { if (open(snapshot_.track, snapshot_.position)) publish(); return; }
            player_->resume(); snapshot_.status = PlaybackStatus::Playing; publish();
        } else if (const auto track = queue_.current()) {
            if (open(*track)) publish();
        } else { emit startRequested(); }
    } catch (...) { emit failed(std::current_exception()); }
}

void PlaybackController::seek(double seconds) {
    if (!snapshot_.hasTrack()) return;
    if (restored_) { snapshot_.position = std::clamp(seconds, 0.0, snapshot_.track.duration); emit positionChanged(snapshot_.position); return; }
    try {
        accepting_events_ = false; ++generation_;
        player_->seek(std::clamp(seconds, 0.0, snapshot_.track.duration));
        snapshot_.status = PlaybackStatus::Playing;
        snapshot_.position = std::clamp(seconds, 0.0, snapshot_.track.duration);
        accepting_events_ = true;
        publish(); emit positionChanged(snapshot_.position);
    } catch (...) { stop(); emit failed(std::current_exception()); }
}

void PlaybackController::stop(bool shutdown_device) {
    accepting_events_ = false; ++generation_;
    try { player_->stop(true, shutdown_device, true); }
    catch (...) { emit failed(std::current_exception()); }
    clear();
}

void PlaybackController::clear() {
    restored_ = false;
    accepting_events_ = false;
    // Keep the queue and origin for the next play command, clear every visible track projection.
    snapshot_.status = PlaybackStatus::Stopped;
    snapshot_.track = {};
    snapshot_.cover = {};
    snapshot_.bitperfect = false;
    snapshot_.format.clear();
    snapshot_.position = 0;
    snapshot_.upcoming.clear();
    publish();
}

void PlaybackController::acceptState(PlayerState state) {
    if (!snapshot_.hasTrack()) return;
    if (state != player_->getState()) return;
    if (state == PlayerState::PLAYER_STATE_STOPPED) {
        if (snapshot_.source == PlaybackSource::External) clear();
        else {
            const auto revision = snapshot_.track_revision;
            next(1);
            if (snapshot_.track_revision == revision) clear();
        }
    } else if (state == PlayerState::PLAYER_STATE_USER_STOPPED) {
        clear();
    } else if (state == PlayerState::PLAYER_STATE_PAUSED) {
        snapshot_.status = PlaybackStatus::Paused; publish();
    } else if (state == PlayerState::PLAYER_STATE_RUNNING || state == PlayerState::PLAYER_STATE_RESUME) {
        snapshot_.status = PlaybackStatus::Playing; publish();
    }
}

void PlaybackController::refreshOrder() { publish(); }
void PlaybackController::updateAlbumCover(int album_id, const QPixmap& cover) {
    if (!snapshot_.hasTrack() || snapshot_.track.album_id != album_id || cover.isNull()) return;
    snapshot_.cover = cover; publish();
}
void PlaybackController::setFavorite(bool favorite) {
    if (!snapshot_.hasTrack() || snapshot_.track.music_id <= 0) return;
    qDaoFacade.music_dao.updateMusicHeart(snapshot_.track.music_id, favorite);
    snapshot_.track.heart = favorite; publish();
}
void PlaybackController::publish() {
    const auto order = qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder);
    snapshot_.shuffle = order == PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM;
    snapshot_.upcoming = snapshot_.hasTrack() ? queue_.preview(order) : QList<PlayListEntity>{};
    // Legacy SQL-backed views consume this projection; only the controller writes live flags.
    try {
        QSqlQuery query(qGuiDb.database());
        query.prepare("UPDATE playlistMusics SET playing = CASE WHEN playlistId = :playlist AND playlistMusicsId = :item THEN :state ELSE 0 END WHERE playing <> 0 OR (playlistId = :playlist AND playlistMusicsId = :item)"_str);
        query.bindValue(":playlist"_str, snapshot_.playlist_id);
        query.bindValue(":item"_str, snapshot_.track.playlist_music_id);
        query.bindValue(":state"_str, !snapshot_.hasTrack() ? PlayingState::PLAY_CLEAR :
            snapshot_.status == PlaybackStatus::Paused ? PlayingState::PLAY_PAUSE : PlayingState::PLAY_PLAYING);
        if (!query.exec()) throw std::runtime_error(query.lastError().text().toStdString());
    } catch (...) { emit failed(std::current_exception()); }
    emit changed(snapshot_);
}
void PlaybackController::shutdown() {
    shutting_down_ = true;
    accepting_events_ = false; ++generation_;
    player_->destroy();
}

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
namespace {
QJsonObject sessionTrack(const PlayListEntity& t) {
    QJsonObject o;
    o.insert("file_path"_str, t.file_path);
    o.insert("title"_str, t.title);
    o.insert("artist"_str, t.artist);
    o.insert("album"_str, t.album);
    o.insert("cover_id"_str, t.cover_id);
    o.insert("file_extension"_str, t.file_extension);
    o.insert("parent_path"_str, t.parent_path);
    o.insert("file_name"_str, t.file_name);
    o.insert("playlist_music_id"_str, t.playlist_music_id);
    o.insert("music_id"_str, t.music_id);
    o.insert("album_id"_str, t.album_id);
    o.insert("artist_id"_str, t.artist_id);
    o.insert("track"_str, qint64(t.track));
    o.insert("sample_rate"_str, qint64(t.sample_rate));
    o.insert("bit_rate"_str, qint64(t.bit_rate));
    o.insert("duration"_str, t.duration);
    o.insert("is_cue_file"_str, t.is_cue_file);
    o.insert("is_zip_file"_str, t.is_zip_file);
    o.insert("heart"_str, t.heart);
    if (t.offset) o.insert("offset"_str, *t.offset);
    if (t.music_cover_id) o.insert("music_cover_id"_str, *t.music_cover_id);
    if (t.archive_entry_name) o.insert("archive_entry_name"_str, *t.archive_entry_name);
    return o;
}
PlayListEntity sessionTrack(const QJsonObject& o) {
    PlayListEntity t;
    t.file_path = o.value("file_path"_str).toString();
    t.title = o.value("title"_str).toString();
    t.artist = o.value("artist"_str).toString();
    t.album = o.value("album"_str).toString();
    t.cover_id = o.value("cover_id"_str).toString();
    t.file_extension = o.value("file_extension"_str).toString();
    t.parent_path = o.value("parent_path"_str).toString();
    t.file_name = o.value("file_name"_str).toString();
    t.playlist_music_id = o.value("playlist_music_id"_str).toInt();
    t.music_id = o.value("music_id"_str).toInt();
    t.album_id = o.value("album_id"_str).toInt();
    t.artist_id = o.value("artist_id"_str).toInt();
    t.track = o.value("track"_str).toInt();
    t.sample_rate = o.value("sample_rate"_str).toInt();
    t.bit_rate = o.value("bit_rate"_str).toInt();
    t.duration = o.value("duration"_str).toDouble();
    t.is_cue_file = o.value("is_cue_file"_str).toBool();
    t.is_zip_file = o.value("is_zip_file"_str).toBool();
    t.heart = o.value("heart"_str).toBool();
    if (o.contains("offset"_str)) t.offset = o.value("offset"_str).toDouble();
    if (o.contains("music_cover_id"_str)) t.music_cover_id = o.value("music_cover_id"_str).toString();
    if (o.contains("archive_entry_name"_str)) t.archive_entry_name = o.value("archive_entry_name"_str).toString();
    return t;
}
}
void PlaybackController::saveUpdateSession() {
    QJsonObject state;
    if (snapshot_.hasTrack()) {
        state.insert("track"_str, sessionTrack(snapshot_.track));
        state.insert("position"_str, snapshot_.position);
        state.insert("playlist"_str, snapshot_.playlist_id);
        state.insert("source"_str, int(snapshot_.source));
        QJsonArray tracks;
        for (const auto& track : queue_.tracks()) tracks.append(sessionTrack(track));
        state.insert("queue"_str, tracks);
    }
    qAppSettings.setValue("updatePlaybackSession"_str, QString::fromUtf8(QJsonDocument(state).toJson(QJsonDocument::Compact)));
    qAppSettings.save();
}
void PlaybackController::restoreUpdateSession() {
    const auto state = QJsonDocument::fromJson(qAppSettings.valueAsString("updatePlaybackSession"_str).toUtf8()).object();
    qAppSettings.setValue("updatePlaybackSession"_str, QString());
    qAppSettings.save();
    if (!state.contains("track"_str)) return;
    const auto track = sessionTrack(state.value("track"_str).toObject());
    if (!QFileInfo::exists(track.file_path)) return;
    snapshot_.track = track;
    snapshot_.playlist_id = state.value("playlist"_str).toInt(-1);
    snapshot_.source = static_cast<PlaybackSource>(state.value("source"_str).toInt());
    snapshot_.position = std::clamp(state.value("position"_str).toDouble(), 0.0, track.duration);
    snapshot_.status = PlaybackStatus::Paused;
    snapshot_.track_revision = ++generation_;
    snapshot_.cover = qImageCache.getOrDefault(track.validCoverId());
    QList<PlayListEntity> tracks;
    for (const auto& item : state.value("queue"_str).toArray()) tracks.append(sessionTrack(item.toObject()));
    queue_.reset(tracks, track);
    restored_ = true;
    publish();
    emit positionChanged(snapshot_.position);
}

void PlaybackController::discardPlaylist(int playlist_id) {
    if (snapshot_.playlist_id != playlist_id) return;
    stop();
    queue_ = PlaybackQueue{};
    snapshot_.playlist_id = -1;
    publish();
}
