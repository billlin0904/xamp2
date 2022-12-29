#include <QDebug>
#include <QToolTip>
#include <QCloseEvent>
#include <QInputDialog>
#include <QShortcut>
#include <QWidgetAction>
#include <QFileDialog>
#include <QProcess>
#include <QProgressDialog>
#include <QFileSystemWatcher>
#include <QtMath>

#include <base/scopeguard.h>
#include <base/str_utilts.h>
#include <base/logger_impl.h>
#include <base/volume.h>

#ifdef XAMP_OS_WIN
#include <stream/mfaacencoder.h>
#endif

#include <stream/soxresampler.h>
#include <stream/r8brainresampler.h>
#include <stream/idspmanager.h>
#include <stream/pcm2dsdsamplewriter.h>
#include <stream/bassaacfileencoder.h>
#include <stream/dsd_utils.h>
#include <stream/api.h>

#include <output_device/api.h>
#include <output_device/iaudiodevicemanager.h>

#include <player/api.h>

#include <widget/xdialog.h>
#include <widget/appsettings.h>
#include <widget/albumview.h>
#include <widget/lyricsshowwidget.h>
#include <widget/playlisttableview.h>
#include <widget/albumartistpage.h>
#include <widget/lrcpage.h>
#include <widget/str_utilts.h>
#include <widget/playlistpage.h>
#include <widget/xmessagebox.h>
#include <widget/image_utiltis.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/artistinfopage.h>
#include <widget/jsonsettings.h>
#include <widget/ui_utilts.h>
#include <widget/read_utiltis.h>
#include <widget/equalizerdialog.h>
#include <widget/playlisttablemodel.h>
#include <widget/actionmap.h>
#include <widget/spectrumwidget.h>
#include <widget/filesystemviewpage.h>
#include <widget/tooltips.h>
#include <widget/tooltipsfilter.h>
#include <widget/backgroundworker.h>
#include <widget/podcast_uiltis.h>
#include <widget/http.h>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#endif

#include "cdpage.h"
#include "aboutpage.h"
#include "preferencepage.h"
#include "thememanager.h"
#include "version.h"
#include "xamp.h"

static PlayerOrder getNextOrder(PlayerOrder cur) noexcept {
    auto next = static_cast<int32_t>(cur) + 1;
    auto max = static_cast<int32_t>(PlayerOrder::PLAYER_ORDER_MAX);
    return static_cast<PlayerOrder>(next % max);
}

static AlignPtr<IAudioProcessor> makeR8BrainSampleRateConverter() {
    return MakeAlign<IAudioProcessor, R8brainSampleRateConverter>();
}

static AlignPtr<IAudioProcessor> makeSampleRateConverter(const QVariantMap &settings) {
    const auto quality = static_cast<SoxrQuality>(settings[kSoxrQuality].toInt());
    const auto stop_band = settings[kSoxrStopBand].toInt();
    const auto pass_band = settings[kSoxrPassBand].toInt();
    const auto phase = settings[kSoxrPhase].toInt();
    const auto enable_steep_filter = settings[kSoxrEnableSteepFilter].toBool();
    const auto roll_off_level = static_cast<SoxrRollOff>(settings[kSoxrRollOffLevel].toInt());

    auto converter = MakeAlign<IAudioProcessor, SoxrSampleRateConverter>();
    auto* soxr_sample_rate_converter = dynamic_cast<SoxrSampleRateConverter*>(converter.get());
    soxr_sample_rate_converter->SetQuality(quality);
    soxr_sample_rate_converter->SetStopBand(stop_band);
    soxr_sample_rate_converter->SetPassBand(pass_band);
    soxr_sample_rate_converter->SetPhase(phase);
    soxr_sample_rate_converter->SetRollOff(roll_off_level);
    soxr_sample_rate_converter->SetSteepFilter(enable_steep_filter);
    return converter;
}

static PlaybackFormat playbackFormat(IAudioPlayer* player) {
    PlaybackFormat format;

    if (player->IsDSDFile()) {
        format.dsd_mode = player->GetDsdModes();
        format.dsd_speed = *player->GetDSDSpeed();
        format.is_dsd_file = true;
    }

    format.enable_sample_rate_convert = player->GetDSPManager()->IsEnableSampleRateConverter();
    format.file_format = player->GetInputFormat();
    format.output_format = player->GetOutputFormat();
    return format;
}

Xamp::Xamp(const std::shared_ptr<IAudioPlayer>& player)
    : is_seeking_(false)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , lrc_page_(nullptr)
    , playlist_page_(nullptr)
	, podcast_page_(nullptr)
	, music_page_(nullptr)
	, cd_page_(nullptr)
	, current_playlist_page_(nullptr)
    , album_page_(nullptr)
    , artist_info_page_(nullptr)
	, file_system_view_page_(nullptr)
	, tray_icon_menu_(nullptr)
    , tray_icon_(nullptr)
	, top_window_(nullptr)
	, background_worker_(nullptr)
    , state_adapter_(std::make_shared<UIPlayerStateAdapter>())
	, player_(player) {
    ui_.setupUi(this);
}

Xamp::~Xamp() {
    cleanup();  
}

void Xamp::setXWindow(IXWindow* top_window) {
    top_window_ = top_window;
    background_worker_ = new BackgroundWorker();  
    background_worker_->moveToThread(&background_thread_);
    background_thread_.start();
    player_->Startup(state_adapter_);

    initialUI();
    initialController();
    initialPlaylist();
    initialShortcut();
    initialTrayIcon();
    initialSpectrum();

    setPlaylistPageCover(nullptr, playlist_page_);
    setPlaylistPageCover(nullptr, podcast_page_);  
    setPlaylistPageCover(nullptr, cd_page_->playlistPage());
    setPlaylistPageCover(nullptr, file_system_view_page_->playlistPage());

    playlist_page_->hidePlaybackInformation(true);
    podcast_page_->hidePlaybackInformation(true);
    cd_page_->playlistPage()->hidePlaybackInformation(true);
    file_system_view_page_->playlistPage()->hidePlaybackInformation(false);
    album_page_->album()->albumViewPage()->playlistPage()->hidePlaybackInformation(false);
    
    avoidRedrawOnResize();

    applyTheme(qTheme.palette().color(QPalette::WindowText), qTheme.themeTextColor());
    qTheme.setPlayOrPauseButton(ui_, false);       

    const auto tab_name = AppSettings::getValueAsString(kAppSettingLastTabName);
    const auto tab_id = ui_.sliderBar->getTabId(tab_name);
    if (tab_id != -1) {
        ui_.sliderBar->setCurrentIndex(ui_.sliderBar->model()->index(tab_id, 0));
        setCurrentTab(tab_id);
    }

    QTimer::singleShot(300, [this]() {
        initialDeviceList();
        });
}

void Xamp::avoidRedrawOnResize() {
    ui_.coverLabel->setAttribute(Qt::WA_StaticContents);
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

void Xamp::initialSpectrum() {
    if (!AppSettings::getValueAsBool(kAppSettingEnableSpectrum)) {
        return;
    }

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::fftResultChanged,
        lrc_page_->spectrum(),
        &SpectrumWidget::onFFTResultChanged,
        Qt::QueuedConnection);

    lrc_page_->spectrum()->setStyle(AppSettings::getAsEnum<SpectrumStyles>(kAppSettingSpectrumStyles));
    lrc_page_->spectrum()->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(lrc_page_->spectrum(), &SpectrumWidget::customContextMenuRequested, [this](auto pt) {
        ActionMap<SpectrumWidget> action_map(lrc_page_->spectrum());

    (void)action_map.addAction(tr("Bar style"), [this]() {
        lrc_page_->spectrum()->setStyle(SpectrumStyles::BAR_STYLE);
		AppSettings::setEnumValue(kAppSettingSpectrumStyles, SpectrumStyles::BAR_STYLE);
        });

    (void)action_map.addAction(tr("Wave style"), [this]() {
        lrc_page_->spectrum()->setStyle(SpectrumStyles::WAVE_STYLE);
		AppSettings::setEnumValue(kAppSettingSpectrumStyles, SpectrumStyles::WAVE_STYLE);
        });

    (void)action_map.addAction(tr("Wave line style"), [this]() {
        lrc_page_->spectrum()->setStyle(SpectrumStyles::WAVE_LINE_STYLE);
		AppSettings::setEnumValue(kAppSettingSpectrumStyles, SpectrumStyles::WAVE_LINE_STYLE);
        });

    action_map.addSeparator();

    (void)action_map.addAction(tr("No Window"), []() {
        AppSettings::setEnumValue(kAppSettingWindowType, WindowType::NO_WINDOW);
        });

    (void)action_map.addAction(tr("Hamming Window"), []() {
        AppSettings::setEnumValue(kAppSettingWindowType, WindowType::HAMMING);
        });

    (void)action_map.addAction(tr("Blackman harris Window"), []() {
        AppSettings::setEnumValue(kAppSettingWindowType, WindowType::BLACKMAN_HARRIS);
        });

		action_map.exec(pt);
        });
}

