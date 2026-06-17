#include <thememanager.h>
#include <version.h>
#include <xamp.h>
#include <deviceselectormenu.h>
#include <sharedmodeplayback.h>

#ifdef Q_OS_WIN
#include <QSimpleUpdater.h>
#endif

#include <algorithm>
#include <iterator>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QImage>
#include <QMap>
#include <QProcess>
#include <QPointer>
#include <QScreen>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QFileInfo>

#include <base/threadpool.h>
#include <base/crashhandler.h>
#include <base/scopeguard.h>
#include <base/stopwatch.h>

#include <player/audio_player.h>
#include <stream/api.h>
#include <stream/idspmanager.h>
#include <stream/mqafilestream.h>
#include <stream/mqaidentifier.h>

#include <output_device/audiodevicemanager.h>

#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>
#include <widget/equalizerview.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/jsonsettings.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/filesystemviewpage.h>
#include <widget/chatgpt/spectrogramwidget.h>
#include <widget/richplaylistpage.h>
#include <widget/databasefacade.h>
#include <widget/imagecache.h>
#include <widget/playlisttableview.h>
#include <widget/playlistpage.h>
#include <widget/encodejobwidget.h>
#include <widget/logview.h>
#include <widget/preferencepage.h>
#include <widget/cdpage.h>
#include <widget/waveformslider.h>
#include <widget/musicbrainzeditpage.h>
#include <widget/aboutpage.h>
#include <widget/scanfileprogresspage.h>
#include <widget/xmessagebox.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/worker/backgroundservice.h>
#include <widget/worker/filesystemservice.h>

#include <style_util.h>

namespace {
    const auto kUpdateDefinitionsUrl = "https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json"_str;

    size_t CountTracks(const std::forward_list<TrackInfo>& tracks) {
        return static_cast<size_t>(std::distance(tracks.begin(), tracks.end()));
    }

    size_t CountTrackBatches(const std::vector<std::forward_list<TrackInfo>>& batches) {
        size_t track_count = 0;
        for (const auto& tracks : batches) {
            track_count += CountTracks(tracks);
        }
        return track_count;
    }

    bool IsStopped(PlayerState state) {
        return state == PlayerState::PLAYER_STATE_STOPPED
            || state == PlayerState::PLAYER_STATE_USER_STOPPED;
    }

