#include <QDebug>
#include <QToolTip>
#include <QMenu>
#include <QCloseEvent>
#include <QInputDialog>
#include <QShortcut>
#include <QProgressDialog>
#include <QWidgetAction>
#include <QMessageBox>
#include <QFileDialog>

#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <stream/compressor.h>
#include <stream/podcastcache.h>

#include <output_device/audiodevicemanager.h>

#include <player/soxresampler.h>
#include <player/audio_util.h>

#include <widget/albumview.h>
#include <widget/lyricsshowwidget.h>
#include <widget/playlisttableview.h>
#include <widget/albumartistpage.h>
#include <widget/lrcpage.h>
#include <widget/artistview.h>
#include <widget/str_utilts.h>
#include <widget/playlistpage.h>
#include <widget/toast.h>
#include <widget/image_utiltis.h>
#include <widget/time_utilts.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/selectcolorwidget.h>
#include <widget/artistinfopage.h>
#include <widget/jsonsettings.h>
#include <widget/playbackhistorypage.h>
#include <widget/ui_utilts.h>
#include <widget/compressordialog.h>
#include <widget/spectrograph.h>
#include <widget/read_helper.h>

#include "aboutdialog.h"
#include "preferencedialog.h"
#include "thememanager.h"
#include "xamp.h"

static AlignPtr<SampleRateConverter> makeSampleRateConverter(const QVariantMap &settings) {
    const auto quality = static_cast<SoxrQuality>(settings[kSoxrQuality].toInt());
    const auto phase = settings[kSoxrPhase].toInt();
    const auto pass_band = settings[kSoxrPassBand].toInt();
    const auto enable_steep_filter = settings[kSoxrEnableSteepFilter].toBool();

    auto converter = MakeAlign<SampleRateConverter, SoxrSampleRateConverter>();
    auto *soxr_sample_rate_converter = dynamic_cast<SoxrSampleRateConverter*>(converter.get());
    soxr_sample_rate_converter->SetQuality(quality);
    soxr_sample_rate_converter->SetPhase(phase);
    soxr_sample_rate_converter->SetPassBand(pass_band);
    soxr_sample_rate_converter->SetSteepFilter(enable_steep_filter);
    return converter;
}

static AlignPtr<AudioProcessor> makeCompressor(uint32_t sample_rate) {
    auto processor = MakeAlign<AudioProcessor, Compressor>();
    auto* compressor = dynamic_cast<Compressor*>(processor.get());
    compressor->SetSampleRate(sample_rate);
    /*CompressorParameters parameters;
    parameters.gain = AppSettings::getAsInt(kCompressorGain);
    parameters.threshold = AppSettings::getAsInt(kCompressorThreshold);
    parameters.ratio = AppSettings::getAsInt(kCompressorRatio);
    parameters.attack = AppSettings::getAsInt(kCompressorAttack);
    parameters.release = AppSettings::getAsInt(kCompressorRelease);
    compressor->Init(parameters);*/
    compressor->Init();
    return processor;
}

static PlaybackFormat getPlaybackFormat(const AudioPlayer* player) {
    PlaybackFormat format;

    if (player->IsDSDFile()) {
        format.dsd_mode = player->GetDsdModes();
        format.dsd_speed = *player->GetDSDSpeed();
        format.is_dsd_file = true;
    }

    format.enable_sample_rate_convert = player->IsEnableSampleRateConverter();
    format.file_format = player->GetInputFormat();
    format.output_format = player->GetOutputFormat();
    return format;
}

QString Xamp::translasteError(Errors error) {
    static const QMap<Errors, QString> error_str_lut = {
        { Errors::XAMP_ERROR_SUCCESS, tr("Success.") },
        { Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR, tr("Platform spec error.") },
        { Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR, tr("Library spec error.") },
        { Errors::XAMP_ERROR_DEVICE_NOT_INITIALIZED, tr("Device not initialized.") },
        { Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT, tr("Device unsupported format.") },
        { Errors::XAMP_ERROR_DEVICE_IN_USE, tr("Device in use.") },
        { Errors::XAMP_ERROR_DEVICE_NOT_FOUND, tr("Device not found.") },
        { Errors::XAMP_ERROR_FILE_NOT_FOUND, tr("File not found.") },
        { Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE, tr("Not support samplerate.") },
        { Errors::XAMP_ERROR_NOT_SUPPORT_FORMAT, tr("Not support format.") },
        { Errors::XAMP_ERROR_LOAD_DLL_FAILURE, tr("Load dll failure.") },
        { Errors::XAMP_ERROR_STOP_STREAM_TIMEOUT, tr("Stop stream thread timeout.") },
        { Errors::XAMP_ERROR_NOT_SUPPORT_RESAMPLE_SAMPLERATE, tr("Resampler not support variable resample.") },
        { Errors::XAMP_ERROR_SAMPLERATE_CHANGED, tr("Samplerate was changed.") },
        { Errors::XAMP_ERROR_NOT_FOUND_DLL_EXPORT_FUNC, tr("Not found dll export function.") },
    };
    if (error_str_lut.contains(error)) {
        return error_str_lut.value(error);
    }
    return Qt::EmptyString;
}

Xamp::Xamp(QWidget *parent)
    : FramelessWindow(parent)
    , is_seeking_(false)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , lrc_page_(nullptr)
    , playlist_page_(nullptr)
	, podcast_page_(nullptr)
	, current_playlist_page_(nullptr)
    , album_artist_page_(nullptr)
    , artist_info_page_(nullptr)
	, tray_icon_menu_(nullptr)
	, tray_icon_(nullptr)
    , playback_history_page_(nullptr)
    , state_adapter_(std::make_shared<UIPlayerStateAdapter>())
#ifdef Q_OS_WIN
    , player_(std::make_shared<AudioPlayer>(state_adapter_))
    , discord_notify_(this) {
#else
    , player_(std::make_shared<AudioPlayer>(state_adapter_)) {
#endif
    initial();
}

void Xamp::initial() {
    player_->Startup();
    AppSettings::startMonitorFile(this);
    PodcastCache.SetTempPath(AppSettings::getValueAsString(kAppSettingPodcastCachePath).toStdWString());
    initialUI();
    initialController();
    initialDeviceList();
    initialPlaylist();
    initialShortcut();
    createTrayIcon();
    setCover(nullptr, playlist_page_);
    setCover(nullptr, podcast_page_);
    setDefaultStyle();    
    timer_.singleShot(1000, [this]() {
        ThemeManager::instance().enableBlur(this, AppSettings::getValueAsBool(kAppSettingEnableBlur), useNativeWindow());
        });
#ifdef Q_OS_WIN
    discord_notify_.discordInit();
#endif
}

void Xamp::onActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    {
        showNormal();
        raise();
        activateWindow();
        break;
    }
    case QSystemTrayIcon::DoubleClick:
    {
        break;
    }
    default:
        break;
    }
}

