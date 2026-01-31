#include <thememanager.h>
#include <version.h>
#include <xamp.h>

#include <QWidgetAction>
#include <QActionGroup>
#include <QDirIterator>

#include <base/ithreadpoolexecutor.h>
#include <player/audio_player.h>
#include <stream/idspmanager.h>

#include <output_device/audiodevicemanager.h>
#include <output_device/idevicetype.h>

#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>
#include <widget/equalizerview.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/actionmap.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/filesystemviewpage.h>
#include <widget/databasefacade.h>
#include <widget/playbackqueueviewpage.h>
#include <widget/playlisttableview.h>
#include <widget/playlistpage.h>
#include <widget/logview.h>
#include <widget/preferencepage.h>

#include <style_util.h>

namespace {
    
}

Xamp::Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer>& player)
    : IXFrame(parent)
	, player_(player) {
    thread_pool_ = ThreadPoolBuilder::MakeBackgroundThreadPool();
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_.setupUi(this);
}

Xamp::~Xamp() {
    if (main_window_ != nullptr) {
        main_window_->saveAppGeometry();
    }
}

QString Xamp::translateDeviceDescription(const IDeviceType* device_type) {
    static const QMap<std::string_view, ConstexprQString> lut{
        { "WASAPI (Exclusive Mode)", QT_TRANSLATE_NOOP("Xamp", "WASAPI (Exclusive Mode)") },
        { "WASAPI (Shared Mode)",    QT_TRANSLATE_NOOP("Xamp", "WASAPI (Shared Mode)") },
        { "Null Output",             QT_TRANSLATE_NOOP("Xamp", "Null Output") },
        { "XAudio2",                 QT_TRANSLATE_NOOP("Xamp", "XAudio2") },
        { "ASIO",                    QT_TRANSLATE_NOOP("Xamp", "ASIO") },
    };
    const auto str = lut.value(device_type->GetDescription(),
        fromStdStringView(device_type->GetDescription()));
    return tr(str.data());
}