    uint32_t ResamplerTargetSampleRate(const QString& type) {
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

    void CollectAlbumIds(const std::forward_list<TrackInfo>& tracks, QSet<int32_t>& album_ids) {
        for (const auto& track : tracks) {
            const auto music_id = qDaoFacade.music_dao.getMusicId(toQString(track.file_path));
            if (!music_id.has_value()) {
                continue;
            }
            const auto album_id = qDaoFacade.album_dao.getAlbumIdFromAlbumMusic(music_id.value());
            if (album_id > 0 && album_id != qDatabaseFacade.unknownAlbumId()) {
                album_ids.insert(album_id);
            }
        }
    }

    QSet<int32_t> CollectAlbumIds(const std::vector<std::forward_list<TrackInfo>>& batches) {
        QSet<int32_t> album_ids;
        for (const auto& tracks : batches) {
            CollectAlbumIds(tracks, album_ids);
        }
        return album_ids;
    }

    QSize dialogContentSize(QWidget* dialog, const QWidget* content) {
        auto* screen = dialog != nullptr ? dialog->screen() : QGuiApplication::primaryScreen();
        if (screen == nullptr) {
            return content->sizeHint();
        }

        const auto available = screen->availableGeometry().size();
        const QSize max_size(static_cast<int>(available.width() * 0.85), static_cast<int>(available.height() * 0.85));
        auto size = content->sizeHint()
            .expandedTo(content->minimumSizeHint())
            .boundedTo(max_size);
        if (dialog != nullptr) {
            const auto content_hint = content->sizeHint().expandedTo(content->minimumSizeHint());
            const auto dialog_hint = dialog->sizeHint().expandedTo(dialog->minimumSizeHint());
            const QSize chrome(
                std::max(0, dialog_hint.width() - content_hint.width()),
                std::max(0, dialog_hint.height() - content_hint.height()));
            size = (size + chrome).boundedTo(max_size);
        }
        return size;
    }

    QSize dialogSizeFromHost(QWidget* dialog,
        const QWidget* content,
        const QWidget* host,
        double ratio) {
        auto* screen = dialog != nullptr ? dialog->screen() : QGuiApplication::primaryScreen();
        const auto fallback_size = dialogContentSize(dialog, content);
        if (screen == nullptr || host == nullptr) {
            return fallback_size;
        }

        const auto available = screen->availableGeometry().size();
        const auto host_size = host->size();
        if (host_size.isEmpty()) {
            return fallback_size;
        }

        const QSize target_size(static_cast<int>(host_size.width() * ratio),
            static_cast<int>(host_size.height() * ratio));
        const QSize max_size(static_cast<int>(available.width() * 0.95),
            static_cast<int>(available.height() * 0.95));
        return target_size
            .expandedTo(fallback_size)
            .boundedTo(max_size);
    }

    bool HasMultipleKnownAlbums(const QList<PlayListEntity>& entities) {
        QSet<QString> albums;
        for (const auto& entity : entities) {
            const auto album = entity.album.trimmed();
            if (album.isEmpty()) {
                continue;
            }
            albums.insert(album.toCaseFolded());
            if (albums.size() > 1) {
                return true;
            }
        }
        return false;
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

    void applyParametricEqToPlayer(const std::shared_ptr<IAudioPlayer>& player,
        bool enabled,
        const EqSettings& settings) {
        player->setParametricEq(enabled, settings);
    }

    template <typename Service>
    void destroyWorkerService(QScopedPointer<Service>& service, QThread& thread) {
        auto* object = service.take();
        if (object == nullptr) {
            return;
        }

        const auto destroy_object = [object]() {
            object->cancelRequested();
            object->deleteLater();
            QCoreApplication::sendPostedEvents(object, QEvent::DeferredDelete);
        };

        if (thread.isRunning() && object->thread() != QThread::currentThread()) {
            QMetaObject::invokeMethod(object, destroy_object, Qt::BlockingQueuedConnection);
        }
        else {
            destroy_object();
        }
    }
}

Xamp::Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer>& player)
    : IXFrame(parent)
	, player_(player) {
    thread_pool_ = ThreadPoolBuilder::makeBackgroundThreadPool();
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_.setupUi(this);
    ui_.verticalSpacer->changeSize(10, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui_.verticalSpacer_4->changeSize(10, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui_.horizontalLayout->setStretch(0, 0);
    ui_.horizontalLayout->setStretch(1, 0);
    ui_.horizontalLayout->setStretch(2, 0);
    ui_.horizontalLayout->setStretch(3, 1);
    ui_.currentView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui_.horizontalLayout->invalidate();
    device_menu_.reset(new DeviceSelectorMenu(ui_.selectDeviceButton, ui_.deviceDescLabel, this));
}

Xamp::~Xamp() {
    destory();
}

void Xamp::pushWidget(QWidget* widget) {
    const auto id = ui_.currentView->addWidget(widget);
    widgets_.push_back(widget);
    ui_.currentView->setCurrentIndex(id);
}

void Xamp::setCurrentTab(int32_t table_id) {
    const auto set_current_widget = [this](QWidget* widget) {
        if (widget != nullptr && ui_.currentView->indexOf(widget) >= 0) {
            ui_.currentView->setCurrentWidget(widget);
        }
    };

    switch (table_id) {
    case TAB_MUSIC_LIBRARY:
        break;
    case TAB_FILE_EXPLORER:
        set_current_widget(file_explorer_page_.get());
        return;
        break;
    case TAB_RICH_PLAYLIST:
        set_current_widget(rich_playlist_page_.get());
        return;
        break;
    case TAB_LYRICS:
        set_current_widget(lrc_page_.get());
        break;
    case TAB_CD:
        set_current_widget(cd_page_.get());
        return;
        break;
    }
    if (widgets_.size() > table_id) {
        ui_.currentView->slideInWidget(widgets_[table_id]);
    }
    else {
        ui_.currentView->slideInWidget(lrc_page_.get());
    }
}

void Xamp::initialDeviceList(const std::string& device_id) {
    XAMP_LOG_DEBUG("initial device list");

    const auto device_type_id = qAppSettings.valueAsId(kAppSettingDeviceType);
    auto current_device_id = device_id;
    if (current_device_id.empty()) {
        current_device_id = qAppSettings.valueAsString(kAppSettingDeviceId).toStdString();
    }

    auto apply_device = [this](const DeviceInfo& device_info) {
        device_info_ = device_info;
        qAppSettings.setValue(kAppSettingDeviceType, device_info.device_type_id);
        qAppSettings.setValue(kAppSettingDeviceId, device_info.device_id);
    };

    const auto selected_device = device_menu_->rebuild(player_->getAudioDeviceManager(),
        device_type_id,
        current_device_id,
        device_info_,
        apply_device);

    if (selected_device.has_value()) {
        apply_device(*selected_device);
        XAMP_LOG_DEBUG("Use device Id : {}", selected_device->device_id);
    }
}

QString Xamp::translateText(const std::string_view& text) {
    return tr(text.data());
}

void Xamp::destory() {
    if (main_window_ != nullptr) {
        main_window_->saveAppGeometry();
    }
    else {
        return;
    }

    auto quit_and_wait_thread = [](auto& thread) {
        if (!thread.isFinished()) {
            thread.requestInterruption();
            thread.quit();
            thread.wait();
        }
        };

    destroyWorkerService(file_system_service_, file_system_service_thread_);
    quit_and_wait_thread(file_system_service_thread_);

    destroyWorkerService(album_cover_service_, album_cover_service_thread_);
    quit_and_wait_thread(album_cover_service_thread_);

    destroyWorkerService(background_service_, background_service_thread_);
    quit_and_wait_thread(background_service_thread_);
    qGuiDb.close();
    XampCrashHandler.cleanup();
    main_window_ = nullptr;
}

void Xamp::shortcutsPressed(const QKeySequence& shortcut) {
    XAMP_LOG_DEBUG("shortcutsPressed: {}", shortcut.toString().toStdString());

    static const QMap<QKeySequence, std::function<void()>> shortcut_map{
        { QKeySequence(Qt::Key_MediaPlay), [this]() {
            playOrPause();
            }},
         { QKeySequence(Qt::Key_MediaStop), [this]() {
            stopPlay();
            }},
        { QKeySequence(Qt::Key_MediaPrevious), [this]() {
            playPrevious();
            }},
        { QKeySequence(Qt::Key_MediaNext), [this]() {
            playNext();
            }},
        { QKeySequence(Qt::Key_VolumeUp), [this]() {
            setVolume(player_->getVolume() + 1);
            }},
        { QKeySequence(Qt::Key_VolumeDown), [this]() {
            setVolume(player_->getVolume() - 1);
            }},
        {
            QKeySequence(Qt::Key_VolumeMute),
            [this]() {
                setVolume(0);
            },
        }
    };

    const auto key = shortcut_map.value(shortcut);
    if (key != nullptr) {
        key();
    }
}

void Xamp::setVolume(uint32_t volume) {
    ui_.mutedButton->onVolumeChanged(volume);
    ui_.mutedButton->showDialog();
    ui_.mutedButton->updateState();
}

void Xamp::drivesRemoved(const DriveInfo& drive_info) {
    cd_page_->playlistPage()->playlist()->removeAll();
    cd_page_->playlistPage()->playlist()->reload();
    cd_page_->showPlaylistPage(false);
}

void Xamp::drivesChanges(const QList<DriveInfo>& drive_infos) {
    cd_page_->playlistPage()->playlist()->removeAll();
    cd_page_->playlistPage()->playlist()->reload();
    emit fetchCdInfo(drive_infos.first());
}

void Xamp::setAlbumCover(const QPixmap& cover) {
    const auto ui_cover = image_util::roundCoverImage(cover,
        ui_.coverLabel->size(),
        image_util::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);
}

void Xamp::playLocalFile(const PlayListEntity& entity, bool queue, bool update_playlist_now_playing) {
    playLocalFile(entity.file_path, queue, &entity, update_playlist_now_playing);
}

void Xamp::playLocalFile(const QString& file_name,
    bool queue,
    const PlayListEntity* entity,
    bool update_playlist_now_playing) {
    auto file_sample_rate = 44100;
    auto file_duration = 0.0;
    QPixmap display_cover = qTheme.unknownCover();
    TrackInfo track_info;
    auto has_display_cover = false;

    if (entity != nullptr) {
        const auto cover_id = entity->validCoverId();
        if (!cover_id.isEmpty() && cover_id != qImageCache.unknownCoverId()) {
            if (auto cache_cover = qImageCache.tryGet(kAlbumCacheTag, cover_id);
                cache_cover.has_value() && !cache_cover->isNull()) {
                display_cover = cache_cover.value();
                has_display_cover = true;
            }
            else {
                const auto cache_entity = qImageCache.getFromFile(kAlbumCacheTag + cover_id);
                if (!cache_entity.image.isNull()) {
                    display_cover = cache_entity.image;
                    has_display_cover = true;
                }
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
            return;
        }
    }
    catch (...) {
        logAndShowMessage(std::current_exception());
        return;
    }

    (void)queue;

    try {
        player_->stop();
    }
    catch (...) {
        logAndShowMessage(std::current_exception());
        return;
    }

    const auto is_shared_device = player_->getAudioDeviceManager()->isSharedDevice(
        device_info_.value().device_type_id);
    const auto is_asio_device = player_->getAudioDeviceManager()->isASIODevice(
        device_info_.value().device_type_id);
    auto playback_plan = resolvePlaybackPlan(device_info_.value(),
        file_sample_rate,
        isDsdFile(file_name.toStdWString()),
        is_shared_device,
        is_asio_device);    

    auto& dsp_manager = player_->getDspManager();
    dsp_manager->removeSampleRateConverter();
    if (playback_plan.needs_resample) {
        dsp_manager->addPreDSP(makeSrcSampleRateConverter());
    }
    else if (qAppSettings.valueAsBool(kAppSettingResamplerEnable)) {
        const auto type = qAppSettings.valueAsString(kAppSettingResamplerType);
        const auto target_sample_rate = ResamplerTargetSampleRate(type);
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

    if (qAppSettings.valueAsBool(kAppSettingEnableEQ)) {
        if (const auto eq_settings = storedParametricEqSettings()) {
            player_->getDspConfig().create(DspConfig::kEQSettings, *eq_settings);
            player_->getDspManager()->addParametricEq();
        }
    }
    else {
        player_->getDspManager()->removeParametricEq();
    }

    try {
        auto file_stream = StreamFactory::MakeFileStream(file_name.toStdWString(),
            playback_plan.output_mode,
            playback_plan.use_mqa_decode);

        auto use_mqa_decode = dynamic_cast<MqaFileStream*>(file_stream.get()) != nullptr;
        if (!use_mqa_decode) {
            playback_plan.use_mqa_decode = false;
        }
        const auto byte_format = resolvePreparedPlaybackByteFormat(
            playback_plan,
            use_mqa_decode);
        if (use_mqa_decode) {
            player_->getDspManager()->removeParametricEq();
        }

        player_->open(std::move(file_stream),
            device_info_.value(),
            playback_plan.target_sample_rate,
            playback_plan.output_mode);

        player_->getDspManager()->setSampleWriter();
        player_->prepareToPlay(byte_format);
        player_->bufferStream(0, 0, std::nullopt);
        player_->play();
    }
    catch (...) {
        logAndShowMessage(std::current_exception());
        return;
	}

    qTheme.setPlayOrPauseButton(ui_.playButton, true);

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
    lrc_page_->setPlayListEntity(playing_entity);
    lrc_page_->disableLoadLrcButton();
    if (!lrc_page_->lyrics()->loadFile(file_name)) {
        emit searchLyrics(playing_entity);
    }
    lrc_page_->setCover(display_cover);
    const auto playback_file_ext = track_info.file_ext()
        ? toQString(track_info.file_ext().value())
        : file_info.suffix();
    lrc_page_->format()->setText(format2String(playback_format,
        playback_file_ext,
        fromStdStringView(device_info_.value().desc)));
    lrc_page_->clearBackground();
    if (update_playlist_now_playing) {
        rich_playlist_page_->setNowPlaying(track_info, display_cover);
    }

    main_window_->setIconicThumbnail(display_cover);

    double duration = 0;
    duration = Round(file_duration) * 1000;
    ui_.seekSlider->setRange(0, duration);
    ui_.seekSlider->setValue(0);
    ui_.seekSlider->loadFile(file_name);

    ui_.titleLabel->setText(toQString(track_info.title));
    ui_.artistLabel->setText(toQString(track_info.artist));

    setAlbumCover(display_cover);
}

void Xamp::setMainWindow(IXMainWindow* main_window) {
    main_window_ = main_window;

    setThemeIcon(ui_);
    setWidgetStyle(ui_);
    setShufflePlayOrder(ui_);
    updateButtonState(ui_.playButton, PlayerState::PLAYER_STATE_STOPPED);
    initialDeviceList();
    showNaviBarButton();

    if (!qDaoFacade.playlist_dao.isPlaylistExist(kDefaultPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("Default Playlist"),
            kDefaultPlaylistId);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kFileSystemPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("FileSystem Playlist"),
            kFileSystemPlaylistId);
    }    
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kCdPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("CD Playlist"),
            kCdPlaylistId);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kAlbumPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("Album Playlist"),
            kAlbumPlaylistId);
    }

    (void)QObject::connect(ui_.naviBarButton, &QToolButton::clicked, [this]() {
        showNaviBarButton();
        });

    (void)QObject::connect(ui_.prevButton, &QToolButton::clicked, [this]() {
        playPrevious();
        });

    (void)QObject::connect(ui_.nextButton, &QToolButton::clicked, [this]() {
        playNext();
        });

    setAlbumCover(qTheme.unknownCover());

    setRepeatButtonIcon(ui_, qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder));