void Xamp::createTrayIcon() {
    auto* minimize_action = new QAction(tr("Mi&nimize"), this);
    QObject::connect(minimize_action, &QAction::triggered, this, &QWidget::hide);

    auto* maximize_action = new QAction(tr("Ma&ximize"), this);
    QObject::connect(maximize_action, &QAction::triggered, this, &QWidget::showMaximized);

    auto* restore_action = new QAction(tr("&Restore"), this);
    QObject::connect(restore_action, &QAction::triggered, this, &QWidget::showNormal);

    auto* quit_action = new QAction(tr("&Quit"), this);
    QObject::connect(quit_action, &QAction::triggered, this, &QWidget::close);

    auto* about_action = new QAction(tr("&About"), this);
    (void)QObject::connect(about_action, &QAction::triggered, [=]() {
        AboutDialog aboutdlg;
        aboutdlg.setFont(font());
        aboutdlg.exec();       
        });

    tray_icon_menu_ = new QMenu(this);
    tray_icon_menu_->addAction(minimize_action);
    tray_icon_menu_->addAction(maximize_action);
    tray_icon_menu_->addAction(restore_action);
    tray_icon_menu_->addAction(about_action);
    tray_icon_menu_->addSeparator();
    tray_icon_menu_->addAction(quit_action);

    tray_icon_ = new QSystemTrayIcon(ThemeManager::instance().appIcon(), this);
    tray_icon_->setContextMenu(tray_icon_menu_);
    tray_icon_->setToolTip(Q_UTF8("XAMP"));
    tray_icon_->show();

    QObject::connect(tray_icon_,
        SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this,
        SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
}

void Xamp::closeEvent(QCloseEvent* event) {    
    if (tray_icon_->isVisible() && !isHidden()) {
	    const auto minimize_to_tray_ask = AppSettings::getValueAsBool(kAppSettingMinimizeToTrayAsk);
        QMessageBox::StandardButton reply = QMessageBox::No;

	    const auto is_min_system_tray = AppSettings::getValueAsBool(kAppSettingMinimizeToTray);

        if (!is_min_system_tray && minimize_to_tray_ask) {
            auto [show_again_res, reply_res] = showDontShowAgainDialog(this, minimize_to_tray_ask);
            AppSettings::setValue(kAppSettingMinimizeToTrayAsk, show_again_res);
            AppSettings::setValue(kAppSettingMinimizeToTray, reply == QMessageBox::Ok);
            reply = reply_res;
        }

        if (reply == QMessageBox::Ok) {
            hide();
            event->ignore();
            return;
        }        
    }

    try {
        AppSettings::setValue(kAppSettingVolume, player_->GetVolume());
    } catch (...) {
    }

    AppSettings::setValue(kAppSettingWidth, size().width());
    AppSettings::setValue(kAppSettingHeight, size().height());
    AppSettings::setValue(kAppSettingVolume, ui_.volumeSlider->value());

    if (player_ != nullptr) {
        player_->Destroy();
        player_.reset();
    }

    AppSettings::shutdownMonitorFile();
}

void Xamp::setDefaultStyle() {
    ThemeManager::instance().setDefaultStyle(ui_);
    setStyleSheet(Q_UTF8(R"(
	QListView#sliderBar::item {
		border: 0px;
		padding-left: 15px;
	}
	QListVieww#sliderBar::text {
		left: 15px;
	}
	)"));

    applyTheme(ThemeManager::instance().getBackgroundColor());
}

void Xamp::registerMetaType() {
    qRegisterMetaType<std::vector<Metadata>>("std::vector<Metadata>");
    qRegisterMetaType<DeviceState>("DeviceState");
    qRegisterMetaType<PlayerState>("PlayerState");
    qRegisterMetaType<PlayListEntity>("PlayListEntity");
    qRegisterMetaType<Errors>("Errors");
    qRegisterMetaType<std::vector<float>>("std::vector<float>");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<int32_t>("int32_t");
}

void Xamp::initialUI() {
    registerMetaType();    
    ui_.setupUi(this);
    auto f = font();
    f.setPointSize(10);
    ui_.titleLabel->setFont(f);
    f.setPointSize(8);
    ui_.artistLabel->setFont(f);
    if (useNativeWindow()) {
        ui_.closeButton->hide();
        ui_.maxWinButton->hide();
        ui_.minWinButton->hide();
    }
#ifdef Q_OS_WIN
    f.setPointSize(7);
    ui_.startPosLabel->setFont(f);
    ui_.endPosLabel->setFont(f);
#else
    f.setPointSize(11);
    ui_.titleLabel->setFont(f);
    f.setPointSize(10);
    ui_.artistLabel->setFont(f);
#endif
}

QWidgetAction* Xamp::createTextSeparator(const QString& text) {
	auto* label = new QLabel(text);
    label->setObjectName(Q_UTF8("textSeparator"));
    auto f = font();
    f.setBold(true);
    label->setFont(f);
    auto* separator = new QWidgetAction(this);
    separator->setDefaultWidget(label);
    return separator;
}

void Xamp::onVolumeChanged(float volume) {
    if (volume > 0) {
        player_->SetMute(false);
        ui_.mutedButton->setIcon(ThemeManager::instance().volumeUp());
    }
    else {
        player_->SetMute(true);
        ui_.mutedButton->setIcon(ThemeManager::instance().volumeOff());
    }
    ui_.volumeSlider->setValue(static_cast<int32_t>(volume * 100.0));
}

void Xamp::onDeviceStateChanged(DeviceState state) {
    if (state == DeviceState::DEVICE_STATE_REMOVED) {
        player_->Stop(true, true, true);
    }
    if (state == DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE) {
        return;
    }
    initialDeviceList();
}