void Xamp::initialTrayIcon() {
    if (!AppSettings::getValueAsBool(kAppSettingMinimizeToTray)) {
        return;
    }

    auto* minimize_action = new QAction(tr("Mi&nimize"), this);
    (void)QObject::connect(minimize_action, &QAction::triggered, this, &QWidget::hide);

    auto* maximize_action = new QAction(tr("Ma&ximize"), this);
    (void)QObject::connect(maximize_action, &QAction::triggered, this, &QWidget::showMaximized);

    auto* restore_action = new QAction(tr("&Restore"), this);
    (void)QObject::connect(restore_action, &QAction::triggered, this, &QWidget::showNormal);

    auto* quit_action = new QAction(tr("&Quit"), this);
    (void)QObject::connect(quit_action, &QAction::triggered, this, &QWidget::close);

    tray_icon_menu_ = new XMenu(this);
    qTheme.setMenuStyle(tray_icon_menu_);

    tray_icon_menu_->addAction(minimize_action);
    tray_icon_menu_->addAction(maximize_action);
    tray_icon_menu_->addAction(restore_action);
    tray_icon_menu_->addSeparator();
    tray_icon_menu_->addAction(quit_action);

    tray_icon_ = new QSystemTrayIcon(qTheme.appIcon(), this);
    tray_icon_->setContextMenu(tray_icon_menu_);
    tray_icon_->setToolTip(kApplicationTitle);
    tray_icon_->show();

    (void)QObject::connect(tray_icon_,
        SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
        this,
        SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
}

void Xamp::updateMaximumState(bool is_maximum) {
    qTheme.updateMaximumIcon(ui_, is_maximum);
}

void Xamp::focusIn() {
    qTheme.updateTitlebarState(ui_.titleFrame, true);
}

void Xamp::focusOut() {
    qTheme.updateTitlebarState(ui_.titleFrame, false);
}

void Xamp::closeEvent(QCloseEvent* event) {
    if (tray_icon_ != nullptr) {
        if (tray_icon_->isVisible() && !isHidden()) {
            const auto minimize_to_tray_ask = AppSettings::getValueAsBool(kAppSettingMinimizeToTrayAsk);
            QMessageBox::StandardButton reply = QMessageBox::No;

            const auto is_min_system_tray = AppSettings::getValueAsBool(kAppSettingMinimizeToTray);

            if (!is_min_system_tray && minimize_to_tray_ask) {
                QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
                auto [show_again_res, reply_res] = showDontShowAgainDialog(minimize_to_tray_ask);
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
    }

    AppSettings::setValue(kAppSettingVolume, ui_.volumeSlider->value());
    
    cleanup();
    window()->close();
}

void Xamp::cleanup() {
    if (player_ != nullptr) {
        player_->Destroy();
        player_.reset();
    }

    if (!background_thread_.isFinished()) {
        if (background_worker_ != nullptr) {
            background_worker_->stopThreadPool();
        }       
        background_thread_.quit();
        background_thread_.wait();
    }
    if (top_window_ != nullptr) {
        top_window_->saveGeometry();
    }
}

void Xamp::initialUI() {
    QFont f(qTEXT("DisplayFont"));
    f.setWeight(QFont::DemiBold);
    f.setPointSize(qTheme.fontSize());
    ui_.titleLabel->setFont(f);

    f.setWeight(QFont::Normal);
    f.setPointSize(qTheme.fontSize());
    ui_.artistLabel->setFont(f);

    ui_.bitPerfectButton->setFont(f);

    QToolTip::setFont(QFont(qTEXT("FormatFont")));
    ui_.formatLabel->setFont(QFont(qTEXT("FormatFont")));

    ui_.formatLabel->setStyleSheet(qTEXT("background-color: transparent"));
    ui_.hiResLabel->setStyleSheet(qTEXT("background-color: transparent"));

    if (qTheme.useNativeWindow()) {
        ui_.closeButton->hide();
        ui_.maxWinButton->hide();
        ui_.minWinButton->hide();
        ui_.horizontalLayout->removeItem(ui_.horizontalSpacer_15);        
    } else {
        f.setWeight(QFont::DemiBold);
        f.setPointSize(qTheme.fontSize());
        ui_.titleFrameLabel->setFont(f);
        ui_.titleFrameLabel->setText(qTEXT("XAMP2"));
        ui_.titleFrameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    QFont mono_font(qTEXT("MonoFont"));
#ifdef Q_OS_WIN
    mono_font.setPointSize(qTheme.fontSize());
    ui_.startPosLabel->setFont(mono_font);
    ui_.endPosLabel->setFont(mono_font);
#else
    f.setPointSize(9);
    ui_.startPosLabel->setFont(mono_font);
    ui_.endPosLabel->setFont(mono_font);
#endif

    ui_.searchLineEdit->addAction(qTheme.iconFromFont(Glyphs::ICON_SEARCH), QLineEdit::LeadingPosition);
    top_window_->setTitleBarAction(ui_.titleFrame);
}

void Xamp::onVolumeChanged(float volume) {
    setVolume(static_cast<int32_t>(volume * 100.0f));
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

QWidgetAction* Xamp::createTextSeparator(const QString& desc) {
    QWidget* desc_label = new QLabel(desc);

    desc_label->setObjectName(qTEXT("textSeparator"));

    QFont f(qTEXT("DisplayFont"));
    f.setPointSize(qTheme.fontSize());
    f.setBold(true);
    desc_label->setFont(f);

    auto* frame = new QFrame();
    qTheme.setTextSeparator(frame);
    auto* default_layout = new QVBoxLayout(frame);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 0, 0, 0);

    default_layout->addWidget(desc_label);
    frame->setLayout(default_layout);

    auto* separator = new QWidgetAction(this);
    separator->setDefaultWidget(frame);
    return separator;
}

void Xamp::initialDeviceList() {    
    auto* menu = ui_.selectDeviceButton->menu();
    if (!menu) {
        menu = new XMenu();
        qTheme.setMenuStyle(menu);
        ui_.selectDeviceButton->setMenu(menu);
    }

    menu->clear();

    DeviceInfo init_device_info;
    auto is_find_setting_device = false;

    auto* device_action_group = new QActionGroup(this);
    device_action_group->setExclusive(true);

    OrderedMap<std::string, QAction*> device_id_action;

    const auto device_type_id = AppSettings::getValueAsID(kAppSettingDeviceType);
    const auto device_id = AppSettings::getValueAsString(kAppSettingDeviceId).toStdString();
    const auto & device_manager = player_->GetAudioDeviceManager();

    auto set_device_type_icon = [this](auto connect_type) {
        switch (connect_type) {
        case DeviceConnectType::UKNOWN:
        case DeviceConnectType::BUILT_IN:
            ui_.selectDeviceButton->setIcon(qTheme.iconFromFont(Glyphs::ICON_BUILD_IN_SPEAKER));
            break;
        case DeviceConnectType::USB:
            ui_.selectDeviceButton->setIcon(qTheme.iconFromFont(Glyphs::ICON_USB));
            break;
        case DeviceConnectType::BLUE_TOOTH:
            ui_.selectDeviceButton->setIcon(qTheme.iconFromFont(Glyphs::ICON_BLUE_TOOTH));
            break;
        }
    };

    for (auto itr = device_manager->Begin(); itr != device_manager->End(); ++itr) {
	    const auto device_type = (*itr).second();
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            return;
        }

        menu->addSeparator();
        menu->addAction(createTextSeparator(fromStdStringView(device_type->GetDescription())));

        for (const auto& device_info : device_info_list) {
            auto* device_action = new QAction(QString::fromStdWString(device_info.name), this);

            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_action->setChecked(false);
            device_id_action[device_info.device_id] = device_action;            

            auto trigger_callback = [device_info, set_device_type_icon, this]() {
                set_device_type_icon(device_info.connect_type);
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
                set_device_type_icon(device_info.connect_type);
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
        set_device_type_icon(device_info_.connect_type);
        device_id_action[device_info_.device_id]->setChecked(true);
        AppSettings::setValue(kAppSettingDeviceType, device_info_.device_type_id);
        AppSettings::setValue(kAppSettingDeviceId, device_info_.device_id);
        XAMP_LOG_DEBUG("Use default device Id : {}", device_info_.device_id);
    }
}

void Xamp::sliderAnimation(bool enable) {
    auto* animation = new QPropertyAnimation(ui_.sliderFrame, "geometry");
    const auto slider_geometry = ui_.sliderFrame->geometry();
    constexpr auto kMaxSliderWidth = 200;
    constexpr auto kMinSliderWidth = 43;
    QSize size;
    if (!enable) {
        ui_.searchFrame->hide();
        ui_.tableLabel->hide();
        animation->setEasingCurve(QEasingCurve::InCubic);
        animation->setStartValue(QRect(slider_geometry.x(), slider_geometry.y(), kMaxSliderWidth, slider_geometry.height()));
        animation->setEndValue(QRect(slider_geometry.x(), slider_geometry.y(), kMinSliderWidth, slider_geometry.height()));
        size = QSize(kMinSliderWidth, slider_geometry.height());
    }
    else {
        animation->setEasingCurve(QEasingCurve::InExpo);
        animation->setStartValue(QRect(slider_geometry.x(), slider_geometry.y(), kMinSliderWidth, slider_geometry.height()));
        animation->setEndValue(QRect(slider_geometry.x(), slider_geometry.y(), kMaxSliderWidth, slider_geometry.height()));
        size = QSize(kMaxSliderWidth, slider_geometry.height());
    }

    (void)QObject::connect(animation, &QPropertyAnimation::finished, [=]() {
        if (enable) {
            ui_.searchFrame->show();
            ui_.tableLabel->show();
        }
        ui_.sliderFrame->setMaximumSize(size);
        });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Xamp::initialController() {
    (void)QObject::connect(ui_.minWinButton, &QToolButton::pressed, [this]() {
        top_window_->showMinimized();
    });

    (void)QObject::connect(ui_.maxWinButton, &QToolButton::pressed, [this]() {
        top_window_->updateMaximumState();
    });

    (void)QObject::connect(ui_.mutedButton, &QToolButton::pressed, [this]() {
        if (!ui_.volumeSlider->isEnabled()) {
            return;
        }
        if (player_->IsMute()) {
            player_->SetMute(false);            
            qTheme.setMuted(ui_, false);
        } else {
            player_->SetMute(true);
            qTheme.setMuted(ui_, true);
        }
    });

    (void)QObject::connect(ui_.closeButton, &QToolButton::pressed, [this]() {
        QWidget::close();
    });

    qTheme.setBitPerfectButton(ui_, AppSettings::getValueAsBool(kEnableBitPerfect));

    (void)QObject::connect(ui_.bitPerfectButton, &QToolButton::pressed, [this]() {
	    const auto enable_or_disable = !AppSettings::getValueAsBool(kEnableBitPerfect);
        AppSettings::setValue(kEnableBitPerfect, enable_or_disable);
        qTheme.setBitPerfectButton(ui_, enable_or_disable);
        });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::leftButtonValueChanged, [this](auto value) {
        try {
			player_->Seek(value);
            qTheme.setPlayOrPauseButton(ui_, true);
            top_window_->setTaskbarPlayingResume();
        }
        catch (const Exception & e) {
            player_->Stop(false);
            XMessageBox::showError(qTEXT(e.GetErrorMessage()));
        }
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::sliderReleased, [this]() {
        is_seeking_ = false;
        XAMP_LOG_DEBUG("SeekSlider released!");
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::sliderPressed, [this]() {
        is_seeking_ = false;
        XAMP_LOG_DEBUG("sliderPressed pressed!");
    });

    order_ = AppSettings::getAsEnum<PlayerOrder>(kAppSettingOrder);
    setPlayerOrder();

    ui_.volumeSlider->setRange(0, 100);

    if (AppSettings::getValueAsBool(kAppSettingIsMuted)) {
        setVolume(0);
    } else {
        const auto vol = AppSettings::getValue(kAppSettingVolume).toUInt();
        setVolume(vol);
        ui_.volumeSlider->setValue(vol);
    }    

    (void)QObject::connect(ui_.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
        setVolume(volume);
    });

    (void)QObject::connect(ui_.volumeSlider, &SeekSlider::leftButtonValueChanged, [this](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + qTEXT("%"));
        setVolume(volume);
    });

    (void)QObject::connect(ui_.volumeSlider, &QSlider::sliderMoved, [](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + qTEXT("%"));
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
            emit album_page_->album()->onSearchTextChanged(text);
        }
    });

    (void)QObject::connect(ui_.nextButton, &QToolButton::pressed, [this]() {
        playNext();
    });

    (void)QObject::connect(ui_.prevButton, &QToolButton::pressed, [this]() {
        playPrevious();
    });

    (void)QObject::connect(ui_.eqButton, &QToolButton::pressed, [this]() {
        auto* dialog = new XDialog(this);
        auto* eq = new EqualizerDialog(dialog);
        dialog->setContentWidget(eq);
        dialog->setTitle(tr("Equalizer"));

        (void)QObject::connect(eq, &EqualizerDialog::bandValueChange, [](auto, auto, auto) {
            AppSettings::save();
        });

        (void)QObject::connect(eq, &EqualizerDialog::preampValueChange, [](auto) {
            AppSettings::save();
        });

        QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
        dialog->exec();
    });

    (void)QObject::connect(ui_.repeatButton, &QToolButton::pressed, [this]() {
        order_ = getNextOrder(order_);
        setPlayerOrder();
    });

    (void)QObject::connect(ui_.playButton, &QToolButton::pressed, [this]() {
        playOrPause();
    });

    (void)QObject::connect(ui_.stopButton, &QToolButton::pressed, [this]() {
        stop();
    });

    (void)QObject::connect(ui_.artistLabel, &ClickableLabel::clicked, [this]() {
        onArtistIdChanged(current_entity_.artist, current_entity_.cover_id, current_entity_.artist_id);
    });

    (void)QObject::connect(ui_.coverLabel, &ClickableLabel::clicked, [this]() {
        ui_.currentView->setCurrentWidget(lrc_page_);
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::clickedTable, [this](auto table_id) {
        setCurrentTab(table_id);
        AppSettings::setValue(kAppSettingLastTabName, ui_.sliderBar->getTabName(table_id));
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::tableNameChanged, [](auto table_id, const auto &name) {
        qDatabase.setTableName(table_id, name);
    });

    tool_tips_filter_ = new ToolTipsFilter(this);
    setTipHint(ui_.playButton, tr("Play/Pause"));
    setTipHint(ui_.stopButton, tr("Stop"));
    setTipHint(ui_.prevButton, tr("Previous"));
    setTipHint(ui_.nextButton, tr("Next"));
    setTipHint(ui_.coverLabel, tr("Cover"));
    setTipHint(ui_.selectDeviceButton, tr("Device list"));
    setTipHint(ui_.eqButton, tr("Equalizer"));

    setTipHint(ui_.closeButton, tr("Close window"));
    setTipHint(ui_.maxWinButton, tr("Maximum window"));
    setTipHint(ui_.minWinButton, tr("Minimum window"));

    QTimer::singleShot(500, [this]() {
        sliderAnimation(AppSettings::getValueAsBool(kAppSettingShowLeftList));
        });

    ui_.sliderBarButton->setIconSize(qTheme.tabIconSize());
   (void)QObject::connect(ui_.sliderBarButton, &QToolButton::clicked, [this]() {
	   const auto enable = !AppSettings::getValueAsBool(kAppSettingShowLeftList);
       AppSettings::setValue(kAppSettingShowLeftList, enable);
       sliderAnimation(enable);
        });   

    auto* check_for_update = new QAction(tr("Check For Updates"), this);

#if 0
    static const QString kSoftwareUpdateUrl =
        qTEXT("https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json");
    auto* updater = QSimpleUpdater::getInstance();

    if (AppSettings::getValueAsBool(kAppSettingAutoCheckForUpdate)) {
        (void)QObject::connect(updater, &QSimpleUpdater::checkingFinished, [updater, this](auto url) {
            auto change_log = updater->getChangelog(url);

            auto html = qTEXT(R"(
            <h3>Find New Version:</h3> 			
			<br>
            %1
			</br>
           )").arg(change_log);

            QMessageBox::about(this,
                qTEXT("Check For Updates"),
                html);
            });

        (void)QObject::connect(updater, &QSimpleUpdater::downloadFinished, [updater, this](auto url, auto filepath) {
            XAMP_LOG_DEBUG("Donwload path: {}", filepath.toStdString());
            });

        (void)QObject::connect(updater, &QSimpleUpdater::appcastDownloaded, [updater](auto url, auto reply) {
            XAMP_LOG_DEBUG(QString::fromUtf8(reply).toStdString());
            });

        updater->setPlatformKey(kSoftwareUpdateUrl, qTEXT("windows"));
        updater->setModuleVersion(kSoftwareUpdateUrl, kXAMPVersion);
        updater->setNotifyOnFinish(kSoftwareUpdateUrl, false);
        updater->setNotifyOnUpdate(kSoftwareUpdateUrl, true);
        updater->setUseCustomAppcast(kSoftwareUpdateUrl, false);
        updater->setDownloaderEnabled(kSoftwareUpdateUrl, false);
        updater->setMandatoryUpdate(kSoftwareUpdateUrl, false);
        updater->checkForUpdates(kSoftwareUpdateUrl);

        (void)QObject::connect(check_for_update, &QAction::triggered, [=]() {
            updater->checkForUpdates(kSoftwareUpdateUrl);
            });
    }    
#endif

    auto* about_action = new QAction(tr("About"), this);
    ui_.seekSlider->setEnabled(false);
    ui_.startPosLabel->setText(streamTimeToString(0));
    ui_.endPosLabel->setText(streamTimeToString(0));
    ui_.searchLineEdit->setPlaceholderText(tr("Search anything"));
}

void Xamp::setCurrentTab(int32_t table_id) {
    switch (table_id) {
    case TAB_ALBUM:
        album_page_->refreshOnece();
        ui_.currentView->setCurrentWidget(album_page_);
        break;
    case TAB_ARTIST:
        ui_.currentView->setCurrentWidget(artist_info_page_);
        break;
    case TAB_FILE_EXPLORER:
        ui_.currentView->setCurrentWidget(file_system_view_page_);
        break;
    case TAB_PODCAST:
        ui_.currentView->setCurrentWidget(podcast_page_);
        break;
    case TAB_PLAYLIST:
        ui_.currentView->setCurrentWidget(playlist_page_);
        break;
    case TAB_LYRICS:
        ui_.currentView->setCurrentWidget(lrc_page_);
        break;
    case TAB_SETTINGS:
        ui_.currentView->setCurrentWidget(preference_page_);
        break;
    case TAB_CD:
        ui_.currentView->setCurrentWidget(cd_page_);
        break;
    case TAB_ABOUT:
        ui_.currentView->setCurrentWidget(about_page_);
        break;
    }
}

void Xamp::updateButtonState() {    
    qTheme.setPlayOrPauseButton(ui_, player_->GetState() != PlayerState::PLAYER_STATE_PAUSED);
    preference_page_->update();
}

void Xamp::systemThemeChanged(ThemeColor theme_color) {
    if (qTheme.themeColor() == theme_color) {
        return;
    }

	switch (theme_color) {
	case ThemeColor::DARK_THEME:
        qTheme.setThemeColor(ThemeColor::DARK_THEME);		
        break;
    case ThemeColor::LIGHT_THEME:
        qTheme.setThemeColor(ThemeColor::LIGHT_THEME);
        break;
	}
    qTheme.applyTheme();

    /*cleanup();
    qApp->exit(kRestartExistCode);*/
}

void Xamp::applyTheme(QColor backgroundColor, QColor color) {
    themeChanged(backgroundColor, color);
    qTheme.setBackgroundColor(ui_, backgroundColor);
    qTheme.setWidgetStyle(ui_);
    updateButtonState();
}

void Xamp::shortcutsPressed(const QKeySequence& shortcut) {
    XAMP_LOG_DEBUG("shortcutsPressed: {}", shortcut.toString().toStdString());

	const QMap<QKeySequence, std::function<void()>> shortcut_map {
        { QKeySequence(Qt::Key_MediaPlay), [this]() {
            playOrPause();
            }},
         { QKeySequence(Qt::Key_MediaStop), [this]() {
            stop();
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
        { QKeySequence(Qt::Key_VolumeMute), [this]() {
            setVolume(0);
            }},
    };

    auto key = shortcut_map.value(shortcut);
    if (key != nullptr) {
        key();
    }
}

void Xamp::getNextPage() {
    if (stack_page_id_.isEmpty()) {
        return;
    }
    auto idx = ui_.currentView->currentIndex();
    ui_.currentView->setCurrentIndex(idx + 1);
}

void Xamp::setTablePlaylistView(int table_id, ConstLatin1String column_setting_name) {
	const auto playlist_id = qDatabase.findTablePlaylistId(table_id);

    auto found = false;
    Q_FOREACH(auto idx, stack_page_id_) {
        if (auto* page = dynamic_cast<PlaylistPage*>(ui_.currentView->widget(idx))) {
            if (page->playlist()->playlistId() == playlist_id) {
                ui_.currentView->setCurrentIndex(idx);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        auto* playlist_page = newPlaylistPage(playlist_id, column_setting_name);
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

void Xamp::setVolume(uint32_t volume) {
    if (volume > 100) {
        return;
    }

    try {
        if (volume > 0) {
            player_->SetMute(false);
        }
        else {
            player_->SetMute(true);
        }

        if (player_->IsHardwareControlVolume()) {
            if (!player_->IsMute()) {
                player_->SetVolume(volume);
                qTheme.setVolume(ui_, volume);
            }
            else {
                qTheme.setVolume(ui_, 0);
            }
        }
        else {
            ui_.volumeSlider->setDisabled(true);
        }

        /*const auto volume_db = VolumeToDb(ui_.volumeSlider->value());
        player_->SetSoftwareVolumeDb(volume_db);*/
    }
    catch (const Exception& e) {
        player_->Stop(false);
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
    }
}

void Xamp::initialShortcut() {
    const auto* space_key = new QShortcut(QKeySequence(Qt::Key_Space), this);
    (void)QObject::connect(space_key, &QShortcut::activated, [this]() {
        playOrPause();
    });

    const auto* play_key = new QShortcut(QKeySequence(Qt::Key_F4), this);
    (void)QObject::connect(play_key, &QShortcut::activated, [this]() {
        playNextItem(1);
        });
}

bool Xamp::hitTitleBar(const QPoint& ps) const {
    return ui_.titleFrame->rect().contains(ps);
}

void Xamp::stop() {
    player_->Stop(true, true);
    setSeekPosValue(0);
    lrc_page_->spectrum()->reset();
    ui_.seekSlider->setEnabled(false);
    playlist_page_->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
    album_page_->album()->albumViewPage()->playlistPage()->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
    qTheme.setPlayOrPauseButton(ui_, false);
}

void Xamp::playNext() {
    playNextItem(1);
}

void Xamp::playPrevious() {
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
	const auto order = AppSettings::getAsEnum<PlayerOrder>(kAppSettingOrder);

    switch (order_) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        if (order_ != order) {
            XMessageBox::showInformation(tr("Repeat once"));
        }
        qTheme.setRepeatOncePlayOrder(ui_);
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        if (order_ != order) {
            XMessageBox::showInformation(tr("Repeat one"));
        }
        qTheme.setRepeatOnePlayOrder(ui_);
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        if (order_ != order) {
            XMessageBox::showInformation(tr("Shuffle all"));
        }
        qTheme.setShufflePlayorder(ui_);
        break;
    default:
        break;
    }
    AppSettings::setEnumValue(kAppSettingOrder, order_);
}

void Xamp::onSampleTimeChanged(double stream_time) {
    if (!player_ || is_seeking_) {
        return;
    }
    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {         
        setSeekPosValue(stream_time);
    }
}

void Xamp::setSeekPosValue(double stream_time) {
    const auto full_text = isMoreThan1Hours(player_->GetDuration());
    ui_.endPosLabel->setText(streamTimeToString(player_->GetDuration() - stream_time, full_text));
    const auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
    ui_.seekSlider->setValue(stream_time);
    ui_.startPosLabel->setText(streamTimeToString(stream_time, full_text));
    top_window_->setTaskbarProgress(static_cast<int32_t>(100.0 * ui_.seekSlider->value() / ui_.seekSlider->maximum()));
    lrc_page_->lyrics()->setLrcTime(stream_time_as_ms);
}

void Xamp::playLocalFile(const PlayListEntity& item) {
    top_window_->setTaskbarPlayerPlaying();
    playPlayListEntity(item);
}

void Xamp::playOrPause() {
    XAMP_LOG_DEBUG("Player state:{}", player_->GetState());

    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
        qTheme.setPlayOrPauseButton(ui_, false);
        player_->Pause();
        current_playlist_page_->playlist()->setNowPlayState(PlayingState::PLAY_PAUSE);
        top_window_->setTaskbarPlayerPaused();
    }
    else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
        qTheme.setPlayOrPauseButton(ui_, true);
        player_->Resume();
        current_playlist_page_->playlist()->setNowPlayState(PlayingState::PLAY_PLAYING);
        top_window_->setTaskbarPlayingResume();
    }
    else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        if (!ui_.currentView->count()) {
            return;
        }
        playlist_page_ = current_playlist_page_;
        if (const auto select_item = playlist_page_->playlist()->selectItem()) {
            play_index_ = select_item.value();
        }
        play_index_ = playlist_page_->playlist()->model()->index(
            play_index_.row(), PLAYLIST_PLAYING);
        if (play_index_.row() == -1) {
            XMessageBox::showInformation(tr("Not found any playing item."));
            return;
        }
        playlist_page_->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
        playlist_page_->playlist()->setNowPlaying(play_index_, true);
        playlist_page_->playlist()->play(play_index_);
    }
}

void Xamp::resetSeekPosValue() {
    ui_.seekSlider->setValue(0);
    ui_.startPosLabel->setText(streamTimeToString(0));
}

void Xamp::processMeatadata(int64_t dir_last_write_time, const ForwardList<TrackInfo>& medata) const {
    MetadataExtractAdapter::processMetadata(medata, nullptr, dir_last_write_time);
    album_page_->album()->refreshOnece();
}

void Xamp::setupDSP(const AlbumEntity& item) {
    if (AppSettings::getValueAsBool(kAppSettingEnableReplayGain)) {
        const auto mode = AppSettings::getAsEnum<ReplayGainMode>(kAppSettingReplayGainMode);
        if (mode == ReplayGainMode::RG_ALBUM_MODE) {
            player_->GetDSPManager()->AddVolume();
            player_->GetDspConfig().AddOrReplace(DspConfig::kVolume, item.album_replay_gain);
        } else if (mode == ReplayGainMode::RG_TRACK_MODE) {
            player_->GetDSPManager()->AddVolume();
            player_->GetDspConfig().AddOrReplace(DspConfig::kVolume, item.track_replay_gain);
        } else {
            player_->GetDSPManager()->RemoveVolume();
        }
    } else {
        player_->GetDSPManager()->RemoveVolume();
    }

    if (AppSettings::getValueAsBool(kAppSettingEnableEQ)) {
        if (AppSettings::contains(kAppSettingEQName)) {
            const auto [name, settings] = 
                AppSettings::getEQSettings();
            player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, settings);
            player_->GetDSPManager()->AddEqualizer();
        }
    } else {
        player_->GetDSPManager()->RemoveEqualizer();
    }

    if (preference_page_->adapter->IsActive()) {
        auto adapter = MakeAlign<IAudioProcessor, FoobarDspAdapter>();
        auto* foo_dsp = static_cast<FoobarDspAdapter*>(adapter.get());
        foo_dsp->CloneDSPChainConfig(*preference_page_->adapter);
        player_->GetDSPManager()->AddPostDSP(std::move(adapter));
    }
}

QString Xamp::translateErrorCode(const Errors error) {
    static const QMap<Errors, QString> et_lut{
        { Errors::XAMP_ERROR_DEVICE_IN_USE, tr("Device in use.") },
        { Errors::XAMP_ERROR_FILE_NOT_FOUND, tr("File not found.") },
        { Errors::XAMP_ERROR_DEVICE_NOT_FOUND, tr("Device not found.") },
        { Errors::XAMP_ERROR_DEVICE_UNSUPPORTED_FORMAT, tr("Device unsupported format.") },
        { Errors::XAMP_ERROR_NOT_SUPPORT_SAMPLERATE, tr("Not support samplate rate.") },
    };
    if (!et_lut.contains(error)) {
        return fromStdStringView(EnumToString(error));
    }
    return et_lut[error];
}

void Xamp::setupSampleWriter(PlaybackFormat& playback_format, QString& samplerate_converter_type, ByteFormat byte_format) {
    const auto input_sample_rate = player_->GetInputFormat().GetSampleRate();
    auto dsd_modes = DsdModes::DSD_MODE_AUTO;

    uint32_t device_sample_rate = 0;
	auto const convert_mode = Pcm2DsdConvertModes::PCM2DSD_DSD_DOP;

	// note: PCM2DSD function have some issue.
	// 1. Only convert 44.1Khz 88.2Khz 176.4Khz ...
	// 2. Fixed sample size input (can't use samplerate converter).

	if (AppSettings::getValueAsBool(kEnablePcm2Dsd)
        && !player_->IsDSDFile()
		&& !player_->GetDSPManager()->IsEnableSampleRateConverter()
		&& input_sample_rate % kPcmSampleRate441 == 0) {
		auto config = JsonSettings::getValueAsMap(kPCM2DSD);
		auto dsd_times = static_cast<DsdTimes>(config[kPCM2DSDDsdTimes].toInt());
		auto pcm2dsd_writer = MakeAlign<ISampleWriter, Pcm2DsdSampleWriter>(dsd_times);
		auto* writer = dynamic_cast<Pcm2DsdSampleWriter*>(pcm2dsd_writer.get());

		CpuAffinity affinity;
		affinity.Set(2);
		affinity.Set(3);
		writer->Init(input_sample_rate, affinity, convert_mode);

		if (convert_mode == Pcm2DsdConvertModes::PCM2DSD_DSD_DOP) {
			device_sample_rate = GetDOPSampleRate(writer->GetDsdSpeed());
			dsd_modes = DsdModes::DSD_MODE_DOP;
		} else {
			device_sample_rate = writer->GetDsdSampleRate();
			dsd_modes = DsdModes::DSD_MODE_NATIVE;
		}

		player_->GetDSPManager()->SetSampleWriter(std::move(pcm2dsd_writer));

		player_->PrepareToPlay(byte_format, device_sample_rate, dsd_modes);
		player_->SetReadSampleSize(writer->GetDataSize() * 2);

        samplerate_converter_type = qEmptyString;
		playback_format = playbackFormat(player_.get());
		playback_format.is_dsd_file = true;
		playback_format.dsd_mode = dsd_modes;
		playback_format.dsd_speed = writer->GetDsdSpeed();
		playback_format.output_format.SetSampleRate(device_sample_rate);
	} else {
		player_->GetDSPManager()->SetSampleWriter();
		player_->PrepareToPlay(byte_format, device_sample_rate, dsd_modes);
		playback_format = playbackFormat(player_.get());
	}
}

void Xamp::showMeMessage(const QString& message) {
    if (AppSettings::showMeAgain(message)) {
        if (XMessageBox::showCheckBoxInformation(
            message,
            tr("Ok, and don't show again.")) == QDialogButtonBox::Yes) {
            AppSettings::addDontShowMeAgain(message);
        }
    }
}

void Xamp::setupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
    uint32_t &target_sample_rate,
    QString& sample_rate_converter_type) {
    sample_rate_converter_type = AppSettings::getValueAsString(kAppSettingResamplerType);

    if (!AppSettings::getValueAsBool(kEnableBitPerfect)) {
		if (AppSettings::getValueAsBool(kAppSettingResamplerEnable)) {
			if (sample_rate_converter_type == kSoxr || sample_rate_converter_type.isEmpty()) {
				QMap<QString, QVariant> soxr_settings;
				const auto setting_name = AppSettings::getValueAsString(kAppSettingSoxrSettingName);
				soxr_settings = JsonSettings::getValue(kSoxr).toMap()[setting_name].toMap();
				target_sample_rate = soxr_settings[kResampleSampleRate].toUInt();

				initial_sample_rate_converter = [=]() {
					player_->GetDSPManager()->AddPreDSP(makeSampleRateConverter(soxr_settings));
				};
			}
			else if (sample_rate_converter_type == kR8Brain) {
				auto config = JsonSettings::getValueAsMap(kR8Brain);
				target_sample_rate = config[kResampleSampleRate].toUInt();

				initial_sample_rate_converter = [=]() {
					player_->GetDSPManager()->AddPreDSP(makeR8BrainSampleRateConverter());
				};
			}
		}
	}
}

void Xamp::playAlbumEntity(const AlbumEntity& item) {
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

    QString sample_rate_converter_type;
    PlaybackFormat playback_format;
    uint32_t target_sample_rate = 0;
    auto byte_format = ByteFormat::SINT32;

    std::function<void()> sample_rate_converter_factory;
    setupSampleRateConverter(sample_rate_converter_factory, target_sample_rate, sample_rate_converter_type);

    player_->EnableFadeOut(false);

    try {
        player_->Open(item.file_path.toStdWString(), device_info_, target_sample_rate);

        if (!sample_rate_converter_factory) {
            player_->GetDSPManager()->RemoveSampleRateConverter();
            player_->GetDSPManager()->RemoveVolume();
            player_->GetDSPManager()->RemoveEqualizer();

            if (player_->GetInputFormat().GetByteFormat() == ByteFormat::SINT16) {
                byte_format = ByteFormat::SINT16;
            }
        } else {
            if (player_->GetInputFormat().GetSampleRate() == target_sample_rate) {
                player_->GetDSPManager()->RemoveSampleRateConverter();
            }
            else {
                sample_rate_converter_factory();
            }
            setupDSP(item);
        }

        player_->GetDSPManager()->AddCompressor();

        if (device_info_.connect_type == DeviceConnectType::BLUE_TOOTH) {
            if (player_->GetInputFormat().GetBitsPerSample() != 16) {
                const auto message = qSTR("Playing blue-tooth device need set %1bit to 16bit depth.")
                    .arg(player_->GetInputFormat().GetBitsPerSample());
                showMeMessage(message);
            }
            byte_format = ByteFormat::SINT16;
        }

        setupSampleWriter(playback_format, sample_rate_converter_type, byte_format);

        playback_format.bitrate = item.bitrate;
        if (sample_rate_converter_type == kR8Brain) {
            player_->SetReadSampleSize(kR8brainBufferSize);
        }

        player_->BufferStream();
        open_done = true;
    }
    catch (const Exception & e) {        
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
        XAMP_LOG_DEBUG(e.GetStackTrace());
    }
    catch (const std::exception & e) {
        XMessageBox::showError(qTEXT(e.what()));
    }
    catch (...) {        
        XMessageBox::showError(tr("Unknown error"));
    }

    updateUI(item, playback_format, open_done);
}

void Xamp::updateUI(const AlbumEntity& item, const PlaybackFormat& playback_format, bool open_done) {
    auto* cur_page = current_playlist_page_;

    QString ext = item.file_ext;
    if (item.file_ext.isEmpty()) {
        ext = qTEXT(".m4a");
    }
	
    qTheme.setPlayOrPauseButton(ui_, open_done);
    lrc_page_->spectrum()->reset();
	
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

        ui_.seekSlider->setRange(0, Round(player_->GetDuration()));
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(streamTimeToString(0));
        ui_.endPosLabel->setText(streamTimeToString(player_->GetDuration()));
        cur_page->format()->setText(format2String(playback_format, ext));

        artist_info_page_->setArtistId(item.artist,
            qDatabase.getArtistCoverId(item.artist_id),
            item.artist_id);

        updateButtonState();
        emit nowPlaying(item.artist, item.title);
    } else {
        cur_page->playlist()->executeQuery();
    }

    setCover(item.cover_id, cur_page);

    ui_.titleLabel->setText(item.title);
    ui_.artistLabel->setText(item.artist);

    cur_page->title()->setText(item.title);    
    lrc_page_->lyrics()->loadLrcFile(item.file_path);
	
    lrc_page_->title()->setText(item.title);
    lrc_page_->album()->setText(item.album);
    lrc_page_->artist()->setText(item.artist);

    if (isHidden()) {
        tray_icon_->showMessage(item.album, 
            item.title, 
            qTheme.appIcon(),
            1000);
    }

    if (open_done) {
        ui_.formatLabel->setText(format2String(playback_format, ext));
        if (playback_format.file_format > AudioFormat::k16BitPCM441Khz) {
            ui_.hiResLabel->show();
            ui_.hiResLabel->setPixmap(qTheme.hiResIcon().pixmap(ui_.hiResLabel->size()));
        } else {
            ui_.hiResLabel->hide();
        }        
        player_->Play();
    }
    podcast_page_->format()->setText(qEmptyString);
}

void Xamp::onUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info) {
    const auto disc_id = QString::fromStdString(mb_disc_id_info.disc_id);
    const auto album = QString::fromStdWString(mb_disc_id_info.album);
    const auto artist = QString::fromStdWString(mb_disc_id_info.artist);

    if (!album.isEmpty()) {
        qDatabase.updateAlbumByDiscId(disc_id, album);
    }
    if (!artist.isEmpty()) {
        qDatabase.updateArtistByDiscId(disc_id, artist);
    }

	const auto album_id = qDatabase.getAlbumIdByDiscId(disc_id);

    if (!mb_disc_id_info.tracks.empty()) {
        qDatabase.forEachAlbumMusic(album_id, [&mb_disc_id_info](const auto& entity) {
            qDatabase.updateMusicTitle(entity.music_id, QString::fromStdWString(mb_disc_id_info.tracks.front().title));
            });
    }    

    if (!album.isEmpty()) {
        cd_page_->playlistPage()->title()->setText(album);
        cd_page_->playlistPage()->playlist()->executeQuery();
    }

    if (const auto album_stats = qDatabase.getAlbumStats(album_id)) {
        cd_page_->playlistPage()->format()->setText(tr("%1 Tracks, %2, %3")
            .arg(QString::number(album_stats.value().tracks))
            .arg(streamTimeToString(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year)));
    }
}

void Xamp::onUpdateDiscCover(const QString& disc_id, const QString& cover_id) {
	const auto album_id = qDatabase.getAlbumIdByDiscId(disc_id);
    qDatabase.setAlbumCover(album_id, cover_id);
    setCover(cover_id, cd_page_->playlistPage());
}

void Xamp::onUpdateCdMetadata(const QString& disc_id, const ForwardList<TrackInfo>& metadatas) {
    const auto album_id = qDatabase.getAlbumIdByDiscId(disc_id);
    qDatabase.removeAlbum(album_id);

    cd_page_->playlistPage()->playlist()->removeAll();
    cd_page_->playlistPage()->playlist()->processMeatadata(QDateTime::currentSecsSinceEpoch(), metadatas);
    cd_page_->showPlaylistPage(true);
}

void Xamp::drivesChanges(const QList<DriveInfo>& drive_infos) {
    cd_page_->playlistPage()->playlist()->removeAll();
    cd_page_->playlistPage()->playlist()->executeQuery();
    emit fetchCdInfo(drive_infos.first());
}

void Xamp::drivesRemoved(const DriveInfo& /*drive_info*/) {
    cd_page_->playlistPage()->playlist()->removeAll();
    cd_page_->playlistPage()->playlist()->executeQuery();
    cd_page_->showPlaylistPage(false);
}

void Xamp::setCover(const QString& cover_id, PlaylistPage* page) {
    auto found_cover = !current_entity_.cover_id.isEmpty();

    if (cover_id != qPixmapCache.getUnknownCoverId()) {
        const auto* cover = qPixmapCache.find(cover_id);
        found_cover = cover != nullptr;
        if (cover != nullptr) {
            setPlaylistPageCover(cover, page);
            emit addBlurImage(cover_id, cover->toImage());
        }
        else if (lrc_page_ != nullptr) {
            lrc_page_->clearBackground();
        }
    }

    if (!found_cover) {
        setPlaylistPageCover(nullptr, page);
        if (lrc_page_ != nullptr) {
            lrc_page_->setBackgroundColor(qTheme.backgroundColor());
        }
    }
}

PlaylistPage* Xamp::currentPlyalistPage() {
    current_playlist_page_ = dynamic_cast<PlaylistPage*>(ui_.currentView->currentWidget());
    if (!current_playlist_page_) {
        current_playlist_page_ = playlist_page_;
    }
    return current_playlist_page_;
}

void Xamp::playPlayListEntity(const PlayListEntity& item) {
    current_playlist_page_ = qobject_cast<PlaylistPage*>(sender());
    top_window_->setTaskbarPlayerPlaying();
    current_entity_ = item;
    playAlbumEntity(toAlbumEntity(item));
    update();
}

void Xamp::playNextItem(int32_t forward) {
    if (!current_playlist_page_) {
        currentPlyalistPage();
    }

    auto* playlist_view = current_playlist_page_->playlist();
    const auto count = playlist_view->model()->rowCount();
    if (count == 0) {
        stop();
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
            XMessageBox::showInformation(tr("Not found any playlist item."));
            return;
        }
    } else {
        play_index_ = playlist_view->model()->index(0, 0);
    }

    playlist_view->play(play_index_);
    playlist_view->reload();
}

void Xamp::onArtistIdChanged(const QString& artist, const QString& /*cover_id*/, int32_t artist_id) {
    artist_info_page_->setArtistId(artist, qDatabase.getArtistCoverId(artist_id), artist_id);
    ui_.currentView->setCurrentWidget(artist_info_page_);
}

void Xamp::addPlaylistItem(const ForwardList<int32_t>& music_ids, const ForwardList<PlayListEntity> & entities) {
    auto playlist_view = playlist_page_->playlist();
    qDatabase.addMusicToPlaylist(music_ids, playlist_view->playlistId());
    emit playlist_view->readReplayGain(false, entities);
    playlist_view->executeQuery();
}

void Xamp::onClickedAlbum(const QString& album, int32_t album_id, const QString& cover_id) {
    album_page_->album()->hideWidget();
    album_page_->album()->albumViewPage()->setPlaylistMusic(album, album_id, cover_id);
    ui_.currentView->setCurrentWidget(album_page_);
}

void Xamp::setPlaylistPageCover(const QPixmap* cover, PlaylistPage* page) {
    if (!cover) {
        cover = &qTheme.unknownCover();
    }

	if (!page) {
        page = current_playlist_page_;
	}

    const auto ui_cover = Pixmap::roundImage(
        Pixmap::scaledImage(*cover, ui_.coverLabel->size(), false),
        Pixmap::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);

    page->setCover(cover);

    if (lrc_page_ != nullptr) {
        lrc_page_->setCover(Pixmap::scaledImage(*cover, lrc_page_->cover()->size(), true));
    }   
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
    if (!player_) {
        return;
    }
    if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
        top_window_->resetTaskbarProgress();
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(streamTimeToString(0));
        playNextItem(1);
        payNextMusic();
    }
}

void Xamp::initialPlaylist() {
    lrc_page_ = new LrcPage(this);
    album_page_ = new AlbumArtistPage(this);
    artist_info_page_ = new ArtistInfoPage(this);
    preference_page_ = new PreferencePage(this);
    about_page_ = new AboutPage(this);

    ui_.sliderBar->addTab(tr("Playlists"), TAB_PLAYLIST, qTheme.iconFromFont(Glyphs::ICON_PLAYLIST));
    ui_.sliderBar->addTab(tr("Podcast"), TAB_PODCAST, qTheme.iconFromFont(Glyphs::ICON_PODCAST));
    ui_.sliderBar->addTab(tr("File Explorer"), TAB_FILE_EXPLORER, qTheme.iconFromFont(Glyphs::ICON_DESKTOP));
    ui_.sliderBar->addTab(tr("Albums"), TAB_ALBUM, qTheme.iconFromFont(Glyphs::ICON_ALBUM));
    ui_.sliderBar->addTab(tr("Artists"), TAB_ARTIST, qTheme.iconFromFont(Glyphs::ICON_ARTIST));
    ui_.sliderBar->addTab(tr("Lyrics"), TAB_LYRICS, qTheme.iconFromFont(Glyphs::ICON_SUBTITLE));
    ui_.sliderBar->addTab(tr("Settings"), TAB_SETTINGS, qTheme.iconFromFont(Glyphs::ICON_PREFERENCE));
    ui_.sliderBar->addTab(tr("CD"), TAB_CD, qTheme.iconFromFont(Glyphs::ICON_CD));
    ui_.sliderBar->addTab(tr("About"), TAB_ABOUT, qTheme.iconFromFont(Glyphs::ICON_ABOUT));
    ui_.sliderBar->setCurrentIndex(ui_.sliderBar->model()->index(0, 0));

    qDatabase.forEachTable([this](auto table_id,
        auto /*table_index*/,
        auto playlist_id,
        const auto& name) {
            if (name.isEmpty()) {
                return;
            }

            ui_.sliderBar->addTab(name, table_id, qTheme.iconFromFont(Glyphs::ICON_PLAYLIST));

            if (!playlist_page_) {
                playlist_page_ = newPlaylistPage(playlist_id, qEmptyString);
                playlist_page_->playlist()->setPlaylistId(playlist_id, name);
            }

            if (playlist_page_->playlist()->playlistId() != playlist_id) {
                playlist_page_ = newPlaylistPage(playlist_id, qEmptyString);
                playlist_page_->playlist()->setPlaylistId(playlist_id, name);
            }
        });

    if (!playlist_page_) {
        auto playlist_id = kDefaultPlaylistId;
        if (!qDatabase.isPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.addPlaylist(qEmptyString, 0);
        }
        playlist_page_ = newPlaylistPage(kDefaultPlaylistId, kAppSettingPlaylistColumnName);
        connectPlaylistPageSignal(playlist_page_);
    }

    if (!podcast_page_) {
        auto playlist_id = kDefaultPodcastPlaylistId;
        if (!qDatabase.isPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.addPlaylist(qEmptyString, 1);
        }
        podcast_page_ = newPlaylistPage(playlist_id, kAppSettingPodcastPlaylistColumnName);
        podcast_page_->playlist()->setPodcastMode();
        connectPlaylistPageSignal(podcast_page_);
    }

    if (!file_system_view_page_) {
        file_system_view_page_ = new FileSystemViewPage(this);
        auto playlist_id = kDefaultFileExplorerPlaylistId;
        if (!qDatabase.isPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.addPlaylist(qEmptyString, 2);
        }
        file_system_view_page_->playlistPage()->playlist()->setPlaylistId(playlist_id, kAppSettingFileSystemPlaylistColumnName);
        setCover(qEmptyString, file_system_view_page_->playlistPage());
        connectPlaylistPageSignal(file_system_view_page_->playlistPage());
    }

    if (!cd_page_) {
        auto playlist_id = kDefaultCdPlaylistId;
        if (!qDatabase.isPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.addPlaylist(qEmptyString, 4);
        }
        cd_page_ = new CdPage(this);
        cd_page_->playlistPage()->playlist()->setPlaylistId(playlist_id, kAppSettingCdPlaylistColumnName);
        setCover(qEmptyString, cd_page_->playlistPage());
        connectPlaylistPageSignal(cd_page_->playlistPage());
    }

    current_playlist_page_ = playlist_page_;

    (void)QObject::connect(this,
        &Xamp::addBlurImage,
        background_worker_,
        &BackgroundWorker::onBlurImage);

    (void)QObject::connect(this,
        &Xamp::fetchCdInfo,
        background_worker_,
        &BackgroundWorker::onFetchCdInfo);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::updateCdMetadata,
        this,
        &Xamp::onUpdateCdMetadata,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::updateMbDiscInfo,
        this,
        &Xamp::onUpdateMbDiscInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_,
		&BackgroundWorker::updateDiscCover,
        this,
        &Xamp::onUpdateDiscCover,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_view_page_,
        &FileSystemViewPage::addDirToPlyalist,
        this,
        &Xamp::appendToPlaylist);    

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        album_page_,
        &AlbumArtistPage::onThemeChanged);

    if (!qDatabase.isPlaylistExist(kDefaultAlbumPlaylistId)) {
        qDatabase.addPlaylist(qEmptyString, 1);
    }
    connectPlaylistPageSignal(album_page_->album()->albumViewPage()->playlistPage());

    pushWidget(lrc_page_);
    pushWidget(playlist_page_);
    pushWidget(album_page_);
    pushWidget(artist_info_page_);
    pushWidget(podcast_page_);
    pushWidget(file_system_view_page_);
    pushWidget(preference_page_);
    pushWidget(cd_page_);
    pushWidget(about_page_);
    ui_.currentView->setCurrentIndex(0);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::clickedArtist,
        this,
        &Xamp::onArtistIdChanged);

    (void)QObject::connect(artist_info_page_->album(),
        &AlbumView::clickedAlbum,
        this,
        &Xamp::onClickedAlbum);

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        album_page_->album(),
        &AlbumView::onThemeChanged);

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        artist_info_page_,
        &ArtistInfoPage::onThemeChanged);

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        lrc_page_,
        &LrcPage::onThemeChanged);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::updateBlurImage,
        lrc_page_,
        &LrcPage::setBackground);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::addPlaylist,
        this,
        &Xamp::addPlaylistItem);

    (void)QObject::connect(artist_info_page_->album(),
        &AlbumView::addPlaylist,
        this,
        &Xamp::addPlaylistItem);
}