    ui_.naviBar->addTab(translateText("Playback"),
        TAB_LYRICS,
        qTheme.fontIcon(Glyphs::ICON_PLAYING));
    ui_.naviBar->addTab(translateText("Playlist"),
        TAB_RICH_PLAYLIST,
        qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
    ui_.naviBar->addTab(translateText("Library"),
        TAB_FILE_EXPLORER,
        qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_.naviBar->addTab(translateText("CD"),
        TAB_CD,
        qTheme.fontIcon(Glyphs::ICON_CD));

    if (qAppSettings.valueAsBool(kAppSettingHideNaviBar)) {
        ui_.sliderFrame2->setMaximumWidth(50);
        ui_.naviBar->collapse();
    }
    else {
        ui_.sliderFrame2->setMaximumWidth(180);
        ui_.naviBar->expand();
    }

    lrc_page_.reset(new LrcPage(this));
    rich_playlist_page_.reset(new RichPlaylistPage(this));
    file_explorer_page_.reset(new FileSystemViewPage(this));
    file_explorer_page_->setScannerThreadPool(ThreadPoolBuilder::makeBackgroundThreadPool());

    state_adapter_.reset(new UIPlayerStateAdapter(this));
    player_->setStateAdapter(state_adapter_);

    ui_.mutedButton->setAudioPlayer(player_);
    ui_.mutedButton->updateState();

    auto f = font();
    f.setPointSize(qTheme.fontSize(9));
    ui_.titleLabel->setFont(f);
    ui_.titleLabel->setElideMode(Qt::ElideRight);
    f.setPointSize(qTheme.fontSize(8));
    ui_.artistLabel->setFont(f);
    ui_.artistLabel->setWordWrap(false);
    ui_.artistLabel->setElideMode(Qt::ElideRight);

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::stateChanged,
        this,
        &Xamp::onPlayerStateChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::sampleTimeChanged,
        this,
        &Xamp::onSampleTimeChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::sampleTimeChanged,
        file_explorer_page_.get(),
        [this](double stream_time) {
            if (!spectrogram_tracks_playback_) {
                return;
            }
            file_explorer_page_->spectrogramWidget()->setCurrentPosition(static_cast<float>(stream_time));
        },
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_->spectrogramWidget(),
        &SpectrogramWidget::playAt,
        this,
        [this](float sec) {
            if (!spectrogram_tracks_playback_) {
                return;
            }
            if (IsStopped(player_->getState())) {
                return;
            }
            try {
                is_seeking_ = true;
                XAMP_ON_SCOPE_EXIT(is_seeking_ = false);
                player_->seek(sec);
                qTheme.setPlayOrPauseButton(ui_.playButton, true);
                main_window_->setTaskbarPlayingResume();
            }
            catch (...) {
                player_->stop(false);
                logAndShowMessage(std::current_exception());
            }
        });

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::deviceChanged,
        this,
        &Xamp::onDeviceStateChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_->playlistPage()->playlist(),
        &PlaylistTableView::playMusic,
        this,
        [this](int32_t playlist_id, const PlayListEntity& item, bool is_play) {
            (void)playlist_id;
            spectrogram_tracks_playback_ = true;
            playLocalFile(item, is_play, false);
        });

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::playMusic,
        this,
        [this](int32_t playlist_id, const PlayListEntity& item, bool is_play) {
            (void)playlist_id;
            spectrogram_tracks_playback_ = false;
            playLocalFile(item, is_play);
        });

    (void)QObject::connect(ui_.playButton, &QToolButton::clicked, [this]() {
        playOrPause();
        });

    (void)QObject::connect(ui_.naviBar, &NavBarListView::clickedTable, [this](auto table_id) {
        setCurrentTab(table_id);
        });

    (void)QObject::connect(ui_.repeatButton, &QToolButton::clicked, [this](auto table_id) {
        const auto order = getNextOrder(qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder));
        qAppSettings.setEnumValue(kAppSettingOrder, order);
        setRepeatButtonIcon(ui_, order);
        });

    (void)QObject::connect(ui_.seekSlider, &WaveformSlider::leftButtonValueChanged, [this](auto value) {
        if (IsStopped(player_->getState())) {
            ui_.seekSlider->setSeekEnabled(false);
            qTheme.setPlayOrPauseButton(ui_.playButton, false);
            ui_.seekSlider->setValue(0);
            ui_.startPosLabel->setText(formatDuration(0));
            main_window_->resetTaskbarProgress();
            return;
        }
        try {
            is_seeking_ = true;
            XAMP_ON_SCOPE_EXIT(is_seeking_ = false);
            player_->seek(value / 1000.0);
            qTheme.setPlayOrPauseButton(ui_.playButton, true);
            main_window_->setTaskbarPlayingResume();
        }
        catch (...) {
            player_->stop(false);
            logAndShowMessage(std::current_exception());
        }
        });

    (void)QObject::connect(ui_.eqButton, &QToolButton::clicked, [this]() {
        if (player_->getDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->getDsdModes() == DsdModes::DSD_MODE_NATIVE) {
            return;
        }
        QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
        QScopedPointer<XDialog> dialog(new XDialog(this));
        QScopedPointer<EqualizerView> eq(new EqualizerView(dialog.get()));
        (void)QObject::connect(eq.get(),
            &EqualizerView::parametricEqChanged,
            this,
            [this](bool enabled, const EqSettings& settings) {
                applyParametricEqToPlayer(player_, enabled, settings);
            });
        (void)QObject::connect(state_adapter_.get(),
            static_cast<void (UIPlayerStateAdapter::*)(int32_t, size_t)>(&UIPlayerStateAdapter::outputFormatChanged),
            eq.get(),
            &EqualizerView::outputFormatChanged,
            Qt::QueuedConnection);
        (void)QObject::connect(state_adapter_.get(),
            &UIPlayerStateAdapter::samplesChanged,
            eq.get(),
            &EqualizerView::samplesChanged,
            Qt::QueuedConnection);
        eq->outputFormatChanged(state_adapter_->sampleRate(), state_adapter_->outputBufferSize());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
        dialog->setTitle(tr("Equalizer"));
        dialog->setContentWidget(eq.get(), false);
        dialog->setTitleBarBackgroundColor(QColor(qTheme.backgroundColorString()));
        dialog->resize(dialogContentSize(dialog.get(), eq.get()));
        dialog->setFixedSize(dialog->size());
        dialog->exec();
        });

    //setupSystemMenu();

    cd_page_.reset(new CdPage(this));
    cd_page_->playlistPage()->playlist()->setPlaylistId(kCdPlaylistId, kAppSettingCdPlaylistColumnName);
    cd_page_->playlistPage()->playlist()->setHeaderViewHidden(false);
    cd_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);
    file_explorer_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    file_system_service_.reset(new FileSystemService());
    file_system_service_->setScannerThreadPool(ThreadPoolBuilder::makeBackgroundThreadPool());
    file_system_service_->moveToThread(&file_system_service_thread_);
    file_system_service_thread_.start(QThread::LowestPriority);

    album_cover_service_.reset(new AlbumCoverService());
    album_cover_service_->moveToThread(&album_cover_service_thread_);
    album_cover_service_thread_.start(QThread::LowestPriority);

    auto connect_playlist_cover_service = [this](PlaylistTableView* playlist) {
        (void)QObject::connect(playlist->styledDelegate(),
            &PlaylistStyledItemDelegate::findAlbumCover,
            album_cover_service_.get(),
            &AlbumCoverService::onFindAlbumCover,
            Qt::QueuedConnection);
        };

    connect_playlist_cover_service(file_explorer_page_->playlistPage()->playlist());
    connect_playlist_cover_service(cd_page_->playlistPage()->playlist());

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::findAlbumCover,
        album_cover_service_.get(),
        &AlbumCoverService::onFindAlbumCover,
        Qt::QueuedConnection);

    auto connect_playlist_changed = [this](PlaylistTableView* playlist) {
        (void)QObject::connect(playlist,
            &PlaylistTableView::playlistChanged,
            this,
            [this](int32_t) {
                rich_playlist_page_->reload();
                ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
                setCurrentTab(TAB_RICH_PLAYLIST);
            });
        };

    connect_playlist_changed(file_explorer_page_->playlistPage()->playlist());
    connect_playlist_changed(cd_page_->playlistPage()->playlist());

    (void)QObject::connect(album_cover_service_.get(),
        &AlbumCoverService::albumCoverLoaded,
        this,
        [this](int32_t album_id, const QImage& image, bool save_only) {
            Stopwatch total_elapsed;
            Stopwatch stage_elapsed;

            const auto cover = QPixmap::fromImage(image);
            const auto convert_elapsed = stage_elapsed.ElapsedSeconds();
            if (cover.isNull()) {
                XAMP_LOG_DEBUG("Album cover loaded but pixmap is null. album:{} save_only:{}",
                    album_id,
                    save_only);
                return;
            }

            stage_elapsed.reset();
            const auto cover_id = qImageCache.addImage(cover, save_only);
            const auto cache_elapsed = stage_elapsed.ElapsedSeconds();

            stage_elapsed.reset();
            qDaoFacade.album_dao.setAlbumCover(album_id, cover_id);
            const auto db_elapsed = stage_elapsed.ElapsedSeconds();

            stage_elapsed.reset();
            file_explorer_page_->playlistPage()->playlist()->setAlbumCoverId(album_id, cover_id);
            const auto file_playlist_elapsed = stage_elapsed.ElapsedSeconds();

            stage_elapsed.reset();
            cd_page_->playlistPage()->playlist()->setAlbumCoverId(album_id, cover_id);
            const auto cd_playlist_elapsed = stage_elapsed.ElapsedSeconds();

            stage_elapsed.reset();
            rich_playlist_page_->onAlbumCoverLoaded(album_id);
            const auto rich_reload_elapsed = stage_elapsed.ElapsedSeconds();

            XAMP_LOG_DEBUG("Album cover loaded. album:{} cover:{} save_only:{} size:{}x{} convert:{:.3f}s cache:{:.3f}s db:{:.3f}s file_playlist:{:.3f}s cd_playlist:{:.3f}s rich_reload:{:.3f}s total:{:.3f}s",
                album_id,
                cover_id.toStdString(),
                save_only,
                cover.width(),
                cover.height(),
                convert_elapsed,
                cache_elapsed,
                db_elapsed,
                file_playlist_elapsed,
                cd_playlist_elapsed,
                rich_reload_elapsed,
                total_elapsed.ElapsedSeconds());
        },
        Qt::QueuedConnection);

    auto request_album_covers = [this](const QSet<int32_t>& album_ids) {
        for (const auto album_id : album_ids) {
            QMetaObject::invokeMethod(album_cover_service_.get(),
                [service = album_cover_service_.get(), album_id]() {
                    service->onFindAlbumCover(DatabaseCoverId(kInvalidDatabaseId, album_id));
                },
                Qt::QueuedConnection);
        }
        };

    auto* rich_playlist_progress = rich_playlist_page_->progressPage();

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile,
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_.get(),
        &FileSystemViewPage::addPathToPlaylist,
        this,
        [this](const QString& path, bool append_to_playlist) {
            rich_playlist_page_->loadPath(path, append_to_playlist);
            ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
            setCurrentTab(TAB_RICH_PLAYLIST);
        });

    (void)QObject::connect(rich_playlist_progress,
        &ScanFileProgressPage::cancelRequested,
        file_system_service_.get(),
        &FileSystemService::cancelRequested,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::batchInsertDatabase,
        this,
        [this, request_album_covers](const std::vector<std::forward_list<TrackInfo>>& results, int32_t playlist_id) {
            Stopwatch total_elapsed;
            Stopwatch stage_elapsed;
            const auto track_count = CountTrackBatches(results);
            qDatabaseFacade.insertMultipleTrackInfo(results,
                playlist_id,
                QString(),
                DatabaseFacade::kSkipFetchCover);
            const auto insert_seconds = stage_elapsed.ElapsedSeconds();
            stage_elapsed.reset();
            request_album_covers(CollectAlbumIds(results));
            rich_playlist_page_->reload();
            const auto reload_seconds = stage_elapsed.ElapsedSeconds();
            XAMP_LOG_DEBUG("Metadata DB batch write playlist:{} batches:{} tracks:{} insert:{:.3f}s reload:{:.3f}s total:{:.3f}s",
                playlist_id,
                results.size(),
                track_count,
                insert_seconds,
                reload_seconds,
                total_elapsed.ElapsedSeconds());
        },
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::insertDatabase,
        this,
        [this, request_album_covers](const std::forward_list<TrackInfo>& result, int32_t playlist_id) {
            Stopwatch total_elapsed;
            Stopwatch stage_elapsed;
            const auto track_count = CountTracks(result);
            qDatabaseFacade.insertTrackInfo(result,
                playlist_id,
                QString(),
                DatabaseFacade::kSkipFetchCover);
            const auto insert_seconds = stage_elapsed.ElapsedSeconds();
            stage_elapsed.reset();
            QSet<int32_t> album_ids;
            CollectAlbumIds(result, album_ids);
            request_album_covers(album_ids);
            rich_playlist_page_->reload();
            const auto reload_seconds = stage_elapsed.ElapsedSeconds();
            XAMP_LOG_DEBUG("Metadata DB write playlist:{} tracks:{} insert:{:.3f}s reload:{:.3f}s total:{:.3f}s",
                playlist_id,
                track_count,
                insert_seconds,
                reload_seconds,
                total_elapsed.ElapsedSeconds());
        },
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileStart,
        rich_playlist_progress,
        &ScanFileProgressPage::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileProgress,
        rich_playlist_progress,
        &ScanFileProgressPage::setFileProgress,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::foundFileCount,
        rich_playlist_progress,
        &ScanFileProgressPage::setFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFilePath,
        rich_playlist_progress,
        &ScanFileProgressPage::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::remainingTimeEstimation,
        rich_playlist_progress,
        &ScanFileProgressPage::onRemainingTimeEstimation,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readCompleted,
        rich_playlist_progress,
        &ScanFileProgressPage::onReadCompleted,
        Qt::QueuedConnection);

    pushWidget(lrc_page_.get());
    pushWidget(rich_playlist_page_.get());
    pushWidget(file_explorer_page_.get());
    pushWidget(cd_page_.get());

    ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
    setCurrentTab(TAB_RICH_PLAYLIST);

    background_service_.reset(new BackgroundService());
    background_service_->moveToThread(&background_service_thread_);
    background_service_thread_.start(QThread::LowestPriority);