void Xamp::initialDeviceList() {    
    auto* menu = ui_.selectDeviceButton->menu();
    if (!menu) {
        menu = new QMenu(this);
        ui_.selectDeviceButton->setMenu(menu);
    }

    ThemeManager::instance().setBackgroundColor(menu);
    menu->clear();

    DeviceInfo init_device_info;
    auto is_find_setting_device = false;

    auto* device_action_group = new QActionGroup(this);
    device_action_group->setExclusive(true);

    std::map<std::string, QAction*> device_id_action;

    const auto device_type_id = AppSettings::getID(kAppSettingDeviceType);
    const auto device_id = AppSettings::getValueAsString(kAppSettingDeviceId).toStdString();

    player_->GetAudioDeviceManager().ForEach([&](const auto &device_type) {
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            return;
        }        

        menu->addAction(createTextSeparator(fromStdStringView(device_type->GetDescription())));

        for (const auto& device_info : device_info_list) {
            auto* device_action = new QAction(QString::fromStdWString(device_info.name), this);
            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_id_action[device_info.device_id] = device_action;

            auto trigger_callback = [device_info, this]() {
                device_info_ = device_info;
                AppSettings::setValue(kAppSettingDeviceType, device_info_.device_type_id);
                AppSettings::setValue(kAppSettingDeviceId, device_info_.device_id);                
            };

            (void)QObject::connect(device_action, &QAction::triggered, trigger_callback);
            menu->addAction(device_action);

            if (device_type_id == device_info.device_type_id && device_id == device_info.device_id) {
                device_info_ = device_info;
                is_find_setting_device = true;
                device_action->setChecked(true);
            }
        }

        if (!is_find_setting_device) {
            auto itr = std::find_if(device_info_list.begin(), device_info_list.end(), [](const auto& info) {
                return info.is_default_device && !AudioDeviceManager::IsExclusiveDevice(info);
            });
            if (itr != device_info_list.end()) {
                init_device_info = (*itr);
            }
        }
    });

    if (!is_find_setting_device) {
        device_info_ = init_device_info;
        device_id_action[device_info_.device_id]->setChecked(true);
        AppSettings::setValue(kAppSettingDeviceType, device_info_.device_type_id);
        AppSettings::setValue(kAppSettingDeviceId, device_info_.device_id);
        XAMP_LOG_DEBUG("Use default device Id : {}", device_info_.device_id);
    }
}