void Xamp::appendToPlaylist(const QString& file_name) {
    try {
        playlist_page_->playlist()->append(file_name, false, false);
        album_page_->refreshOnece();
    }
    catch (const Exception& e) {
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
    }
}

void Xamp::addItem(const QString& file_name) {
	const auto add_playlist = dynamic_cast<PlaylistPage*>(
        ui_.currentView->currentWidget()) != nullptr;

    if (add_playlist) {
        appendToPlaylist(file_name);
    }
    else {
        extractFile(file_name);
    }
}

void Xamp::pushWidget(QWidget* widget) {
	const auto id = ui_.currentView->addWidget(widget);
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
	    const auto id = stack_page_id_.pop();
        auto* widget = ui_.currentView->widget(id);
        ui_.currentView->removeWidget(widget);
        if (!stack_page_id_.isEmpty()) {
            ui_.currentView->setCurrentIndex(stack_page_id_.top());
            return widget;
        }
    }
    return nullptr;
}

void Xamp::encodeAACFile(const PlayListEntity& item, const EncodingProfile& profile) {
    auto last_dir = AppSettings::getValueAsString(kDefaultDir);

    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    const auto file_name = QFileDialog::getSaveFileName(this,
        tr("Save AAC file"),
        save_file_name,
        tr("AAC Files (*.m4a)"));

    if (file_name.isNull()) {
        return;
    }

    QDir current_dir;
    AppSettings::setValue(kDefaultDir, current_dir.absoluteFilePath(file_name));

    const auto dialog = makeProgressDialog(
        tr("Export progress dialog"),
        tr("Export '") + item.title + tr("' to aac file"),
        tr("Cancel"));

    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));

    TrackInfo metadata;
    metadata.album = item.album.toStdWString();
    metadata.artist = item.artist.toStdWString();
    metadata.title = item.title.toStdWString();
    metadata.track = item.track;

    try {
        auto encoder = StreamFactory::MakeAACEncoder();
#ifdef XAMP_OS_WIN
        auto *mf_aac_encode = dynamic_cast<MFAACFileEncoder*>(encoder.get());
        mf_aac_encode->SetEncodingProfile(profile);
#else
        auto *bass_aac_encoder = dynamic_cast<BassAACFileEncoder*>(encoder.get());
        bass_aac_encoder->SetEncodingProfile(profile);
#endif

        read_utiltis::encodeFile(item.file_path.toStdWString(),
            file_name.toStdWString(),
            encoder,
            L"",
            [&](auto progress) -> bool {
                dialog->setValue(progress);
                qApp->processEvents();
                return dialog->wasCanceled() != true;
            }, metadata);
    }
    catch (Exception const& e) {
        XMessageBox::showError(qTEXT(e.what()));
    }
}