#ifdef Q_OS_WIN
    (void)QObject::connect(this,
        &Xamp::fetchCdInfo,
        background_service_.get(),
        &BackgroundService::onFetchCdInfo);
#endif

    (void)QObject::connect(this,
        &Xamp::searchLyrics,
        background_service_.get(),
        &BackgroundService::onSearchLyrics,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::fetchLyricsCompleted,
        lrc_page_.get(),
        &LrcPage::onFetchLyricsCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readCdTrackInfo,
        this,
        &Xamp::onUpdateCdTrackInfo,
        Qt::QueuedConnection);

    auto connect_encode_jobs = [this](PlaylistTableView* playlist) {
        (void)QObject::connect(playlist,
            &PlaylistTableView::encodeAlacFiles,
            this,
            &Xamp::showEncodeJobs);
        };

    connect_encode_jobs(file_explorer_page_->playlistPage()->playlist());
    connect_encode_jobs(cd_page_->playlistPage()->playlist());

    (void)QObject::connect(file_explorer_page_->playlistPage()->playlist(),
        &PlaylistTableView::findMusicbrainRecording,
        this,
        &Xamp::OnReadMusicBrainzAlbums);

#ifdef Q_OS_WIN
    configureUpdater(false);
    QTimer::singleShot(std::chrono::seconds(5), this, [this]() {
        QSimpleUpdater::getInstance()->checkForUpdates(kUpdateDefinitionsUrl);
        });