void Xamp::initialController() {
    (void)QObject::connect(ui_.minWinButton, &QToolButton::pressed, [this]() {
        showMinimized();
    });

    (void)QObject::connect(ui_.maxWinButton, &QToolButton::pressed, [this]() {
        if (isMaximized()) {
            showNormal();
        }
        else {
            showMaximized();
        }
    });

    (void)QObject::connect(ui_.mutedButton, &QToolButton::pressed, [this]() {
        if (!ui_.volumeSlider->isEnabled()) {
            return;
        }
        if (player_->IsMute()) {
            player_->SetMute(false);            
            ui_.mutedButton->setIcon(ThemeManager::instance().volumeUp());
        } else {
            player_->SetMute(true);
            ui_.mutedButton->setIcon(ThemeManager::instance().volumeOff());
        }
    });

    (void)QObject::connect(ui_.closeButton, &QToolButton::pressed, [this]() {
        QWidget::close();
    });

    (void)QObject::connect(ui_.sampleConverterButton, &QToolButton::pressed, [this]() {
	    const auto enable_or_disable = !AppSettings::getValueAsBool(kAppSettingResamplerEnable);
        AppSettings::setValue(kAppSettingResamplerEnable, enable_or_disable);
        setButtonState();
        });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::leftButtonValueChanged, [this](auto value) {
        try {
            player_->Seek(static_cast<double>(value / 1000.0));
            ThemeManager::instance().setPlayOrPauseButton(ui_, true);
            setTaskbarPlayingResume();
        }
        catch (const Exception & e) {
            player_->Stop(false);
            Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
            return;
        }
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::sliderMoved, [this](auto value) {
        QToolTip::showText(QCursor::pos(), Time::msToString(static_cast<double>(ui_.seekSlider->value()) / 1000.0));
        if (!is_seeking_) {
            return;
        }
        ui_.seekSlider->setValue(value);
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::sliderReleased, [this]() {
        XAMP_LOG_DEBUG("SeekSlider release!");
        QToolTip::showText(QCursor::pos(), Time::msToString(static_cast<double>(ui_.seekSlider->value()) / 1000.0));
        if (!is_seeking_) {
            return;
        }
        try {
            player_->Seek(static_cast<double>(ui_.seekSlider->value() / 1000.0));
        }
        catch (const Exception& e) {
            player_->Stop(false);
            Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
            return;
        }
        is_seeking_ = false;
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::sliderPressed, [this]() {
        QToolTip::showText(QCursor::pos(), Time::msToString(static_cast<double>(ui_.seekSlider->value()) / 1000.0));
        if (is_seeking_) {
            return;
        }
        is_seeking_ = true;
        player_->Pause();
    });

    order_ = static_cast<PlayerOrder>(AppSettings::getAsInt(kAppSettingOrder));
    setPlayerOrder();

    ui_.volumeSlider->setRange(0, 100);
    const auto vol = AppSettings::getValue(kAppSettingVolume).toUInt();
	if (vol <= 0) {
        setVolume(0);
	}
    ui_.volumeSlider->setValue(static_cast<int32_t>(vol));
    player_->SetMute(vol == 0);

    (void)QObject::connect(ui_.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
        setVolume(volume);
    });

    (void)QObject::connect(ui_.volumeSlider, &SeekSlider::leftButtonValueChanged, [this](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
        setVolume(volume);
    });

    (void)QObject::connect(ui_.volumeSlider, &QSlider::sliderMoved, [](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
    });

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

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::volumeChanged,
        this,
        &Xamp::onVolumeChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(ui_.searchLineEdit, &QLineEdit::textChanged, [this](const auto &text) {
        if (ui_.currentView->count() > 0) {
            auto* playlist_view = currentPlyalistPage()->playlist();
            emit playlist_view->search(text, Qt::CaseSensitivity::CaseInsensitive, QRegExp::PatternSyntax());
            emit album_artist_page_->album()->onSearchTextChanged(text);
        }
    });

    (void)QObject::connect(ui_.nextButton, &QToolButton::pressed, [this]() {
        playNextClicked();
    });

    (void)QObject::connect(ui_.prevButton, &QToolButton::pressed, [this]() {
        playPreviousClicked();
    });

    (void)QObject::connect(ui_.repeatButton, &QToolButton::pressed, [this]() {
        order_ = GetNextOrder(order_);
        setPlayerOrder();
    });

    (void)QObject::connect(ui_.playButton, &QToolButton::pressed, [this]() {
        play();
    });

    (void)QObject::connect(ui_.stopButton, &QToolButton::pressed, [this]() {
        stopPlayedClicked();
    });

    (void)QObject::connect(ui_.backPageButton, &QToolButton::pressed, [this]() {
        goBackPage();
        album_artist_page_->refreshOnece();
    });

    (void)QObject::connect(ui_.nextPageButton, &QToolButton::pressed, [this]() {
        getNextPage();
        album_artist_page_->refreshOnece();
        emit album_artist_page_->album()->onSearchTextChanged(ui_.searchLineEdit->text());
    });

    (void)QObject::connect(ui_.addPlaylistButton, &QToolButton::pressed, [this]() {
	    const auto pos = mapFromGlobal(QCursor::pos());
        playback_history_page_->move(QPoint(pos.x() - 300, pos.y() - 410));
        playback_history_page_->setMinimumSize(QSize(550, 400));
        playback_history_page_->refreshOnece();
        playback_history_page_->show();
    });

    (void)QObject::connect(ui_.artistLabel, &ClickableLabel::clicked, [this]() {
        onArtistIdChanged(current_entity_.artist, current_entity_.cover_id, current_entity_.artist_id);
    });

    (void)QObject::connect(ui_.coverLabel, &ClickableLabel::clicked, [this]() {
        ui_.currentView->setCurrentWidget(lrc_page_);
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::clickedTable, [this](auto table_id) {
    	switch (table_id) {
        case 0:
            ui_.currentView->setCurrentWidget(album_artist_page_);
            break;
        case 1:
            ui_.currentView->setCurrentWidget(artist_info_page_);
            break;
        case 3:
            ui_.currentView->setCurrentWidget(podcast_page_);
            current_playlist_page_ = podcast_page_;
            break;
        case 2:
            ui_.currentView->setCurrentWidget(playlist_page_);
            current_playlist_page_ = playlist_page_;
            break;
        case 4:
            ui_.currentView->setCurrentWidget(lrc_page_);
            break;
    	}        
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::tableNameChanged, [](auto table_id, const auto &name) {
        Singleton<Database>::GetInstance().setTableName(table_id, name);
    });

#ifdef Q_OS_WIN
    (void)QObject::connect(this,
        &Xamp::nowPlaying,
        &discord_notify_,
        &DicordNotify::OnNowPlaying);

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::stateChanged,
        &discord_notify_,
        &DicordNotify::OnStateChanged,
        Qt::QueuedConnection);
#endif

    auto* settings_menu = new QMenu(this);
    ThemeManager::instance().setBackgroundColor(settings_menu);

    auto* settings_action = new QAction(tr("Settings"), this);
    settings_menu->addAction(settings_action);
    (void)QObject::connect(settings_action, &QAction::triggered, [=]() {
        PreferenceDialog dialog;
        dialog.setFont(font());
        dialog.exec();
        setButtonState();
    });
	// Theme color
    auto* select_color_widget = new SelectColorWidget();
    auto* theme_color_menu = new QMenu(tr("Theme color"));
    auto* widget_action = new QWidgetAction(theme_color_menu);
    widget_action->setDefaultWidget(select_color_widget);
    (void)QObject::connect(select_color_widget, &SelectColorWidget::colorButtonClicked, [this](auto color) {
        applyTheme(color);
    });
    theme_color_menu->addAction(widget_action);
    settings_menu->addMenu(theme_color_menu);
    // Hide left list
    auto* hide_left_list_action = new QAction(tr("Show left list"), this);
    hide_left_list_action->setCheckable(true);
	if (AppSettings::getValue(kAppSettingShowLeftList).toBool()) {
        hide_left_list_action->setChecked(true);
        ui_.sliderFrame->setVisible(true);
	} else {
        ui_.sliderFrame->setVisible(false);
	}
    (void)QObject::connect(hide_left_list_action, &QAction::triggered, [=]() {
        auto enable = AppSettings::getValueAsBool(kAppSettingShowLeftList);
        enable = !enable;
        hide_left_list_action->setChecked(enable);
        ui_.sliderFrame->setVisible(enable);
        AppSettings::setValue(kAppSettingShowLeftList, enable);
        });
    settings_menu->addAction(hide_left_list_action);
#ifdef Q_OS_WIN
    auto* enable_blur_material_mode_action = new QAction(tr("Enable blur"), this);
    enable_blur_material_mode_action->setCheckable(true);
    if (AppSettings::getValue(kAppSettingEnableBlur).toBool()) {
        enable_blur_material_mode_action->setChecked(true);
    }

    (void)QObject::connect(enable_blur_material_mode_action, &QAction::triggered, [=]() {
        auto enable = AppSettings::getValueAsBool(kAppSettingEnableBlur);
        enable = !enable;
        enable_blur_material_mode_action->setChecked(enable);
        ThemeManager::instance().enableBlur(this, enable, useNativeWindow());
        });
    settings_menu->addAction(enable_blur_material_mode_action);
#endif
    settings_menu->addSeparator();
    auto* about_action = new QAction(tr("About"), this);
    settings_menu->addAction(about_action);
    (void)QObject::connect(about_action, &QAction::triggered, [=]() {
        AboutDialog aboutdlg;
        aboutdlg.setFont(font());
        aboutdlg.exec();       
    });
    ui_.settingsButton->setMenu(settings_menu);

    ui_.seekSlider->setEnabled(false);
    ui_.startPosLabel->setText(Time::msToString(0));
    ui_.endPosLabel->setText(Time::msToString(0));
    ui_.searchLineEdit->setPlaceholderText(tr("Search anything"));    
}

void Xamp::setButtonState() {
    if (AppSettings::getValueAsBool(kAppSettingResamplerEnable)) {
        ui_.sampleConverterButton->setStyleSheet(Q_UTF8("QToolButton#sampleConverterButton { border: none; font-weight: bold; color: rgb(255, 255, 255); }"));
    }
    else {
        ui_.sampleConverterButton->setStyleSheet(Q_UTF8("QToolButton#sampleConverterButton { border: none; font-weight: bold; color: gray; }"));
    }
}

void Xamp::applyTheme(QColor color) {
    if (AppSettings::getValueAsBool(kAppSettingEnableBlur)) {
        color.setAlpha(90);
    } else {
        color.setAlpha(100);
    }
	
    if (qGray(color.rgb()) > 200) {      
        emit themeChanged(color, Qt::black);
        ThemeManager::instance().setThemeColor(ThemeColor::WHITE_THEME);
    }
    else {
        emit themeChanged(color, Qt::white);
        ThemeManager::instance().setThemeColor(ThemeColor::DARK_THEME);
    }

    if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
        ThemeManager::instance().setPlayOrPauseButton(ui_, false);
    }
    else {
        ThemeManager::instance().setPlayOrPauseButton(ui_, true);
    }

    ThemeManager::instance().setBackgroundColor(ui_, color);

    setStyleSheet(Q_STR(R"(background-color: %1;)").arg(colorToString(color)));

    setButtonState();
}

void Xamp::getNextPage() {
    if (stack_page_id_.isEmpty()) {
        return;
    }
    auto idx = ui_.currentView->currentIndex();
    ui_.currentView->setCurrentIndex(idx + 1);
}

void Xamp::setTablePlaylistView(int table_id) {
	const auto playlist_id = Singleton<Database>::GetInstance().findTablePlaylistId(table_id);

    auto found = false;
    for (auto idx : stack_page_id_) {
        if (auto* page = dynamic_cast<PlyalistPage*>(ui_.currentView->widget(idx))) {
            if (page->playlist()->playlistId() == playlist_id) {
                ui_.currentView->setCurrentIndex(idx);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        auto* playlist_page = newPlaylist(playlist_id);
        playlist_page->playlist()->setPlaylistId(playlist_id);
        pushWidget(playlist_page);
    }
}

void Xamp::goBackPage() {
    if (stack_page_id_.isEmpty()) {
        return;
    }
    const auto idx = ui_.currentView->currentIndex();
    ui_.currentView->setCurrentIndex(idx - 1);
}

void Xamp::setVolume(int32_t volume) {
    if (volume > 0) {
        player_->SetMute(false);
        ui_.mutedButton->setIcon(ThemeManager::instance().volumeUp());
    }
    else {
        player_->SetMute(true);
        ui_.mutedButton->setIcon(ThemeManager::instance().volumeOff());
    }

    try {
        player_->SetVolume(static_cast<uint32_t>(volume));
    }
    catch (const Exception& e) {
        player_->Stop(false);
        Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
    }
}

void Xamp::initialShortcut() {
    auto* space_key = new QShortcut(QKeySequence(Qt::Key_Space), this);
    (void)QObject::connect(space_key, &QShortcut::activated, [this]() {
        play();
    });
}

void Xamp::stopPlayedClicked() {
    player_->Stop(false, true);
    setSeekPosValue(0);
    ui_.seekSlider->setEnabled(false);
    playlist_page_->playlist()->removePlaying();
}

void Xamp::playNextClicked() {
    playNextItem(1);
}

void Xamp::playPreviousClicked() {
    playNextItem(-1);
}

void Xamp::deleteKeyPress() {
    if (!ui_.currentView->count()) {
        return;
    }
    auto* playlist_view = playlist_page_->playlist();
    playlist_view->removeSelectItems();
}

void Xamp::setPlayerOrder() {
    switch (order_) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        ThemeManager::instance().setRepeatOncePlayOrder(ui_);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE));
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        ThemeManager::instance().setRepeatOnePlayOrder(ui_);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_REPEAT_ONE));
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        ThemeManager::instance().setShufflePlayorder(ui_);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL));
        break;
    default:
        break;
    }
}

