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
#include <QNetworkProxyFactory>

#include <base/scopeguard.h>
#include <base/str_utilts.h>

#include <stream/basscompressor.h>
#include <stream/podcastcache.h>
#include <stream/api.h>

#include <output_device/api.h>

#include <player/soxresampler.h>
#include <player/api.h>

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
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/ytmusicwebengineview.h>
#include <widget/artistinfopage.h>
#include <widget/jsonsettings.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/time_utilts.h>
#include <widget/equalizerdialog.h>

#include "aboutpage.h"
#include "preferencepage.h"
#include "thememanager.h"
#include "xamp.h"

enum TabIndex {
    TAB_ALBUM = 0,
    TAB_ARTIST,
    TAB_PLAYLIST,
    TAB_PODCAST,
    TAB_LYRICS,
    TAB_SETTINGS,
    TAB_ABOUT,
    TAB_YT_MUSIC,
};

static AlignPtr<ISampleRateConverter> makeSampleRateConverter(const QVariantMap &settings) {
    const auto quality = static_cast<SoxrQuality>(settings[kSoxrQuality].toInt());
    const auto stop_band = settings[kSoxrStopBand].toInt();
    const auto pass_band = settings[kSoxrPassBand].toInt();
    const auto phase = settings[kSoxrPhase].toInt();
    const auto enable_steep_filter = settings[kSoxrEnableSteepFilter].toBool();
    const auto rolloff_level = static_cast<SoxrRollOff>(settings[kSoxrRollOffLevel].toInt());

    auto converter = MakeAlign<ISampleRateConverter, SoxrSampleRateConverter>();
    auto *soxr_sample_rate_converter = dynamic_cast<SoxrSampleRateConverter*>(converter.get());
    soxr_sample_rate_converter->SetQuality(quality);
    soxr_sample_rate_converter->SetStopBand(stop_band);
    soxr_sample_rate_converter->SetPassBand(pass_band);
    soxr_sample_rate_converter->SetPhase(phase);
    soxr_sample_rate_converter->SetRollOff(rolloff_level);
    soxr_sample_rate_converter->SetSteepFilter(enable_steep_filter);
    return converter;
}

static PlaybackFormat getPlaybackFormat(const IAudioPlayer* player) {
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
        { Errors::XAMP_ERROR_NOT_SUPPORT_EXCLUSIVE_MODE, tr("Not support exclusive mode.") },
    };
    if (error_str_lut.contains(error)) {
        return error_str_lut.value(error);
    }
    return Qt::EmptyString;
}

Xamp::Xamp()
    : is_seeking_(false)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , lrc_page_(nullptr)
    , playlist_page_(nullptr)
	, podcast_page_(nullptr)
	, current_playlist_page_(nullptr)
    , album_artist_page_(nullptr)
    , artist_info_page_(nullptr)
	, tray_icon_menu_(nullptr)
	, tray_icon_(nullptr)
	, ytmusic_view_(nullptr)
    , state_adapter_(std::make_shared<UIPlayerStateAdapter>())