#endif
}

void Xamp::OnReadMusicBrainzAlbums(const QList<PlayListEntity>& entities) {
    /*if (HasMultipleKnownAlbums(entities)) {
        XMessageBox::showWarning(
            tr("MusicBrainz tag editing supports one album at a time. Please select tracks from a single album. Tracks with empty album names are allowed."),
            kApplicationTitle,
            true,
            QDialogButtonBox::Ok,
            QDialogButtonBox::Ok,
            this);
        return;
    }*/

    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<MusicbrainzEditPage> eq(new MusicbrainzEditPage(entities, dialog.get()));
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_DRAFT));
    dialog->setTitle(tr("Musicbrainz Edit Page"));
    dialog->setContentWidget(eq.get(), false, false);
    dialog->setMinimumSize(eq->minimumSizeHint());
    dialog->resize(dialogSizeFromHost(dialog.get(), eq.get(), main_window_, 0.8));
    dialog->exec();
}

void Xamp::setupSystemMenu() {
    if (main_window_ == nullptr) {
        return;
    }

    main_window_->clearSystemMenuActions();

    preference_action_ = new QAction(tr("Preference") + "..."_str, this);
    (void)QObject::connect(preference_action_, &QAction::triggered, this, &Xamp::showPreference);
    main_window_->addSystemMenuAction(preference_action_);

#ifdef Q_OS_WIN
    auto* check_for_update_action = new QAction(tr("Check for update"), this);
    (void)QObject::connect(check_for_update_action, &QAction::triggered, this, &Xamp::onCheckForUpdate);
    main_window_->addSystemMenuAction(check_for_update_action);
#endif

    auto* about_action = new QAction(tr("About") + "..."_str, this);
    (void)QObject::connect(about_action, &QAction::triggered, this, &Xamp::showAbout);
    main_window_->addSystemMenuAction(about_action);

    auto* log_action = new QAction(tr("log Viewer") + "..."_str, this);
    (void)QObject::connect(log_action, &QAction::triggered, this, &Xamp::showLogViewer);
    main_window_->addSystemMenuAction(log_action);
}