void Xamp::onSampleTimeChanged(double stream_time) {
    if (!player_) {
        return;
    }
    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {         
        setSeekPosValue(stream_time);
    }
}

void Xamp::setSeekPosValue(double stream_time) {
    ui_.endPosLabel->setText(Time::msToString(player_->GetDuration() - stream_time));
    const auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
    ui_.seekSlider->setValue(stream_time_as_ms);
    ui_.startPosLabel->setText(Time::msToString(stream_time));
    setTaskbarProgress(static_cast<int32_t>(100.0 * ui_.seekSlider->value() / ui_.seekSlider->maximum()));
    lrc_page_->lyricsWidget()->setLrcTime(stream_time_as_ms);
}

void Xamp::playLocalFile(const PlayListEntity& item) {
    setTaskbarPlayerPlaying();
    play(item);
}

void Xamp::play() {
    XAMP_LOG_DEBUG("Player state:{}", player_->GetState());

    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
        ThemeManager::instance().setPlayOrPauseButton(ui_, false);
        player_->Pause();
        setTaskbarPlayerPaused();
    }
    else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
        ThemeManager::instance().setPlayOrPauseButton(ui_, true);
        player_->Resume();
        setTaskbarPlayingResume();
    }
    else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED) {
        if (!ui_.currentView->count()) {
            return;
        }
        if (auto select_item = playlist_page_->playlist()->selectItem()) {
            play_index_ = select_item.value();
        }
        play_index_ = playlist_page_->playlist()->model()->index(
            play_index_.row(), PLAYLIST_PLAYING);
        if (play_index_.row() == -1) {
            Toast::showTip(tr("Not found any playing item."), this);
            return;
        }
        playlist_page_->playlist()->setNowPlaying(play_index_, true);
        playlist_page_->playlist()->play(play_index_);
    }
}

void Xamp::resetSeekPosValue() {
    ui_.seekSlider->setValue(0);
    ui_.startPosLabel->setText(Time::msToString(0));
}

void Xamp::processMeatadata(const std::vector<Metadata>& medata) const {    
    MetadataExtractAdapter::processMetadata(medata);
    album_artist_page_->album()->refreshOnece();
    album_artist_page_->artist()->refreshOnece();
}

void Xamp::playMusic(const MusicEntity& item) {
    auto open_done = false;

    ui_.seekSlider->setEnabled(true);

    XAMP_ON_SCOPE_EXIT(
        if (open_done) {
            return;
        }
        resetSeekPosValue();
        ui_.seekSlider->setEnabled(false);
        player_->Stop(false, true);
        );

    PlaybackFormat playback_format;
    AlignPtr<SampleRateConverter> converter;
    uint32_t target_sample_rate = 0;	
	
    try {
        if (AppSettings::getValue(kAppSettingResamplerEnable).toBool()) {
            auto soxr_settings = JsonSettings::getValue(AppSettings::getValueAsString(kAppSettingSoxrSettingName)).toMap();
            target_sample_rate = soxr_settings[kSoxrResampleSampleRate].toUInt();
            converter = makeSampleRateConverter(soxr_settings);            
        }
        player_->Open(item.file_path.toStdWString(), device_info_, target_sample_rate, std::move(converter));        
        player_->PrepareToPlay();
        const auto enable_dsp = item.true_peak > 1.0;
        if (enable_dsp) {
            player_->SetProcessor(makeCompressor(player_->GetInputFormat().GetSampleRate()));
        }
        playback_format = getPlaybackFormat(player_.get());
        player_->Play();
        open_done = true;
    }
    catch (const Exception & e) {
        XAMP_LOG_DEBUG("Exception: {} {}", e.GetErrorMessage(), e.GetStackTrace());
        Toast::showTip(translasteError(e.GetError()), this);
    }
    catch (const std::exception & e) {
        Toast::showTip(Q_UTF8(e.what()), this);
    }
    catch (...) {
        Toast::showTip(tr("unknown error"), this);
    }
    updateUI(item, playback_format, open_done);
}