QWidgetAction* Xamp::createDeviceMenuWidget(const QString& desc) {
    auto* desc_label = new QLabel(desc);

    desc_label->setObjectName("textSeparator"_str);

    QFont f = font();
    f.setPointSize(qTheme.fontSize(9));
    f.setBold(true);
    desc_label->setFont(f);
    desc_label->setAlignment(Qt::AlignCenter);

    auto* device_type_frame = new QFrame();
    device_type_frame->setObjectName("deviceTypeFrame"_str);
    qTheme.setFrameBackgroundColor(device_type_frame);

    auto* default_layout = new QHBoxLayout(device_type_frame);
    default_layout->addWidget(desc_label);

    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 0, 0, 0);
    device_type_frame->setLayout(default_layout);

    auto* separator = new QWidgetAction(this);
    separator->setDefaultWidget(device_type_frame);

    device_type_frame_.push_back(device_type_frame);
    return separator;
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
    case TAB_PLAYLIST:
        break;
    case TAB_LYRICS:
        ui_.currentView->setCurrentWidget(lrc_page_.get());
        break;
    case TAB_CD:
        break;
    case TAB_YT_MUSIC_PLAYLIST:
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

    auto f = qTheme.defaultFont();
    f.setPointSize(10);
    ui_.deviceDescLabel->setFont(f);

    auto* menu = ui_.selectDeviceButton->menu();
    if (!menu) {
        menu = new XMenu();
        ui_.selectDeviceButton->setMenu(menu);
    }

    menu->clear();

    DeviceInfo init_device_info;
    auto is_find_setting_device = false;

    auto* device_action_group = new QActionGroup(this);
    device_action_group->setExclusive(true);

    OrderedMap<std::string, QAction*> device_id_action;

    const auto device_type_id = qAppSettings.valueAsId(kAppSettingDeviceType);
    auto current_device_id = device_id;
    if (current_device_id.empty()) {
        current_device_id = qAppSettings.valueAsString(kAppSettingDeviceId).toStdString();
    }
    const auto& device_manager = player_->GetAudioDeviceManager();

    auto max_width = 0;
    const QFontMetrics metrics(ui_.deviceDescLabel->font());

    for (auto itr = device_manager->Begin(); itr != device_manager->End(); ++itr) {
        const auto device_type = itr->second();
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            return;
        }

        menu->addSeparator();
        menu->addAction(createDeviceMenuWidget(translateDeviceDescription(device_type.get())));

        for (const auto& device_info : device_info_list) {
            auto* device_action = new QAction(qTheme.connectTypeIcon(device_info.connect_type), QString::fromStdWString(device_info.name), this);
            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_action->setChecked(false);
            device_id_action[device_info.device_id] = device_action;

            if (device_info_) {
                max_width = std::max(metrics.horizontalAdvance(QString::fromStdWString(device_info_.value().name)), max_width);
            }

            auto trigger_callback = [device_info, this]() {
                qTheme.setDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info.connect_type);
                device_info_ = device_info;
                ui_.deviceDescLabel->setText(QString::fromStdWString(device_info_.value().name));
                qAppSettings.setValue(kAppSettingDeviceType, device_info_.value().device_type_id);
                qAppSettings.setValue(kAppSettingDeviceId, device_info_.value().device_id);
                };

            (void)QObject::connect(device_action, &QAction::triggered, trigger_callback);
            menu->addAction(device_action);

            if (device_type_id == device_info.device_type_id && current_device_id == device_info.device_id) {
                device_info_ = device_info;
                is_find_setting_device = true;
                device_action->setChecked(true);
                // BUG! not show current connect icon.
                qTheme.setDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info.connect_type);
                ui_.deviceDescLabel->setMinimumWidth(max_width + 60);
                ui_.deviceDescLabel->setText(QString::fromStdWString(device_info_.value().name));
            }
        }

        if (!is_find_setting_device) {
            auto itr = std::find_if(device_info_list.begin(),
                device_info_list.end(), [](const auto& info) {
                    return info.is_default_device && !IsExclusiveDevice(info);
                });
            if (itr != device_info_list.end()) {
                init_device_info = (*itr);
            }
        }
    }

    if (!is_find_setting_device) {
        device_info_ = init_device_info;
        qTheme.setDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info_.value().connect_type);
        if (device_id_action.find(device_info_.value().device_id) != device_id_action.end()) {
            device_id_action[device_info_.value().device_id]->setChecked(true);
            ui_.deviceDescLabel->setMinimumWidth(max_width + 60);
            ui_.deviceDescLabel->setText(QString::fromStdWString(device_info_.value().name));
            qAppSettings.setValue(kAppSettingDeviceType, device_info_.value().device_type_id);
            qAppSettings.setValue(kAppSettingDeviceId, device_info_.value().device_id);
        }
        XAMP_LOG_DEBUG("Use default device Id : {}", device_info_.value().device_id);
    }
}

QString Xamp::translateText(const std::string_view& text) {
    return tr(text.data());
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

}