void Xamp::showPreference() {
    const QScopedPointer<XDialog> dialog(new XDialog(this));
    const QScopedPointer<PreferencePage> preference_page(new PreferencePage(dialog.get()));
    preference_page->loadSettings();
    dialog->setContentWidget(preference_page.get());
    dialog->setFixedSize(dialog->size());
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
    dialog->setTitle(tr("Preference"));
    dialog->exec();
    preference_page->saveAll();
}

void Xamp::showLogViewer() {
    static QPointer<XDialog> log_dialog;
    if (log_dialog != nullptr) {
        log_dialog->show();
        log_dialog->raise();
        log_dialog->activateWindow();
        return;
    }

    auto* dialog = new XDialog(nullptr, false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlag(Qt::Window, true);
    dialog->setTitle(tr("log Viewer"));
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_REPORT_BUG));

    auto* log_view = new LogView(dialog);
    log_view->loadLogFile("logs/xamp.log"_str);
    log_view->setMinimumSize(QSize(900, 480));
    dialog->setContentWidget(log_view, false, false);
    dialog->resize(dialogSizeFromHost(dialog, log_view, main_window_, 0.6));
    log_dialog = dialog;
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void Xamp::showEncodeJobs(int32_t encode_type, const QList<PlayListEntity>& entities) {
    if (background_service_ == nullptr || entities.isEmpty()) {
        return;
    }

    const auto dir_name = getExistingDirectory(this, tr("Select output directory"));
    if (dir_name.isEmpty()) {
        return;
    }

    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<EncodeJobWidget> encode_job_widget(new EncodeJobWidget(dialog.get()));
    encode_job_widget->setMinimumSize(QSize(1100, 520));

    const auto jobs = encode_job_widget->addJobs(encode_type, entities);
    if (jobs.isEmpty()) {
        return;
    }

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::updateJobProgress,
        encode_job_widget.get(),
        &EncodeJobWidget::onUpdateProgress,
        Qt::QueuedConnection);
    (void)QObject::connect(background_service_.get(),
        &BackgroundService::jobError,
        encode_job_widget.get(),
        &EncodeJobWidget::onJobError,
        Qt::QueuedConnection);

    QMetaObject::invokeMethod(background_service_.get(),
        [service = background_service_.get(), dir_name, jobs]() {
            service->onAddJobs(dir_name, jobs);
        },
        Qt::QueuedConnection);

    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EXPORT_FILE));
    dialog->setTitle(tr("encode Jobs"));
    dialog->setContentWidget(encode_job_widget.get(), false);
    dialog->resize(dialogSizeFromHost(dialog.get(), encode_job_widget.get(), main_window_, 0.75));
    dialog->exec();
}