void Xamp::updateUI(const MusicEntity& item, const PlaybackFormat& playback_format, bool open_done) {
    auto* cur_page = currentPlyalistPage();
	
    ThemeManager::instance().setPlayOrPauseButton(ui_, open_done);
	
    if (open_done) {
        if (player_->IsHardwareControlVolume()) {
            if (!player_->IsMute()) {
                if (player_->GetDsdModes() == DsdModes::DSD_MODE_DOP) {
                    setVolume(100);
                } else {
                    setVolume(ui_.volumeSlider->value());
                }
            } else {
                setVolume(0);
            }
            ui_.volumeSlider->setDisabled(false);
        }
        else {
            ui_.volumeSlider->setDisabled(true);
        }

        Singleton<Database>::GetInstance().addPlaybackHistory(item.album_id, item.artist_id, item.music_id);
        playback_history_page_->refreshOnece();

        ui_.seekSlider->setRange(0, static_cast<int32_t>(player_->GetDuration() * 1000));
        ui_.endPosLabel->setText(Time::msToString(player_->GetDuration()));
        cur_page->format()->setText(format2String(playback_format, item.file_ext));

        artist_info_page_->setArtistId(item.artist,
            Singleton<Database>::GetInstance().getArtistCoverId(item.artist_id),
            item.artist_id);

        emit nowPlaying(item.artist, item.title);
    }

    if (current_entity_.cover_id != item.cover_id) {
        if (item.cover_id == Singleton<PixmapCache>::GetInstance().getUnknownCoverId()) {
            setCover(nullptr);
        }
        else {
            if (const auto * cover = Singleton<PixmapCache>::GetInstance().find(item.cover_id)) {
                setCover(cover);
            }
            else {
                setCover(nullptr);
            }
        }        
    }

    ui_.titleLabel->setText(item.title);
    ui_.artistLabel->setText(item.artist);

    cur_page->title()->setText(item.title);

    const QFileInfo file_info(item.file_path);
    const auto lrc_path = file_info.path()
                          + Q_UTF8("/")
                          + file_info.completeBaseName()
                          + Q_UTF8(".lrc");
    if (!lrc_page_->lyricsWidget()->loadLrcFile(lrc_path)) {
        //todo:
		//musixmatcher_.matcherLyrics(item.title, item.artist);
    }
	
    lrc_page_->title()->setText(item.title);
    lrc_page_->album()->setText(item.album);
    lrc_page_->artist()->setText(item.artist);

    if (isHidden()) {
        tray_icon_->showMessage(item.album, 
            item.title, 
            ThemeManager::instance().appIcon(), 
            1000);
    }
}

PlyalistPage* Xamp::currentPlyalistPage() {
    current_playlist_page_ = dynamic_cast<PlyalistPage*>(ui_.currentView->currentWidget());
    if (!current_playlist_page_) {
        current_playlist_page_ = dynamic_cast<PlyalistPage*>(ui_.currentView->widget(1));
    }
    return current_playlist_page_;
}

void Xamp::play(const PlayListEntity& item) {    
    playMusic(toMusicEntity(item));
    current_entity_ = item;
    applyTheme(ThemeManager::instance().getBackgroundColor());
}

void Xamp::playNextItem(int32_t forward) {
    auto* playlist_view = currentPlyalistPage()->playlist();
    const auto count = playlist_view->model()->rowCount();
    if (count == 0) {
        stopPlayedClicked();
        return;
    }

    play_index_ = playlist_view->currentIndex();

    if (count > 1) {
        switch (order_) {
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
            play_index_ = playlist_view->nextIndex(forward);
            if (play_index_.row() == -1) {
                return;
            }
            break;
        case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
            play_index_ = playlist_view->shuffeIndex();
            break;
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        default:
            break;
        }

        if (!play_index_.isValid()) {
            Toast::showTip(tr("Not found any playlist item."), this);
            return;
        }
    } else {
        play_index_ = playlist_view->model()->index(0, 0);
    }

    playlist_view->setNowPlaying(play_index_, true);
    playlist_view->play(play_index_);
    playlist_view->refresh();
}

void Xamp::play(const QModelIndex&, const PlayListEntity& item) {
    playLocalFile(item);
    if (!player_->IsPlaying()) {
        playlist_page_->format()->setText(Q_UTF8(""));
    }
}

void Xamp::onArtistIdChanged(const QString& artist, const QString& /*cover_id*/, int32_t artist_id) {
    artist_info_page_->setArtistId(artist, Singleton<Database>::GetInstance().getArtistCoverId(artist_id), artist_id);
    ui_.currentView->setCurrentWidget(artist_info_page_);
}

void Xamp::addPlaylistItem(const PlayListEntity &entity) {
    auto playlist_view = playlist_page_->playlist();
    Singleton<Database>::GetInstance().addMusicToPlaylist(entity.music_id, playlist_view->playlistId());
    playlist_view->refresh();
}

void Xamp::setCover(const QPixmap* cover, PlyalistPage* page) {
    if (!cover) {
        cover = &ThemeManager::instance().pixmap().unknownCover();    
    }

	if (!page) {
        page = current_playlist_page_;
	}

    const auto ui_cover = Pixmap::roundImage(
        Pixmap::resizeImage(*cover, ui_.coverLabel->size(), false),
           Pixmap::kSmallImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);

    const auto playlist_cover = Pixmap::roundImage(
        Pixmap::resizeImage(*cover, page->cover()->size(), false),
        Pixmap::kSmallImageRadius);
    page->cover()->setPixmap(playlist_cover);
    if (lrc_page_ != nullptr) {
        lrc_page_->setCover(Pixmap::resizeImage(*cover, lrc_page_->cover()->size(), true));
    }   
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
    if (!player_) {
        return;
    }
    if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
        resetTaskbarProgress();
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(Time::msToString(0));
        playNextItem(1);
        payNextMusic();
    }
}

void Xamp::addTable() {
	auto is_ok = false;
    const auto table_name = QInputDialog::getText(nullptr, tr("Input Table Name"),
                                                  tr("Please input your name"),
                                                  QLineEdit::Normal,
                                                  tr("My favorite"),
                                                  &is_ok);
    if (!is_ok) {
        return;
    }
}