void Xamp::encodeWavFile(const PlayListEntity& item) {
    auto last_dir = AppSettings::getValueAsString(kDefaultDir);

    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    const auto file_name = QFileDialog::getSaveFileName(this,
        tr("Save Wav file"),
        save_file_name,
        tr("Wav Files (*.wav)"));

    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));

    if (file_name.isNull()) {
        return;
    }

    QDir current_dir;
    AppSettings::setValue(kDefaultDir, current_dir.absoluteFilePath(file_name));

    const auto dialog = makeProgressDialog(
        tr("Export progress dialog"),
        tr("Export '") + item.title + tr("' to wav file"),
        tr("Cancel"));

    TrackInfo metadata;
    metadata.album = item.album.toStdWString();
    metadata.artist = item.artist.toStdWString();
    metadata.title = item.title.toStdWString();
    metadata.track = item.track;

    std::wstring command;

    try {
        auto encoder = StreamFactory::MakeWaveEncoder();
        read_utiltis::encodeFile(item.file_path.toStdWString(),
            file_name.toStdWString(),
            encoder,
            command,
            [&](auto progress) -> bool {
                dialog->setValue(progress);
                qApp->processEvents();
                return dialog->wasCanceled() != true;
            }, metadata);
    }
    catch (Exception const& e) {
        XMessageBox::showError(qTEXT(e.what()));
    }
}