void Xamp::configureUpdater(bool notify_on_finish) {
#ifdef Q_OS_WIN
    auto* updater = QSimpleUpdater::getInstance();
    updater->setModuleName(kUpdateDefinitionsUrl, kApplicationTitle);
    updater->setModuleVersion(kUpdateDefinitionsUrl, kApplicationVersion);
    updater->setPlatformKey(kUpdateDefinitionsUrl, "windows"_str);
    updater->setNotifyOnUpdate(kUpdateDefinitionsUrl, true);
    updater->setNotifyOnFinish(kUpdateDefinitionsUrl, notify_on_finish);
    updater->setDownloaderEnabled(kUpdateDefinitionsUrl, true);
    updater->setUseCustomInstallProcedures(kUpdateDefinitionsUrl, true);
    updater->setUserAgentString(kUpdateDefinitionsUrl, kDefaultUserAgent);

    const auto download_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (!download_dir.isEmpty()) {
        updater->setDownloadDir(kUpdateDefinitionsUrl, download_dir);
    }

    (void)QObject::connect(updater,
        &QSimpleUpdater::downloadFinished,
        this,
        &Xamp::installDownloadedUpdate,
        Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
#else
    (void)notify_on_finish;
#endif
}

void Xamp::installDownloadedUpdate(const QString& url, const QString& filepath) {
#ifdef Q_OS_WIN
    if (url != kUpdateDefinitionsUrl || filepath.isEmpty()) {
        return;
    }

    const QStringList args{
        "/SILENT"_str,
        "/SUPPRESSMSGBOXES"_str,
        "/NORESTART"_str,
        "/CLOSEAPPLICATIONS"_str,
    };

    if (QProcess::startDetached(filepath, args)) {
        qApp->quit();
    }
    else {
        XMessageBox::showError(tr("Failed to start the update installer."));
    }
#else
    (void)url;
    (void)filepath;
#endif
}

void Xamp::onCheckForUpdate() {
#ifdef Q_OS_WIN
    configureUpdater(true);
    QSimpleUpdater::getInstance()->checkForUpdates(kUpdateDefinitionsUrl);
#endif
}

void Xamp::showAbout() {
    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<AboutPage> about_page(new AboutPage(dialog.get()));
    (void)QObject::connect(about_page.get(),
        &AboutPage::CheckForUpdate,
        this,
        &Xamp::onCheckForUpdate);
    (void)QObject::connect(about_page.get(),
        &AboutPage::RestartApp,
        []() {
            qApp->exit(kRestartExistCode);
        });
    dialog->setIcon(qTheme.applicationIcon());
    dialog->setTitle(tr("About"));
    dialog->setContentWidget(about_page.get(), false);
    dialog->setFixedSize(dialog->size());
    dialog->exec();

}

void Xamp::setSeekPosValue(double stream_time) {
    if (main_window_ == nullptr) {
        return;
    }
    auto duration = player_->getDuration();
    const auto full_text = isMoreThan1Hours(duration);
    if (duration - stream_time >= 0.0) {
        ui_.endPosLabel->setText(formatDuration(duration - stream_time, full_text));
        const auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
        ui_.seekSlider->setValue(stream_time_as_ms);
        ui_.startPosLabel->setText(formatDuration(stream_time, full_text));
        main_window_->setTaskbarProgress(static_cast<int32_t>(100.0 * ui_.seekSlider->value() / ui_.seekSlider->maximum()));
        lrc_page_->lyrics()->setLrcTime(stream_time_as_ms);
    }
}

void Xamp::onSampleTimeChanged(double stream_time) {
    if (!player_ || is_seeking_) {
        return;
    }
    if (player_->getState() == PlayerState::PLAYER_STATE_RUNNING) {
        setSeekPosValue(stream_time);
    }
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
    if (!player_) {
        return;
    }

    if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
        main_window_->resetTaskbarProgress();
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(formatDuration(0));
        playNextItem(1);
    }
}