void Xamp::initialPlaylist() {
    ui_.sliderBar->addTab(tr("Playlists"), 2, ThemeManager::instance().playlistIcon());
    ui_.sliderBar->addTab(tr("Podcast"), 3, ThemeManager::instance().podcastIcon());
    ui_.sliderBar->addTab(tr("Albums"), 0, ThemeManager::instance().albumsIcon());
	ui_.sliderBar->addTab(tr("Artists"), 1, ThemeManager::instance().artistsIcon());
    ui_.sliderBar->addTab(tr("Lyrics"), 4, ThemeManager::instance().subtitleIcon());
	
    Singleton<Database>::GetInstance().forEachTable([this](auto table_id,
                                             auto /*table_index*/,
                                             auto playlist_id,
                                             const auto &name) {
        if (name.isEmpty()) {
            return;
        }

        ui_.sliderBar->addTab(name, table_id, ThemeManager::instance().playlistIcon());

        if (!playlist_page_) {
            playlist_page_ = newPlaylist(playlist_id);
            playlist_page_->playlist()->setPlaylistId(playlist_id);
        }

        if (playlist_page_->playlist()->playlistId() != playlist_id) {
            playlist_page_ = newPlaylist(playlist_id);
            playlist_page_->playlist()->setPlaylistId(playlist_id);
        }
    });

    constexpr auto kDefaultPlaylistId = 1;
    constexpr auto kDefaultPodcastId = 2;

    if (!playlist_page_) {
        auto playlist_id = kDefaultPlaylistId;
        if (!Singleton<Database>::GetInstance().isPlaylistExist(playlist_id)) {
            playlist_id = Singleton<Database>::GetInstance().addPlaylist(Qt::EmptyString, 0);
        }
        playlist_page_ = newPlaylist(kDefaultPlaylistId);
        playlist_page_->playlist()->setPlaylistId(kDefaultPlaylistId);
    }

    if (!podcast_page_) {
        auto playlist_id = kDefaultPodcastId;
        if (!Singleton<Database>::GetInstance().isPlaylistExist(playlist_id)) {
            playlist_id = Singleton<Database>::GetInstance().addPlaylist(Qt::EmptyString, 1);
        }
        podcast_page_ = newPlaylist(playlist_id);
        podcast_page_->playlist()->setPlaylistId(playlist_id);
    }

    playlist_page_->playlist()->setPodcastMode(false);
    podcast_page_->playlist()->setPodcastMode(true);
    current_playlist_page_ = playlist_page_;

    lrc_page_ = new LrcPage(this);
    album_artist_page_ = new AlbumArtistPage(this);
    playback_history_page_ = new PlaybackHistoryPage(this);
    playback_history_page_->setObjectName(Q_UTF8("playbackHistoryPage"));
    playback_history_page_->setFont(font());
    playback_history_page_->hide();

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        album_artist_page_,
        &AlbumArtistPage::onThemeChanged);

    artist_info_page_ = new ArtistInfoPage(this);
    
    pushWidget(lrc_page_);    
    pushWidget(playlist_page_);
    pushWidget(album_artist_page_);
    pushWidget(artist_info_page_);
    pushWidget(podcast_page_);
    goBackPage();
    goBackPage();
    goBackPage();

    (void)QObject::connect(album_artist_page_->album(),
                            &AlbumView::clickedArtist,
                            this,
                            &Xamp::onArtistIdChanged);

    (void)QObject::connect(this,
                            &Xamp::themeChanged,
                            album_artist_page_->album(),
                            &AlbumView::onThemeChanged);

    (void)QObject::connect(this,
                            &Xamp::themeChanged,
                            artist_info_page_,
                            &ArtistInfoPage::onThemeChanged);

    (void)QObject::connect(this,
                            &Xamp::themeChanged,
                            lrc_page_,
                            &LrcPage::onThemeChanged);

    (void)QObject::connect(&mbc_, &MusicBrainzClient::finished,
                            [this](auto artist_id, auto discogs_artist_id) {
                            Singleton<Database>::GetInstance().updateDiscogsArtistId(artist_id, discogs_artist_id);
                                if (!discogs_artist_id.isEmpty()) {
                                    discogs_.searchArtistId(artist_id, discogs_artist_id);
                                }
                            });

    (void)QObject::connect(&discogs_,
                            &DiscogsClient::getArtistImageUrl,
                            [this](auto artist_id, auto url) {
                                discogs_.downloadArtistImage(artist_id, url);
                                XAMP_LOG_DEBUG("Download artist id: {}, discogs image url: {}", artist_id, url.toStdString());
                            });

    (void)QObject::connect(&discogs_,
                            &DiscogsClient::downloadImageFinished,
                            [](auto artist_id, auto image) {
                                auto cover_id = Singleton<PixmapCache>::GetInstance().addOrUpdate(image);
                                Singleton<Database>::GetInstance().updateArtistCoverId(artist_id, cover_id);
                                XAMP_LOG_DEBUG("Save artist id: {} image, cover id : {}", artist_id, cover_id.toStdString());
                            });

    (void)QObject::connect(album_artist_page_->album(),
                            &AlbumView::addPlaylist,
                            [this](const auto& entity) {
                                addPlaylistItem(entity);
                            });

    (void)QObject::connect(album_artist_page_->album(),
                            &AlbumView::playMusic,
                            [this](const auto& entity) {
                                (void)QObject::disconnect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
                                (void)QObject::connect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
                                playMusic(entity);
                            });
    (void)QObject::connect(playback_history_page_,
                            &PlaybackHistoryPage::playMusic,
                            [this](const auto& entity) {
                                (void)QObject::disconnect(this, &Xamp::payNextMusic, album_artist_page_->album(), &AlbumView::payNextMusic);
                                (void)QObject::connect(this, &Xamp::payNextMusic, playback_history_page_, &PlaybackHistoryPage::playNextMusic);
                                playMusic(entity);
                            });

    setupPlayNextMusicSignals(true);
}

void Xamp::setupPlayNextMusicSignals(bool add_or_remove) {
    if (add_or_remove) {
        (void)QObject::connect(this,
                                &Xamp::payNextMusic,
                                album_artist_page_->album(),
                                &AlbumView::payNextMusic);
        (void)QObject::connect(this,
                                &Xamp::payNextMusic,
                                playback_history_page_,
                                &PlaybackHistoryPage::playNextMusic);
    }
    else {
        (void)QObject::disconnect(this,
                                   &Xamp::payNextMusic,
                                   album_artist_page_->album(),
                                   &AlbumView::payNextMusic);
        (void)QObject::disconnect(this,
                                   &Xamp::payNextMusic,
                                   playback_history_page_,
                                   &PlaybackHistoryPage::playNextMusic);
    }    
}

void Xamp::addItem(const QString& file_name) {
	const auto add_playlist = dynamic_cast<PlyalistPage*>(ui_.currentView->currentWidget()) != nullptr;

    if (add_playlist) {
        try {
            playlist_page_->playlist()->append(file_name);
            album_artist_page_->refreshOnece();
        }
        catch (const Exception & e) {
            Toast::showTip(Q_UTF8(e.GetErrorMessage()), this);
        }
    }
    else {
        extractFile(file_name);
    }
}

void Xamp::pushWidget(QWidget* widget) {
    auto id = ui_.currentView->addWidget(widget);
    stack_page_id_.push(id);
    ui_.currentView->setCurrentIndex(id);
}

QWidget* Xamp::topWidget() {
    if (!stack_page_id_.isEmpty()) {
        return ui_.currentView->widget(stack_page_id_.top());
    }
    return nullptr;
}

QWidget* Xamp::popWidget() {
    if (!stack_page_id_.isEmpty()) {
        auto id = stack_page_id_.pop();
        auto* widget = ui_.currentView->widget(id);
        ui_.currentView->removeWidget(widget);
        if (!stack_page_id_.isEmpty()) {
            ui_.currentView->setCurrentIndex(stack_page_id_.top());
            return widget;
        }
    }
    return nullptr;
}