#ifdef Q_OS_WIN
    , player_(MakeAudioPlayer(state_adapter_))
    , discord_notify_(this) {
#else
    , player_(MakeAudioPlayer(state_adapter_)) {
#endif
    ui_.setupUi(this);
}

void Xamp::initial(ITopWindow *top_window) {
    top_window_ = top_window;
    player_->Startup();
    PodcastCache.SetTempPath(AppSettings::getValueAsString(kAppSettingPodcastCachePath).toStdWString());
    initialUI();
    initialController();
    initialPlaylist();
    initialShortcut();
    createTrayIcon();
    setCover(nullptr, playlist_page_);
    setCover(nullptr, podcast_page_);
    setDefaultStyle();
    const auto enable_blur = AppSettings::getValueAsBool(kAppSettingEnableBlur);
    if (enable_blur) {
        QTimer::singleShot(300, [this]() {
            ThemeManager::instance().enableBlur(top_window_, true);
            ThemeManager::instance().setBackgroundColor(ui_.settingsButton->menu(), 255);
            initialDeviceList();
            });
    } else {
        initialDeviceList();
    }
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

    tray_icon_menu_ = new QMenu(this);
    tray_icon_menu_->addAction(minimize_action);
    tray_icon_menu_->addAction(maximize_action);
    tray_icon_menu_->addAction(restore_action);
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

    window()->close();
}

void Xamp::setDefaultStyle() {
    ThemeManager::instance().setDefaultStyle(ui_);
    applyTheme(ThemeManager::instance().getBackgroundColor());
}

void Xamp::registerMetaType() {
    qRegisterMetaTypeStreamOperators<AppEQSettings>("AppEQSettings");
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
    auto f = font();
    f.setPointSize(10);
    ui_.titleLabel->setFont(f);
    f.setPointSize(8);
    ui_.artistLabel->setFont(f);
    if (top_window_->useNativeWindow()) {
        ui_.closeButton->hide();
        ui_.maxWinButton->hide();
        ui_.minWinButton->hide();
    } else {
        f.setBold(true);
        f.setPointSize(12);
        ui_.titleFrameLabel->setFont(f);
        ui_.titleFrameLabel->setText(Q_UTF8("XAMP2"));
        ui_.titleFrameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
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

    auto setMenuBackgroundColor = [menu]() {
        ThemeManager::instance().setMenuStlye(menu);
    };

    setMenuBackgroundColor();
    menu->clear();

    DeviceInfo init_device_info;
    auto is_find_setting_device = false;

    auto* device_action_group = new QActionGroup(this);
    device_action_group->setExclusive(true);

    std::map<std::string, QAction*> device_id_action;

    const auto device_type_id = AppSettings::getID(kAppSettingDeviceType);
    const auto device_id = AppSettings::getValueAsString(kAppSettingDeviceId).toStdString();
    const auto & device_manager = player_->GetAudioDeviceManager();

    for (auto itr = device_manager->Begin(); itr != device_manager->End(); ++itr) {
        auto device_type = (*itr).second();
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            return;
        }        

        menu->addAction(createTextSeparator(fromStdStringView(device_type->GetDescription())));

        for (const auto& device_info : device_info_list) {
            auto* device_action = new QAction(QString::fromStdWString(device_info.name), this);
            switch (device_info.connect_type) {
            case DeviceConnectType::USB:
                device_action->setIcon(ThemeManager::instance().usb());
                break;
            default:
                device_action->setIcon(ThemeManager::instance().speaker());
                break;
            }
            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_id_action[device_info.device_id] = device_action;

            auto trigger_callback = [device_info, setMenuBackgroundColor, this]() {
                device_info_ = device_info;
                AppSettings::setValue(kAppSettingDeviceType, device_info_.device_type_id);
                AppSettings::setValue(kAppSettingDeviceId, device_info_.device_id);
                setMenuBackgroundColor();
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
                return info.is_default_device && !IsExclusiveDevice(info);
            });
            if (itr != device_info_list.end()) {
                init_device_info = (*itr);
            }
        }
    }

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
        top_window_->showMinimized();
    });

    (void)QObject::connect(ui_.maxWinButton, &QToolButton::pressed, [this]() {
        if (top_window_->isMaximized()) {
            top_window_->showNormal();
        }
        else {
            top_window_->showMaximized();
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
            top_window_->setTaskbarPlayingResume();
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
        /*auto old_value = ui_.volumeSlider->value();
        if (!player_->IsHardwareControlVolume()) {
            ui_.volumeSlider->setValue(old_value);
            Toast::showTip(tr("Device not supports a software volume control."), this);
            return;
        }*/
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + Q_UTF8("%"));
        setVolume(volume);
    });

    (void)QObject::connect(ui_.volumeSlider, &SeekSlider::leftButtonValueChanged, [this](auto volume) {
        /*if (!player_->IsHardwareControlVolume()) {
            Toast::showTip(tr("Device not supports a software volume control."), this);
            return;
        }*/
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

    (void)QObject::connect(ui_.eqButton, &QToolButton::pressed, [this]() {
        EqualizerDialog eq(this);

        (void)QObject::connect(&eq, &EqualizerDialog::bandValueChange, [this](int band, float value, float Q) {
            player_->SetEq(band, value, Q);
        });

        (void)QObject::connect(&eq, &EqualizerDialog::preampValueChange, [this](float value) {
            player_->SetPreamp(value);
        });

        eq.exec();
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

    (void)QObject::connect(ui_.artistLabel, &ClickableLabel::clicked, [this]() {
        onArtistIdChanged(current_entity_.artist, current_entity_.cover_id, current_entity_.artist_id);
    });

    (void)QObject::connect(ui_.coverLabel, &ClickableLabel::clicked, [this]() {
        ui_.currentView->setCurrentWidget(lrc_page_);
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::clickedTable, [this](auto table_id) {
    	switch (table_id) {
        case TAB_ALBUM:
            album_artist_page_->refreshOnece();
            ui_.currentView->setCurrentWidget(album_artist_page_);
            break;
        case TAB_ARTIST:
            ui_.currentView->setCurrentWidget(artist_info_page_);
            break;
        case TAB_PODCAST:
            ui_.currentView->setCurrentWidget(podcast_page_);
            current_playlist_page_ = podcast_page_;
            break;
        case TAB_PLAYLIST:
            ui_.currentView->setCurrentWidget(playlist_page_);
            current_playlist_page_ = playlist_page_;
            break;
        case TAB_LYRICS:
            ui_.currentView->setCurrentWidget(lrc_page_);
            break;
        case TAB_SETTINGS:
            ui_.currentView->setCurrentWidget(preference_page_);
            break;
        case TAB_ABOUT:
            ui_.currentView->setCurrentWidget(about_page_);
            break;
        case TAB_YT_MUSIC:
            if (!ytmusic_view_->isLoaded()) {
                ytmusic_view_->indexPage();
            }
            ui_.currentView->setCurrentWidget(ytmusic_view_);
            break;
    	}        
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::tableNameChanged, [](auto table_id, const auto &name) {
        Singleton<Database>::GetInstance().setTableName(table_id, name);
    });

#ifdef Q_OS_WIN
    if (AppSettings::getValueAsBool(kAppSettingDiscordNotify)) {
        (void)QObject::connect(this,
            &Xamp::nowPlaying,
            &discord_notify_,
            &DicordNotify::OnNowPlaying);

        (void)QObject::connect(state_adapter_.get(),
            &UIPlayerStateAdapter::stateChanged,
            &discord_notify_,
            &DicordNotify::OnStateChanged,
            Qt::QueuedConnection);
    }    
#endif

    auto* settings_menu = new QMenu(this);
    auto hide_widget = [this](bool enable) {
        if (!enable) {
            top_window_->resize(QSize(700, 80));
            top_window_->setMinimumSize(QSize(700, 80));
            top_window_->setMaximumSize(QSize(700, 80));
        }
        else {
            top_window_->resize(QSize(1300, 860));
            top_window_->setMinimumSize(QSize(16777215, 16777215));
            top_window_->setMaximumSize(QSize(16777215, 16777215));
        }
    };

    // Hide left list
    auto* hide_left_list_action = new QAction(tr("Show left list"), this);
    hide_left_list_action->setCheckable(true);
	if (AppSettings::getValue(kAppSettingShowLeftList).toBool()) {
        hide_left_list_action->setChecked(true);
        hide_widget(true);
        ui_.sliderFrame->setVisible(true);
        ui_.currentView->setVisible(true);
        ui_.volumeFrame->setVisible(true);
	} else {
        hide_widget(false);
        ui_.sliderFrame->setVisible(false);
        ui_.currentView->setVisible(false);
        ui_.volumeFrame->setVisible(false);
	}

    (void)QObject::connect(hide_left_list_action, &QAction::triggered, [=]() {
        auto enable = AppSettings::getValueAsBool(kAppSettingShowLeftList);
        enable = !enable;
        hide_left_list_action->setChecked(enable);
        AppSettings::setValue(kAppSettingShowLeftList, enable);
        hide_widget(enable);
        ui_.sliderFrame->setVisible(enable);
        ui_.currentView->setVisible(enable);
        ui_.volumeFrame->setVisible(enable);
        });
    settings_menu->addAction(hide_left_list_action);

    auto* enable_blur_material_mode_action = new QAction(tr("Enable blur"), this);
    enable_blur_material_mode_action->setCheckable(true);
    if (AppSettings::getValue(kAppSettingEnableBlur).toBool()) {
        enable_blur_material_mode_action->setChecked(true);
    }

    (void)QObject::connect(enable_blur_material_mode_action, &QAction::triggered, [=]() {
        auto enable = AppSettings::getValueAsBool(kAppSettingEnableBlur);
        enable = !enable;
        enable_blur_material_mode_action->setChecked(enable);
        ThemeManager::instance().enableBlur(ui_.sliderFrame, enable);
        });
    settings_menu->addAction(enable_blur_material_mode_action);

    settings_menu->addSeparator();
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
        color.setAlpha(0);
    } else {
        color.setAlpha(255);
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

    if (color.alpha() > 0) {
        setStyleSheet(Q_STR(R"(#XampWindow { background-color: %1; })").arg(colorToString(color)));
    }
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
        if (auto* page = dynamic_cast<PlaylistPage*>(ui_.currentView->widget(idx))) {
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

    auto* play_key = new QShortcut(QKeySequence(Qt::Key_F4), this);
    (void)QObject::connect(play_key, &QShortcut::activated, [this]() {
        playNextItem(1);
        });
}

void Xamp::stopPlayedClicked() {
    player_->Stop(false, true);
    setSeekPosValue(0);
    ui_.seekSlider->setEnabled(false);
    playlist_page_->playlist()->removePlaying();
    ThemeManager::instance().setPlayOrPauseButton(ui_, false);
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
        Toast::showTip(tr("Repeat once"), this);
        ThemeManager::instance().setRepeatOncePlayOrder(ui_);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE));
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        Toast::showTip(tr("Repeat one"), this);
        ThemeManager::instance().setRepeatOnePlayOrder(ui_);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_REPEAT_ONE));
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        Toast::showTip(tr("Shuffle all"), this);
        ThemeManager::instance().setShufflePlayorder(ui_);
        AppSettings::setValue(kAppSettingOrder,
                              static_cast<int>(PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL));
        break;
    case PlayerOrder::PLAYER_ORDER_MAX:
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
    top_window_->setTaskbarProgress(static_cast<int32_t>(100.0 * ui_.seekSlider->value() / ui_.seekSlider->maximum()));
    lrc_page_->lyricsWidget()->setLrcTime(stream_time_as_ms);
}

void Xamp::playLocalFile(const PlayListEntity& item) {
    top_window_->setTaskbarPlayerPlaying();
    play(item);
}

void Xamp::play() {
    XAMP_LOG_DEBUG("Player state:{}", player_->GetState());

    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
        ThemeManager::instance().setPlayOrPauseButton(ui_, false);
        player_->Pause();
        top_window_->setTaskbarPlayerPaused();
    }
    else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
        ThemeManager::instance().setPlayOrPauseButton(ui_, true);
        player_->Resume();
        top_window_->setTaskbarPlayingResume();
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

    try {
	    uint32_t target_sample_rate = 0;
	    AlignPtr<ISampleRateConverter> converter;
	    if (AppSettings::getValue(kAppSettingResamplerEnable).toBool()) {
            auto soxr_settings = JsonSettings::getValue(AppSettings::getValueAsString(kAppSettingSoxrSettingName)).toMap();
            target_sample_rate = soxr_settings[kSoxrResampleSampleRate].toUInt();
            converter = makeSampleRateConverter(soxr_settings);            
        }
        player_->Open(item.file_path.toStdWString(), device_info_, target_sample_rate, std::move(converter));
        if (item.true_peak >= 1.0) {
            player_->AddDSP(read_utiltis::makeCompressor(player_->GetInputFormat().GetSampleRate()));
        }
        if (AppSettings::getValueAsBool(kEnableEQ)) {
            player_->AddDSP(MakeEqualizer());
            if (AppSettings::contains(kEQName)) {
                auto eq_setting = AppSettings::getValue(kEQName).value<AppEQSettings>();
                uint32_t i = 0;
                for (auto band : eq_setting.settings.bands) {
                    player_->SetEq(i++, band.gain, band.Q);
                }
                player_->SetPreamp(eq_setting.settings.preamp);
            }
        } else {
            player_->RemoveDSP(IEqualizer::Id);
        }
        player_->PrepareToPlay();        
        playback_format = getPlaybackFormat(player_.get());
        player_->Play();
        open_done = true;
    }
    catch (const Exception & e) {
        XAMP_LOG_DEBUG("Exception: {} {} {}", e.GetErrorMessage(), e.GetExpression(), e.GetStackTrace());
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
                setVolume(ui_.volumeSlider->value());
            } else {
                setVolume(0);
            }
            ui_.volumeSlider->setDisabled(false);
        }
        else {
            ui_.volumeSlider->setDisabled(true);
        }

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
    lrc_page_->lyricsWidget()->loadLrcFile(lrc_path);
	
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

PlaylistPage* Xamp::currentPlyalistPage() {
    current_playlist_page_ = dynamic_cast<PlaylistPage*>(ui_.currentView->currentWidget());
    if (!current_playlist_page_) {
        current_playlist_page_ = playlist_page_;
    }
    return current_playlist_page_;
}

void Xamp::play(const PlayListEntity& item) {    
    playMusic(toMusicEntity(item));
    current_entity_ = item;
    update();
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

void Xamp::setCover(const QPixmap* cover, PlaylistPage* page) {
    if (!cover) {
        cover = &ThemeManager::instance().pixmap().unknownCover();    
    }

	if (!page) {
        page = current_playlist_page_;
	}

    const auto ui_cover = Pixmap::roundImage(
        Pixmap::resizeImage(*cover, ui_.coverLabel->size(), false),
           Pixmap::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);

    const auto playlist_cover = Pixmap::roundImage(
        Pixmap::resizeImage(*cover, page->cover()->size(), false),
        Pixmap::kPlaylistImageRadius);
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
        top_window_->resetTaskbarProgress();
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
    ui_.sliderBar->addTab(tr("Playlists"), TAB_PLAYLIST, ThemeManager::instance().playlistIcon());
    ui_.sliderBar->addTab(tr("Podcast"), TAB_PODCAST, ThemeManager::instance().podcastIcon());
    ui_.sliderBar->addTab(tr("Youtube Music"), TAB_YT_MUSIC, ThemeManager::instance().ytMusicIcon());
    ui_.sliderBar->addSeparator();
    ui_.sliderBar->addTab(tr("Albums"), TAB_ALBUM, ThemeManager::instance().albumsIcon());
	ui_.sliderBar->addTab(tr("Artists"), TAB_ARTIST, ThemeManager::instance().artistsIcon());
    ui_.sliderBar->addTab(tr("Lyrics"), TAB_LYRICS, ThemeManager::instance().subtitleIcon());
    ui_.sliderBar->addSeparator();
    ui_.sliderBar->addTab(tr("Settings"), TAB_SETTINGS, ThemeManager::instance().preferenceIcon());
    ui_.sliderBar->addTab(tr("About"), TAB_ABOUT, ThemeManager::instance().aboutIcon());
    ui_.sliderBar->setCurrentIndex(ui_.sliderBar->model()->index(0, 0));
	
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

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        album_artist_page_,
        &AlbumArtistPage::onThemeChanged);

    artist_info_page_ = new ArtistInfoPage(this);
    preference_page_ = new PreferencePage(this);
    about_page_ = new AboutPage(this);

    QNetworkProxyFactory::setUseSystemConfiguration(false);
    ytmusic_view_ = new YtMusicWebEngineView(this);

    pushWidget(ytmusic_view_);
    pushWidget(lrc_page_);    
    pushWidget(playlist_page_);
    pushWidget(album_artist_page_);
    pushWidget(artist_info_page_);
    pushWidget(podcast_page_);
    pushWidget(preference_page_);
    pushWidget(about_page_);
    goBackPage();
    goBackPage();
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

    (void)QObject::connect(album_artist_page_->album(),
                            &AlbumView::addPlaylist,
                            [this](const auto& entity) {
                                addPlaylistItem(entity);
                            });

    setupPlayNextMusicSignals(true);
}

void Xamp::setupPlayNextMusicSignals(bool add_or_remove) {
    if (add_or_remove) {
        (void)QObject::connect(this,
                                &Xamp::payNextMusic,
                                album_artist_page_->album(),
                                &AlbumView::payNextMusic);
    }
    else {
        (void)QObject::disconnect(this,
                                   &Xamp::payNextMusic,
                                   album_artist_page_->album(),
                                   &AlbumView::payNextMusic);
    }    
}

void Xamp::addItem(const QString& file_name) {
	const auto add_playlist = dynamic_cast<PlaylistPage*>(ui_.currentView->currentWidget()) != nullptr;

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

void Xamp::readFileLUFS(const PlayListEntity& item) {    
    auto dialog = makeProgressDialog(tr("Read progress dialog"),
        tr("Read '") + item.title + tr("' loudness"),
        tr("Cancel"));
    dialog->show();

    try {
        auto [lufs, true_peak] = read_utiltis::readFileLUFS(item.file_path.toStdWString(),
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

void Xamp::encodeFlacFile(const PlayListEntity& item) {
    const auto save_file_name = item.album + Q_UTF8("-") + item.title;
    const auto file_name = QFileDialog::getSaveFileName(this,
        tr("Save Flac file"),
        save_file_name,
        tr("FLAC Files (*.flac)"));

    if (file_name.isNull()) {
        return;
    }

    auto dialog = makeProgressDialog(
        tr("Export progress dialog"),
         tr("Export '") + item.title + tr("' to flac file"),
         tr("Cancel"));
    dialog->show();

    Metadata metadata;
    metadata.album = item.album.toStdWString();
    metadata.artist = item.artist.toStdWString();
    metadata.title = item.title.toStdWString();
    metadata.track = item.track;

    const auto command
    	= Q_STR("-%1 -V").arg(AppSettings::getValue(kFlacEncodingLevel).toInt()).toStdWString();

    try {
        read_utiltis::encodeFlacFile(item.file_path.toStdWString(),
            file_name.toStdWString(),
            command,
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

void Xamp::exportWaveFile(const PlayListEntity& item) {
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
            read_utiltis::export2WaveFile(item.file_path.toStdWString(),
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
            read_utiltis::export2WaveFile(item.file_path.toStdWString(),
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

PlaylistPage* Xamp::newPlaylist(int32_t playlist_id) {
	auto* playlist_page = new PlaylistPage(this);

    ui_.currentView->addWidget(playlist_page);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::playMusic,
                            [this](auto index, const auto& item) {
                                setupPlayNextMusicSignals(false);
                                play(index, item);
                            });

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        playlist_page,
        &PlaylistPage::OnThemeColorChanged);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::removeItems,
                            [](auto playlist_id, const auto& select_music_ids) {
                                IgnoreSqlError(Singleton<Database>::GetInstance().removePlaylistMusic(playlist_id, select_music_ids))
                            });

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::readFileLUFS,
        this, &Xamp::readFileLUFS);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::exportWaveFile,
        this, &Xamp::exportWaveFile);

    (void)QObject::connect(playlist_page->playlist(), &PlayListTableView::encodeFlacFile,
                            this, &Xamp::encodeFlacFile);

    (void)QObject::connect(this, &Xamp::themeChanged,
                            playlist_page->playlist(),
                            &PlayListTableView::onThemeColorChanged);

    playlist_page->playlist()->setPlaylistId(playlist_id);    

    return playlist_page;
}

void Xamp::addDropFileItem(const QUrl& url) {
    addItem(url.toLocalFile());
}

void Xamp::extractFile(const QString& file_path) {
    const auto adapter = QSharedPointer<MetadataExtractAdapter>(new MetadataExtractAdapter());
    (void)QObject::connect(adapter.get(),
        &MetadataExtractAdapter::readCompleted,
        this,
        &Xamp::processMeatadata);
    MetadataExtractAdapter::readFileMetadata(adapter, file_path);
}