void Xamp::onDeviceStateChanged(DeviceState state, const QString& device_id) {
    XAMP_LOG_DEBUG("OnDeviceStateChanged: {}", state);

    if (state == DeviceState::DEVICE_STATE_REMOVED) {
        player_->stop(true, true, true);
        ui_.seekSlider->setValue(0);
        ui_.seekSlider->clearWaveform();
        ui_.startPosLabel->setText(formatDuration(0));
        ui_.endPosLabel->setText(formatDuration(0));
    }

    if (state == DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE) {
        return;
    }

    if (qAppSettings.valueAsBool(kAppSettingAutoSelectNewDevice)) {
        initialDeviceList(device_id.toStdString());
    }
    else {
        initialDeviceList();
    }
}

void Xamp::onUpdateCdTrackInfo(const QString& disc_id, const std::forward_list<TrackInfo>& track_infos) {
    qDatabaseFacade.insertTrackInfo(track_infos,
        kCdPlaylistId,
        disc_id,
        nullptr);

    cd_page_->playlistPage()->playlist()->reload();
    cd_page_->showPlaylistPage(true);
}

void Xamp::showNaviBarButton() {
    constexpr auto kAnimationDuration = 100;
    constexpr auto kMaxWidth = 180;
    constexpr auto kMinWidth = 50;

    auto start_width = ui_.sliderFrame2->maximumWidth();
    auto end_width = (start_width == kMinWidth) ? kMaxWidth : kMinWidth;
    qAppSettings.setValue(kAppSettingHideNaviBar, end_width == kMinWidth);

    auto* animation = new QPropertyAnimation(ui_.sliderFrame2, "maximumWidth");
    animation->setDuration(kAnimationDuration);
    animation->setStartValue(start_width);
    animation->setEndValue(end_width);
    animation->setEasingCurve(QEasingCurve::OutQuad);

    (void)QObject::connect(animation, &QPropertyAnimation::finished, this, [this, end_width]() {
        ui_.sliderFrame2->setMinimumWidth(end_width);
        ui_.sliderFrame2->setMaximumWidth(end_width);
        if (end_width == kMinWidth) {
            ui_.naviBar->collapse();
        }
        else {
            ui_.naviBar->expand();
        }
        });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Xamp::addDropFileItem(const QUrl& url) {
    spectrogram_tracks_playback_ = false;
    playLocalFile(url.toLocalFile());
}

void Xamp::playPrevious() {
    if (player_->getState() == PlayerState::PLAYER_STATE_STOPPED
        || player_->getState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(-1);
    }
    else {
        stopPlay();
    }
}

void Xamp::playNext() {
    if (player_->getState() == PlayerState::PLAYER_STATE_STOPPED
        || player_->getState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(1);
    }
    else {
        stopPlay();
    }
}

void Xamp::playOrPause() {
    if (player_->getState() == PlayerState::PLAYER_STATE_RUNNING) {
        qTheme.setPlayOrPauseButton(ui_.playButton, false);
        player_->pause();
        main_window_->setTaskbarPlayerPaused();
    }
    else if (player_->getState() == PlayerState::PLAYER_STATE_PAUSED) {
        qTheme.setPlayOrPauseButton(ui_.playButton, true);
        player_->resume();
        main_window_->setTaskbarPlayingResume();
    }
    else if (player_->getState() == PlayerState::PLAYER_STATE_STOPPED
        || player_->getState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(1);
    }
}

void Xamp::playNextItem(int32_t forward) {
    if (!rich_playlist_page_->playNextItem(forward)) {
        ui_.seekSlider->clearWaveform();
    }
}

void Xamp::stopPlay() {
    player_->stop();
    qTheme.setPlayOrPauseButton(ui_.playButton, false);
	ui_.seekSlider->setValue(0);
    ui_.seekSlider->clearWaveform();
	ui_.startPosLabel->setText(formatDuration(0));
    ui_.endPosLabel->setText(formatDuration(0));
}