void Xamp::drivesChanges(const QList<DriveInfo>& drive_infos) {

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
#ifdef Q_OS_WIN            
    const auto kDefaultFormat = AudioFormat::k16BitPCM441Khz;
#else
    const auto kDefaultFormat = AudioFormat::k16BitPCM48Khz;
#endif

    uint32_t target_sample_rate = 0;
    auto byte_format = ByteFormat::SINT32;
    auto use_mqa_decode = false;
	auto is_file_path = true;

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

    if (queue) {
        playback_queue_page_->addQueue(track_info);
    }

    player_->Stop();

    if (player_->GetAudioDeviceManager()->IsSharedDevice(device_info_.value().device_type_id)) {
        AudioFormat default_format;
        if (device_info_.value().default_format) {
            default_format = device_info_.value().default_format.value();
        }
        else {
            default_format = kDefaultFormat;
        }

        auto sample_rate = is_file_path
            ? file_sample_rate : AudioFormat::k16BitPCM48Khz.GetSampleRate();

        if (sample_rate != default_format.GetSampleRate()) {
            target_sample_rate = default_format.GetSampleRate();
        }
        else {
            target_sample_rate = sample_rate;
        }
        byte_format = ByteFormat::SINT16;
        player_->GetDspManager()->AddPreDSP(
            makeSoxrSampleRateConverter(target_sample_rate));
    }
    else {
        byte_format = ByteFormat::SINT24;
        use_mqa_decode = true;
    }

    if (qAppSettings.valueAsBool(kAppSettingEnableEQ)) {
        if (qAppSettings.contains(kAppSettingEQName)) {
            auto [name, _] = qAppSettings.eqSettings();
            auto eq_settings = qAppSettings.eqPreset()[name];
            player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, eq_settings);
            player_->GetDspManager()->AddParametricEq();
        }
    }

    try {
        player_->Open(file_name.toStdWString(),
            device_info_.value(),
            target_sample_rate,
            DsdModes::DSD_MODE_PCM,
            0.0f,
            use_mqa_decode);
        player_->GetDspManager()->SetSampleWriter();
        if (!player_->IsMQA()) {
            byte_format = ByteFormat::SINT32;
        }
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

    main_window_->setIconicThumbnail(embedded_cover);

    double duration = 0;
    duration = Round(file_duration) * 1000;
    ui_.seekSlider->setRange(0, duration);
    ui_.seekSlider->setValue(0);

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

    if (!qDaoFacade.playlist_dao.isPlaylistExist(kFileSystemPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("FileSystem Playlist"),
            kFileSystemPlaylistId,
            StoreType::LOCAL_STORE);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kNowPlayingPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("NowPlaying Playlist"),
            kNowPlayingPlaylistId,
            StoreType::LOCAL_STORE);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kNextInQueuePlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("NextInQueue Playlist"),
            kNextInQueuePlaylistId,
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

    ui_.repeatButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));

    ui_.naviBar->addTab(translateText("Now Playing"),
        TAB_LYRICS,
        qTheme.fontIcon(Glyphs::ICON_PLAYING));
    ui_.naviBar->addTab(translateText("File Browser"),
        TAB_FILE_EXPLORER,
        qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_.naviBar->setCurrentIndex(TAB_LYRICS);

    if (qAppSettings.valueAsBool(kAppSettingHideNaviBar)) {
        ui_.sliderFrame2->setMaximumWidth(50);
        ui_.naviBar->collapse();
    }
    else {
        ui_.sliderFrame2->setMaximumWidth(180);
        ui_.naviBar->expand();
    }

    lrc_page_.reset(new LrcPage(this));
    file_explorer_page_.reset(new FileSystemViewPage(this));

    pushWidget(lrc_page_.get());
    pushWidget(file_explorer_page_.get());

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

    (void)QObject::connect(file_explorer_page_.get(),
        &FileSystemViewPage::addPathToPlaylist, [this](const QString& parent_dir_path, bool append_to_playlist) {
        QDirIterator itr(parent_dir_path,
            getTrackInfoFileNameFilter(),
            QDir::NoDotAndDotDot | QDir::Files);

        auto reader = MakeMetadataReader();

        while (itr.hasNext()) {
            const auto path_str = toNativeSeparators(itr.next());
            auto file_path = path_str.toStdWString();

            try {
                reader->Open(file_path);
                auto track_info = reader->Extract();
                if (track_info) {
                    if (track_info.value().sample_rate == 0) {
                        auto file_stream = StreamFactory::MakeFileStream(file_path);
                        file_stream->OpenFile(file_path);
                        auto sampler_rate = file_stream->GetFormat().GetSampleRate();
                        track_info.value().sample_rate = sampler_rate;
                    }
                    playback_queue_page_->addQueue(track_info.value());
                }
            }
            catch (...) {
                logAndShowMessage(std::current_exception());
            }
        }
    });

    (void)QObject::connect(file_explorer_page_->playlistPage()->playlist(),
        &PlaylistTableView::addPlaylist, [this](int32_t playlist_id, const QList<PlayListEntity>& entities) {
            auto metadata_reader = MakeMetadataReader();
            Q_FOREACH(auto entity, entities) {
                try {
                    metadata_reader->Open(entity.file_path.toStdWString());

                    auto track_info_opt = metadata_reader->Extract();
                    if (track_info_opt.has_value()) {
                        playback_queue_page_->addQueue(track_info_opt.value());
                    }
                }
                catch (...) {
                    logAndShowMessage(std::current_exception());
                }
            }
        });

    (void)QObject::connect(ui_.playButton, &QToolButton::clicked, [this]() {
        playOrPause();
        });

    (void)QObject::connect(ui_.naviBar, &NavBarListView::clickedTable, [this](auto table_id) {
        setCurrentTab(table_id);
        });

    playback_queue_page_.reset(new PlaybackQueueViewPage(this));

    (void)QObject::connect(playback_queue_page_.get(),
        &PlaybackQueueViewPage::playFile,
        this, 
        &Xamp::playLocalFile);

    (void)QObject::connect(ui_.repeatButton, &QToolButton::clicked, [this](auto table_id) {
        playback_queue_page_->show();
        moveToTopWidget(playback_queue_page_.get(), ui_.repeatButton);
        });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::leftButtonValueChanged, [this](auto value) {
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
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
        dialog->setTitle(tr("Equalizer"));
        dialog->setContentWidget(eq.get(), false);
        dialog->setFixedSize(dialog->size());
        dialog->exec();
        });

	setCurrentTab(TAB_LYRICS);

    tray_icon_.reset(new QSystemTrayIcon(this));
    tray_icon_->setIcon(qTheme.applicationIcon());
    tray_icon_->setToolTip(kApplicationTitle);
    auto* tray_icon_menu = new QMenu(this);

    preference_action_ = tray_icon_menu->addAction(qTheme.fontIcon(Glyphs::ICON_SETTINGS), tr("Preference"));
    (void)QObject::connect(preference_action_, &QAction::triggered, [this]() {
        const QScopedPointer<XDialog> dialog(new XDialog(this));
        const QScopedPointer<PreferencePage> preference_page(new PreferencePage(dialog.get()));
        preference_page->loadSettings();
        dialog->setContentWidget(preference_page.get());
        dialog->setFixedSize(dialog->size());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
        dialog->setTitle(tr("Preference"));
        dialog->exec();
        preference_page->saveAll();
        });

    auto* check_for_update_action = new QAction(tr("Check for update"));
    (void)QObject::connect(check_for_update_action, &QAction::triggered, this, &Xamp::onCheckForUpdate);
    tray_icon_menu->addAction(check_for_update_action);

    auto* about_action = new QAction(tr("About"));
    (void)QObject::connect(about_action, &QAction::triggered, this, &Xamp::showAbout);
    tray_icon_menu->addAction(about_action);
    tray_icon_menu->addSeparator();

    auto* log_action = new QAction(tr("Log Viewer"));
    (void)QObject::connect(log_action, &QAction::triggered, [this]() {
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
        });
    tray_icon_menu->addAction(log_action);

    auto* quit_action = new QAction(tr("Quit"));
    (void)QObject::connect(quit_action, &QAction::triggered, [this]() {
        tray_icon_->hide();
        qApp->quit();
        });
    tray_icon_menu->addAction(quit_action);

    tray_icon_->setContextMenu(tray_icon_menu);
    (void)QObject::connect(tray_icon_.get(), &QSystemTrayIcon::activated, this, &Xamp::onActivated);
    tray_icon_->show();
}

void Xamp::onCheckForUpdate() {

}

void Xamp::showAbout() {

}

void Xamp::onActivated(QSystemTrayIcon::ActivationReason reason) {

}

void Xamp::setSeekPosValue(double stream_time) {
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
    auto file_path = playback_queue_page_->popQueue();
    if (file_path.isEmpty()) {
        return;
    }
    playLocalFile(file_path, false);
}

void Xamp::stopPlay() {
    player_->Stop();
    qTheme.setPlayOrPauseButton(ui_.playButton, false);
	ui_.seekSlider->setValue(0);
	ui_.startPosLabel->setText(formatDuration(0));
    ui_.endPosLabel->setText(formatDuration(0));
}
