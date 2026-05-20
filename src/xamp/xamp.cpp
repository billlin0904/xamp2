#include <thememanager.h>
#include <version.h>
#include <xamp.h>
#include <deviceselectormenu.h>
#include <sharedmodeplayback.h>
#include <QSimpleUpdater.h>

#include <algorithm>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QGuiApplication>
#include <QMap>
#include <QProcess>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>

#include <base/ithreadpoolexecutor.h>
#include <base/crashhandler.h>

#include <player/audio_player.h>
#include <stream/api.h>
#include <stream/idspmanager.h>
#include <stream/mqafilestream.h>

#include <output_device/audiodevicemanager.h>

#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>
#include <widget/equalizerview.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/filesystemviewpage.h>
#include <widget/richplaylistpage.h>
#include <widget/databasefacade.h>
#include <widget/playlisttableview.h>
#include <widget/playlistpage.h>
#include <widget/logview.h>
#include <widget/preferencepage.h>
#include <widget/cdpage.h>
#include <widget/waveformslider.h>
#include <widget/musicbrainzeditpage.h>
#include <widget/aboutpage.h>
#include <widget/scanfileprogresspage.h>
#include <widget/xmessagebox.h>
#include <widget/worker/filesystemservice.h>

#include <style_util.h>

namespace {
    const auto kUpdateDefinitionsUrl = "https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json"_str;

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
        player->SetParametricEq(enabled, settings);
    }
}