void Xamp::readFileLUFS(const QModelIndex&, const PlayListEntity& item) {    
    auto dialog = makeProgressDialog(tr("Read progress dialog"),
        tr("Read '") + item.title + tr("' loudness"),
        tr("Cancel"));
    dialog->show();

    try {
        auto [lufs, true_peak] = ReadFileLUFS(item.file_path.toStdWString(),
            item.file_ext.toStdWString(),
            [&](auto progress) -> bool {
                dialog->setValue(progress);
                qApp->processEvents();
                return dialog->wasCanceled() != true;
            });
        Singleton<Database>::GetInstance().updateLUFS(item.music_id, lufs, true_peak);
    }
    catch (Exception const &e) {
        Toast::showTip(Q_UTF8(e.what()), this);        
    }    
}

void Xamp::exportWaveFile(const QModelIndex&, const PlayListEntity& item) {
    const auto file_name = QFileDialog::getSaveFileName(this, tr("Save WAVE file"),
        item.title,
        tr("WAVE Files (*.wav)"));
	
    if (file_name.isNull()) {
        return;
    }

    auto dialog = makeProgressDialog(tr("Export progress dialog"),
        tr("Export '") + item.title + tr("' to wave file"),
        tr("Cancel"));
    dialog->show();

    Metadata metadata;
    metadata.album = item.album.toStdWString();
    metadata.artist = item.artist.toStdWString();
    metadata.title = item.title.toStdWString();
    metadata.track = item.track;

    if (AppSettings::getValue(kAppSettingResamplerEnable).toBool()) {
        const auto soxr_settings = JsonSettings::getValue(AppSettings::getValueAsString(kAppSettingSoxrSettingName)).toMap();
        const auto sample_rate = soxr_settings[kSoxrResampleSampleRate].toUInt();
        auto sample_rate_converter = makeSampleRateConverter(soxr_settings);

        try {
            Export2WaveFile(item.file_path.toStdWString(),
                item.file_ext.toStdWString(),
                file_name.toStdWString(),
                [&](auto progress) -> bool {
                    dialog->setValue(progress);
                    qApp->processEvents();
                    return dialog->wasCanceled() != true;
                }, metadata, sample_rate, sample_rate_converter);
        }
        catch (Exception const& e) {
            Toast::showTip(Q_UTF8(e.what()), this);
        }
    }  else {
        try {
            Export2WaveFile(item.file_path.toStdWString(),
                item.file_ext.toStdWString(),
                file_name.toStdWString(),
                [&](auto progress) -> bool {
                    dialog->setValue(progress);
                    qApp->processEvents();
                    return dialog->wasCanceled() != true;
                }, metadata);
        }
        catch (Exception const& e) {
            Toast::showTip(Q_UTF8(e.what()), this);
        }
    }
}

void Xamp::readFingerprint(const QModelIndex&, const PlayListEntity& item) {
    auto dialog = makeProgressDialog(
        tr("Read progress dialog"),
        tr("Read '") + item.title + tr("' fingerprint"),
        tr("Cancel"));
    
    dialog->show();

    FingerprintInfo fingerprint_info;
    QByteArray fingerprint;

    try {
        const auto [duration, fingerprint_result] =
            ReadFingerprint(item.file_path.toStdWString(),
                            item.file_ext.toStdWString(),
                            [&](auto progress) -> bool {
                                dialog->setValue(progress);
                                qApp->processEvents();
                                return dialog->wasCanceled() != true;
                            });

        fingerprint.append(reinterpret_cast<char const *>(fingerprint_result.data()),
                           static_cast<int32_t>(fingerprint_result.size()));
        fingerprint_info.duration = static_cast<int32_t>(duration);
    } catch (Exception const& e) {
        Toast::showTip(Q_UTF8(e.what()), this);
        return;
    }

    fingerprint_info.music_id = item.music_id;
    fingerprint_info.artist_id = item.artist_id;
    fingerprint_info.fingerprint = QString::fromLatin1(fingerprint);
    XAMP_LOG_DEBUG("music id:{} fingerprint:{}", item.music_id, fingerprint_info.fingerprint.toUtf8().toStdString());
    mbc_.searchBy(fingerprint_info);
}

PlyalistPage* Xamp::newPlaylist(int32_t playlist_id) {
	auto* playlist_page = new PlyalistPage(this);

    ui_.currentView->addWidget(playlist_page);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::playMusic,
                            [this](auto index, const auto& item) {
                                setupPlayNextMusicSignals(false);
                                play(index, item);
                            });

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        playlist_page,
        &PlyalistPage::OnThemeColorChanged);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::removeItems,
                            [](auto playlist_id, const auto& select_music_ids) {
                                IgnoreSqlError(Singleton<Database>::GetInstance().removePlaylistMusic(playlist_id, select_music_ids))
                            });

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::readFingerprint,
                            this, &Xamp::readFingerprint);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::readFileLUFS,
        this, &Xamp::readFileLUFS);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::exportWaveFile,
        this, &Xamp::exportWaveFile);

    (void)QObject::connect(this, &Xamp::themeChanged,
                            playlist_page->playlist(),
                            &PlayListTableView::onThemeColorChanged);

    playlist_page->playlist()->setPlaylistId(playlist_id);    

    return playlist_page;
}

void Xamp::addDropFileItem(const QUrl& url) {
    addItem(url.toLocalFile());
}

void Xamp::onFileChanged(const QString& file_path) {    
	if (!QFile::exists(file_path)) {
        XAMP_LOG_DEBUG("File is removed: {}", file_path.toStdString());
        Singleton<Database>::GetInstance().removeMusic(file_path);
	}

    QMessageBox msgbox;
    msgbox.setWindowTitle(Q_UTF8("XAMP"));
    msgbox.setText(tr("File has been modified outside of source editor. Do you want to reload it?"));
    msgbox.setIcon(QMessageBox::Icon::Question);
    msgbox.addButton(QMessageBox::Ok);
    msgbox.addButton(QMessageBox::Cancel);
    msgbox.setDefaultButton(QMessageBox::Cancel);

    const auto reply = static_cast<QMessageBox::StandardButton>(msgbox.exec());
    if (reply != QMessageBox::Ok) {
        return;
    }
	
    extractFile(AppSettings::getMyMusicFolderPath());
}

void Xamp::extractFile(const QString& file_path) {
    const auto adapter = QSharedPointer<MetadataExtractAdapter>(new MetadataExtractAdapter());
    (void)QObject::connect(adapter.get(),
        &MetadataExtractAdapter::readCompleted,
        this,
        &Xamp::processMeatadata);
    MetadataExtractAdapter::readFileMetadata(adapter, file_path);
};