void Xamp::encodeFlacFile(const PlayListEntity& item) {
    auto last_dir = AppSettings::getValueAsString(kDefaultDir);

    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    const auto file_name = QFileDialog::getSaveFileName(this,
        tr("Save Flac file"),
        save_file_name,
        tr("FLAC Files (*.flac)"));

    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));

    if (file_name.isNull()) {
        return;
    }

    QDir current_dir;
    AppSettings::setValue(kDefaultDir, current_dir.absoluteFilePath(file_name));

    const auto dialog = makeProgressDialog(
        tr("Export progress dialog"),
         tr("Export '") + item.title + tr("' to flac file"),
         tr("Cancel"));

    TrackInfo metadata;
    metadata.album = item.album.toStdWString();
    metadata.artist = item.artist.toStdWString();
    metadata.title = item.title.toStdWString();
    metadata.track = item.track;

    const auto command
    	= qSTR("-%1 -V").arg(AppSettings::getValue(kFlacEncodingLevel).toInt()).toStdWString();

    try {
        auto encoder = StreamFactory::MakeFlacEncoder();
        read_utiltis::encodeFile(item.file_path.toStdWString(),
            file_name.toStdWString(),
            encoder,
            command,
            [&](auto progress) -> bool {
                dialog->setValue(progress);
                qApp->processEvents();
                return dialog->wasCanceled() != true;
            }, metadata);
    }
    catch (Exception const& e) {
        XMessageBox::showError(qTEXT(e.what()));
    }
}