Xamp::Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer>& player)
    : IXFrame(parent)
	, player_(player) {
    thread_pool_ = ThreadPoolBuilder::MakeBackgroundThreadPool();
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_.setupUi(this);
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
    switch (table_id) {
    case TAB_MUSIC_LIBRARY:
        break;
    case TAB_FILE_EXPLORER:
        ui_.currentView->setCurrentWidget(file_explorer_page_.get());
        return;
        break;
    case TAB_RICH_PLAYLIST:
        ui_.currentView->setCurrentWidget(rich_playlist_page_.get());
        return;
        break;
    case TAB_LYRICS:
        ui_.currentView->setCurrentWidget(lrc_page_.get());
        break;
    case TAB_CD:
        ui_.currentView->setCurrentWidget(cd_page_.get());
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
    XAMP_LOG_DEBUG("Initial device list");

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

    const auto selected_device = device_menu_->rebuild(player_->GetAudioDeviceManager(),
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

    if (file_system_service_ != nullptr) {
        file_system_service_->cancelRequested();
    }
    quit_and_wait_thread(file_system_service_thread_);

    if (background_service_ != nullptr) {
        background_service_->cancelRequested();
    }
    quit_and_wait_thread(background_service_thread_);
    qGuiDb.Close();
    XampCrashHandler.Cleanup();
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
            setVolume(player_->GetVolume() + 1);
            }},
        { QKeySequence(Qt::Key_VolumeDown), [this]() {
            setVolume(player_->GetVolume() - 1);
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
    const QSize cover_size(ui_.coverLabel->size().width() - image_util::kPlaylistImageRadius,
        ui_.coverLabel->size().height() - image_util::kPlaylistImageRadius);
    const auto ui_cover = image_util::roundImage(
        image_util::resizeImage(cover, cover_size, false),
        image_util::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);
}

void Xamp::playLocalFile(const QString& file_name, bool queue) {
    auto file_sample_rate = 44100;
    auto file_duration = 0.0;
    QPixmap embedded_cover = qTheme.unknownCover();
    TrackInfo track_info;

    try {
        auto metadata_reader = MakeMetadataReader();
        metadata_reader->Open(file_name.toStdWString());

        auto metadata_opt = metadata_reader->Extract();
        if (metadata_opt.has_value()) {
            track_info = metadata_opt.value();
            file_sample_rate = track_info.sample_rate;
            file_duration = track_info.duration;
            auto buffer = metadata_reader->ReadEmbeddedCover();
            if (buffer.has_value() && buffer.value().size() > 0) {
                embedded_cover.loadFromData(reinterpret_cast<uchar*>(buffer.value().data()), buffer.value().size());
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
        player_->Stop();
    }
    catch (...) {
        logAndShowMessage(std::current_exception());
        return;
    }

    const auto is_shared_device = player_->GetAudioDeviceManager()->IsSharedDevice(
        device_info_.value().device_type_id);
    const auto is_asio_device = player_->GetAudioDeviceManager()->IsASIODevice(
        device_info_.value().device_type_id);
    const auto playback_plan = resolvePlaybackPlan(device_info_.value(),
        file_sample_rate,
        IsDsdFile(file_name.toStdWString()),
        is_shared_device,
        is_asio_device);

    if (playback_plan.needs_resample) {
        player_->GetDspManager()->AddPreDSP(makeSoxrSampleRateConverter(playback_plan.target_sample_rate));
    }

    if (qAppSettings.valueAsBool(kAppSettingEnableEQ)) {
        if (const auto eq_settings = storedParametricEqSettings()) {
            player_->GetDspConfig().Create(DspConfig::kEQSettings, *eq_settings);
            player_->GetDspManager()->AddParametricEq();
        }
    }
    else {
        player_->GetDspManager()->RemoveParametricEq();
    }

    try {
        auto file_stream = StreamFactory::MakeFileStream(file_name.toStdWString(),
            playback_plan.output_mode,
            playback_plan.use_mqa_decode);
        const auto byte_format = resolvePreparedPlaybackByteFormat(
            playback_plan,
            dynamic_cast<MqaFileStream*>(file_stream.get()) != nullptr);

        player_->Open(std::move(file_stream),
            device_info_.value(),
            playback_plan.target_sample_rate,
            playback_plan.output_mode);
        player_->GetDspManager()->SetSampleWriter();
        player_->PrepareToPlay(byte_format);
        player_->BufferStream(0, 0, std::nullopt);
        player_->Play();
    }
    catch (...) {
        logAndShowMessage(std::current_exception());
        return;
	}

    qTheme.setPlayOrPauseButton(ui_.playButton, true);

    auto playback_format = getPlaybackFormat(player_.get());
    lrc_page_->lyrics()->loadFile(file_name);
    lrc_page_->setCover(embedded_cover);
    lrc_page_->format()->setText(format2String(playback_format,
        ".FLAC"_str,
        fromStdStringView(device_info_.value().desc)));
    lrc_page_->setBackground(image_util::blurImage(thread_pool_,
        embedded_cover,
        lrc_page_->size()));
    rich_playlist_page_->setNowPlaying(track_info, embedded_cover);

    main_window_->setIconicThumbnail(embedded_cover);

    double duration = 0;
    duration = Round(file_duration) * 1000;
    ui_.seekSlider->setRange(0, duration);
    ui_.seekSlider->setValue(0);
    ui_.seekSlider->loadFile(file_name);

    ui_.titleLabel->setText(QString::fromStdWString(track_info.title));
    ui_.artistLabel->setText(QString::fromStdWString(track_info.artist));

    ui_.coverLabel->setPixmap(
        image_util::resizeImage(embedded_cover,
            ui_.coverLabel->size()));
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
            kDefaultPlaylistId,
            StoreType::LOCAL_STORE);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kFileSystemPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("FileSystem Playlist"),
            kFileSystemPlaylistId,
            StoreType::LOCAL_STORE);
    }    
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kCdPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("CD Playlist"),
            kCdPlaylistId,
            StoreType::LOCAL_STORE);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kAlbumPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("Album Playlist"),
            kAlbumPlaylistId,
            StoreType::LOCAL_STORE);
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

    ui_.naviBar->addTab(translateText("Now Playing"),
        TAB_LYRICS,
        qTheme.fontIcon(Glyphs::ICON_PLAYING));
    ui_.naviBar->addTab(translateText("Rich Playlist"),
        TAB_RICH_PLAYLIST,
        qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
    ui_.naviBar->addTab(translateText("File Browser"),
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

    state_adapter_.reset(new UIPlayerStateAdapter(this));
    player_->SetStateAdapter(state_adapter_);

    ui_.mutedButton->setAudioPlayer(player_);
    ui_.mutedButton->updateState();

    auto f = font();
    f.setPointSize(qTheme.fontSize(9));
    ui_.titleLabel->setFont(f);
    f.setPointSize(qTheme.fontSize(8));
    ui_.artistLabel->setFont(f);

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
        &UIPlayerStateAdapter::deviceChanged,
        this,
        &Xamp::onDeviceStateChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_->playlistPage()->playlist(),
        &PlaylistTableView::playMusic,
        this,
        [this](int32_t playlist_id, const PlayListEntity& item, bool is_play) {
            playLocalFile(item.file_path, is_play);
        });

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::playMusic,
        this,
        [this](int32_t playlist_id, const PlayListEntity& item, bool is_play) {
            playLocalFile(item.file_path, is_play);
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
        try {
            is_seeking_ = true;
            player_->Seek(value / 1000.0);
            qTheme.setPlayOrPauseButton(ui_.playButton, true);
            main_window_->setTaskbarPlayingResume();
        }
        catch (...) {
            player_->Stop(false);
            logAndShowMessage(std::current_exception());
        }
        is_seeking_ = false;
        });

    (void)QObject::connect(ui_.eqButton, &QToolButton::clicked, [this]() {
        if (player_->GetDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->GetDsdModes() == DsdModes::DSD_MODE_NATIVE) {
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
            &UIPlayerStateAdapter::outputFormatChanged,
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

	setCurrentTab(TAB_LYRICS);

    setupSystemMenu();

    cd_page_.reset(new CdPage(this));
    cd_page_->playlistPage()->playlist()->setPlaylistId(kCdPlaylistId, kAppSettingCdPlaylistColumnName);
    cd_page_->playlistPage()->playlist()->setHeaderViewHidden(false);
    cd_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    file_system_service_.reset(new FileSystemService());
    file_system_service_->moveToThread(&file_system_service_thread_);
    file_system_service_thread_.start(QThread::LowestPriority);

    auto* rich_playlist_progress = rich_playlist_page_->progressPage();

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile,
        Qt::QueuedConnection);

    (void)QObject::connect(rich_playlist_progress,
        &ScanFileProgressPage::cancelRequested,
        file_system_service_.get(),
        &FileSystemService::cancelRequested,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::batchInsertDatabase,
        this,
        [this](const std::vector<std::forward_list<TrackInfo>>& results, int32_t playlist_id) {
            qDatabaseFacade.insertMultipleTrackInfo(results, playlist_id, StoreType::LOCAL_STORE);
            rich_playlist_page_->reload();
        },
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::insertDatabase,
        this,
        [this](const std::forward_list<TrackInfo>& result, int32_t playlist_id) {
            qDatabaseFacade.insertTrackInfo(result, playlist_id, StoreType::LOCAL_STORE);
            rich_playlist_page_->reload();
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

    (void)QObject::connect(this,
        &Xamp::fetchCdInfo,
        background_service_.get(),
        &BackgroundService::onFetchCdInfo);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readCdTrackInfo,
        this,
        &Xamp::onUpdateCdTrackInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_->playlistPage()->playlist(),
        &PlaylistTableView::findMusicbrainRecording,
        this,
        &Xamp::OnReadMusicBrainzAlbums);

    configureUpdater(false);
    QTimer::singleShot(std::chrono::seconds(5), this, [this]() {
        QSimpleUpdater::getInstance()->checkForUpdates(kUpdateDefinitionsUrl);
        });
}

void Xamp::OnReadMusicBrainzAlbums(const QList<PlayListEntity>& entities) {
    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<MusicbrainzEditPage> eq(new MusicbrainzEditPage(entities, dialog.get()));
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_DRAFT));
    dialog->setTitle(tr("Musicbrainz Edit Page"));
    dialog->setContentWidget(eq.get(), false, false);
    dialog->setMinimumSize(eq->minimumSizeHint());
    dialog->resize(dialogContentSize(dialog.get(), eq.get()));
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

    auto* check_for_update_action = new QAction(tr("Check for update"), this);
    (void)QObject::connect(check_for_update_action, &QAction::triggered, this, &Xamp::onCheckForUpdate);
    main_window_->addSystemMenuAction(check_for_update_action);

    auto* about_action = new QAction(tr("About") + "..."_str, this);
    (void)QObject::connect(about_action, &QAction::triggered, this, &Xamp::showAbout);
    main_window_->addSystemMenuAction(about_action);

    auto* log_action = new QAction(tr("Log Viewer") + "..."_str, this);
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
    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    const QScopedPointer<XDialog> dialog(new XDialog(this));
    dialog->setTitle(tr("Log Viewer"));
    QScopedPointer<LogView> log_view(new LogView(dialog.get()));
    log_view->loadLogFile("logs/xamp.log"_str);
    log_view->setFixedSize(QSize(1200, 600));
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_REPORT_BUG));
    dialog->setContentWidget(log_view.get(), false);
    dialog->setFixedSize(dialog->size());
    dialog->exec();
}

void Xamp::configureUpdater(bool notify_on_finish) {
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
}

void Xamp::installDownloadedUpdate(const QString& url, const QString& filepath) {
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
}

void Xamp::onCheckForUpdate() {
    configureUpdater(true);
    QSimpleUpdater::getInstance()->checkForUpdates(kUpdateDefinitionsUrl);

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
    auto duration = player_->GetDuration();
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
    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
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
        player_->Stop(true, true, true);
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
        StoreType::LOCAL_STORE,
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
    playLocalFile(url.toLocalFile());
}

void Xamp::playPrevious() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED
        || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(-1);
    }
    else {
        stopPlay();
    }
}

void Xamp::playNext() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED
        || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(1);
    }
    else {
        stopPlay();
    }
}

void Xamp::playOrPause() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
        qTheme.setPlayOrPauseButton(ui_.playButton, false);
        player_->Pause();
        main_window_->setTaskbarPlayerPaused();
    }
    else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
        qTheme.setPlayOrPauseButton(ui_.playButton, true);
        player_->Resume();
        main_window_->setTaskbarPlayingResume();
    }
    else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED
        || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(1);
    }
}

void Xamp::playNextItem(int32_t forward) {
    if (!rich_playlist_page_->playNextItem(forward)) {
        ui_.seekSlider->clearWaveform();
    }
}

void Xamp::stopPlay() {
    player_->Stop();
    qTheme.setPlayOrPauseButton(ui_.playButton, false);
	ui_.seekSlider->setValue(0);
    ui_.seekSlider->clearWaveform();
	ui_.startPosLabel->setText(formatDuration(0));
    ui_.endPosLabel->setText(formatDuration(0));
}