void Xamp::connectPlaylistPageSignal(PlaylistPage* playlist_page) {
    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::addPlaylistItemFinished,
        album_page_,
        &AlbumArtistPage::refreshOnece);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::playMusic,
        playlist_page,
        &PlaylistPage::playMusic);

    (void)QObject::connect(playlist_page,
        &PlaylistPage::playMusic,
        this,
        &Xamp::playPlayListEntity);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::encodeFlacFile,
        this,
        &Xamp::encodeFlacFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::encodeAACFile,
        this,
        &Xamp::encodeAACFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::encodeWavFile,
        this,
        &Xamp::encodeWavFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::readReplayGain,
        background_worker_,
        &BackgroundWorker::onReadReplayGain);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::updateReplayGain,
        playlist_page->playlist(),
        &PlayListTableView::updateReplayGain,
        Qt::QueuedConnection);

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        playlist_page->playlist(),
        &PlayListTableView::onThemeColorChanged);

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        playlist_page,
        &PlaylistPage::OnThemeColorChanged);
}

PlaylistPage* Xamp::newPlaylistPage(int32_t playlist_id, const QString& column_setting_name) {
	auto* playlist_page = new PlaylistPage(this);
    ui_.currentView->addWidget(playlist_page);    
    playlist_page->playlist()->setPlaylistId(playlist_id, column_setting_name);
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
    MetadataExtractAdapter::readFileMetadata(adapter, file_path, false);
}

void Xamp::setTipHint(QWidget* widget, const QString& hint_text) {
    auto *tooltip = new ToolTips(qEmptyString, parentWidget());
    tooltip->hide();
    tooltip->setText(hint_text);
    tooltip->setFixedHeight(32);

    widget->setProperty("ToolTip", QVariant::fromValue<QWidget*>(tooltip));
    widget->installEventFilter(tool_tips_filter_);
}
