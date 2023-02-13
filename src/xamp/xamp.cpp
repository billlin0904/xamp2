#include <QCloseEvent>
#include <QDebug>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QInputDialog>
#include <QProcess>
#include <QShortcut>
#include <QtMath>
#include <QToolTip>
#include <QWidgetAction>

#include <base/logger_impl.h>
#include <base/scopeguard.h>
#include <base/str_utilts.h>

#include <stream/api.h>
#include <stream/bassaacfileencoder.h>
#include <stream/dsd_utils.h>
#include <stream/idspmanager.h>
#include <stream/pcm2dsdsamplewriter.h>
#include <stream/r8brainresampler.h>

#include <output_device/api.h>
#include <output_device/iaudiodevicemanager.h>

#include <player/api.h>

#include <widget/actionmap.h>
#include <widget/albumartistpage.h>
#include <widget/albumview.h>
#include <widget/appsettings.h>
#include <widget/artistinfopage.h>
#include <widget/backgroundworker.h>
#include <widget/database.h>
#include <widget/equalizerdialog.h>
#include <widget/filesystemviewpage.h>
#include <widget/http.h>
#include <widget/image_utiltis.h>
#include <widget/jsonsettings.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/pixmapcache.h>
#include <widget/playlistpage.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlisttableview.h>
#include <widget/podcast_uiltis.h>
#include <widget/read_utiltis.h>
#include <widget/spectrumwidget.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/volumecontroldialog.h>
#include <widget/xdialog.h>
#include <widget/xmessagebox.h>
#include <widget/xprogressdialog.h>

#if defined(Q_OS_WIN)
#include <stream/mfaacencoder.h>
#endif

#include "xamp.h"
#include "aboutpage.h"
#include "cdpage.h"
#include "preferencepage.h"
#include "thememanager.h"
#include "version.h"

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
	, preference_page_(nullptr)
	, file_system_view_page_(nullptr)
	, about_page_(nullptr)
	, main_window_(nullptr)
	, background_worker_(nullptr)
    , state_adapter_(std::make_shared<UIPlayerStateAdapter>())
	, player_(player) {
    ui_.setupUi(this);
}

Xamp::~Xamp() = default;

void Xamp::SetXWindow(IXMainWindow* main_window) {
    main_window_ = main_window;
    background_worker_ = new BackgroundWorker();  
    background_worker_->moveToThread(&background_thread_);
    background_thread_.start(QThread::LowestPriority);
    player_->Startup(state_adapter_);

    InitialUi();
    InitialController();
    InitialPlaylist();
    InitialShortcut();
    InitialSpectrum();

    SetPlaylistPageCover(nullptr, playlist_page_);
    SetPlaylistPageCover(nullptr, podcast_page_);  
    SetPlaylistPageCover(nullptr, cd_page_->playlistPage());
    SetPlaylistPageCover(nullptr, file_system_view_page_->playlistPage());

    playlist_page_->HidePlaybackInformation(true);

    podcast_page_->HidePlaybackInformation(true);
    cd_page_->playlistPage()->HidePlaybackInformation(true);
    file_system_view_page_->playlistPage()->HidePlaybackInformation(false);
    album_page_->album()->albumViewPage()->playlistPage()->HidePlaybackInformation(false);
    
    AvoidRedrawOnResize();

    qTheme.SetPlayOrPauseButton(ui_, false);       

    const auto tab_name = AppSettings::ValueAsString(kAppSettingLastTabName);
    const auto tab_id = ui_.sliderBar->GetTabId(tab_name);
    if (tab_id != -1) {
        ui_.sliderBar->setCurrentIndex(ui_.sliderBar->model()->index(tab_id, 0));
        SetCurrentTab(tab_id);
    }

    QTimer::singleShot(300, [this]() {
        InitialDeviceList();
        });

    (void)QObject::connect(&qTheme, 
        &ThemeManager::CurrentThemeChanged, 
        this, 
        &Xamp::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        ui_.sliderBar,
        &TabListView::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        album_page_->album(),
        &AlbumView::OnCurrentThemeChanged);
}

void Xamp::AvoidRedrawOnResize() {
    ui_.coverLabel->setAttribute(Qt::WA_StaticContents);
}

void Xamp::OnActivated(QSystemTrayIcon::ActivationReason reason) {
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

void Xamp::InitialSpectrum() {
    if (!AppSettings::ValueAsBool(kAppSettingEnableSpectrum)) {
        return;
    }

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::fftResultChanged,
        lrc_page_->spectrum(),
        &SpectrumWidget::onFFTResultChanged,
        Qt::QueuedConnection);

    lrc_page_->spectrum()->setStyle(AppSettings::ValueAsEnum<SpectrumStyles>(kAppSettingSpectrumStyles));
}

void Xamp::UpdateMaximumState(bool is_maximum) {
    qTheme.UpdateMaximumIcon(ui_, is_maximum);
}

void Xamp::FocusIn() {
}

void Xamp::FocusOut() {
}

void Xamp::closeEvent(QCloseEvent*) {
    cleanup();
    window()->close();
}

void Xamp::cleanup() {
    if (player_ != nullptr) {
        player_->Destroy();
        player_.reset();
        XAMP_LOG_DEBUG("Player destroy!");
    }

    if (!background_thread_.isFinished()) {
        if (background_worker_ != nullptr) {
            background_worker_->stopThreadPool();
        }
        background_thread_.requestInterruption();
        background_thread_.quit();
        background_thread_.wait();
        XAMP_LOG_DEBUG("background_thread stop!");
    }

    if (main_window_ != nullptr) {
        main_window_->SaveGeometry();
    }

    XAMP_LOG_DEBUG("Xamp cleanup!");
}

void Xamp::InitialUi() {
    QFont f(qTEXT("DisplayFont"));
    f.setWeight(QFont::DemiBold);
    f.setPointSize(qTheme.GetFontSize());
    ui_.titleLabel->setFont(f);

    f.setWeight(QFont::Normal);
    f.setPointSize(qTheme.GetFontSize());
    ui_.artistLabel->setFont(f);

    ui_.bitPerfectButton->setFont(f);

    QToolTip::setFont(QFont(qTEXT("FormatFont")));
    ui_.formatLabel->setFont(QFont(qTEXT("FormatFont")));

    ui_.formatLabel->setStyleSheet(qTEXT("background-color: transparent"));
    ui_.hiResLabel->setStyleSheet(qTEXT("background-color: transparent"));

    if (qTheme.UseNativeWindow()) {
        ui_.closeButton->hide();
        ui_.maxWinButton->hide();
        ui_.minWinButton->hide();
        ui_.horizontalLayout->removeItem(ui_.horizontalSpacer_15);        
    } else {
        f.setWeight(QFont::DemiBold);
        f.setPointSize(qTheme.GetFontSize());
        ui_.titleFrameLabel->setFont(f);
        ui_.titleFrameLabel->setText(kApplicationTitle);
        ui_.titleFrameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    QFont mono_font(qTEXT("MonoFont"));
#ifdef Q_OS_WIN
    mono_font.setPointSize(qTheme.GetFontSize());
    ui_.startPosLabel->setFont(mono_font);
    ui_.endPosLabel->setFont(mono_font);
#else
    f.setPointSize(9);
    ui_.startPosLabel->setFont(mono_font);
    ui_.endPosLabel->setFont(mono_font);
#endif

    ui_.searchLineEdit->addAction(qTheme.GetFontIcon(Glyphs::ICON_SEARCH), QLineEdit::LeadingPosition);
    main_window_->SetTitleBarAction(ui_.titleFrame);
}

void Xamp::OnVolumeChanged(float volume) {
    SetVolume(static_cast<int32_t>(volume * 100.0f));
}

void Xamp::OnDeviceStateChanged(DeviceState state) {
    if (state == DeviceState::DEVICE_STATE_REMOVED) {
        player_->Stop(true, true, true);
    }
    if (state == DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE) {
        return;
    }
    InitialDeviceList();
}

QWidgetAction* Xamp::CreateDeviceMenuWidget(const QString& desc, const QIcon &icon) {
    auto* desc_label = new QLabel(desc);

    desc_label->setObjectName(qTEXT("textSeparator"));

    QFont f(qTEXT("DisplayFont"));
    f.setPointSize(qTheme.GetFontSize());
    f.setBold(true);
    desc_label->setFont(f);

    auto* device_type_frame = new QFrame();
    qTheme.SetTextSeparator(device_type_frame);
    auto* default_layout = new QHBoxLayout(device_type_frame);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 0, 0, 0);

    if (!icon.isNull()) {
        auto* icon_button = new QToolButton();
        icon_button->setFixedSize(QSize(12, 12));
        icon_button->setIconSize(QSize(12, 12));
        icon_button->setIcon(icon);
        default_layout->addWidget(icon_button);
    }
   
    default_layout->addWidget(desc_label);
    device_type_frame->setLayout(default_layout);

    auto* separator = new QWidgetAction(this);
    separator->setDefaultWidget(device_type_frame);

    device_type_frame_.push_back(device_type_frame);
    return separator;
}

void Xamp::InitialDeviceList() {    
    auto* menu = ui_.selectDeviceButton->menu();
    if (!menu) {
        menu = new XMenu();
        qTheme.SetMenuStyle(menu);
        ui_.selectDeviceButton->setMenu(menu);
    }

    menu->clear();

    DeviceInfo init_device_info;
    auto is_find_setting_device = false;

    auto* device_action_group = new QActionGroup(this);
    device_action_group->setExclusive(true);

    OrderedMap<std::string, QAction*> device_id_action;

    const auto device_type_id = AppSettings::ValueAsID(kAppSettingDeviceType);
    const auto device_id = AppSettings::ValueAsString(kAppSettingDeviceId).toStdString();
    const auto & device_manager = player_->GetAudioDeviceManager();

    for (auto itr = device_manager->Begin(); itr != device_manager->End(); ++itr) {
	    const auto device_type = (*itr).second();
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            return;
        }

        menu->addSeparator();
        menu->addAction(CreateDeviceMenuWidget(FromStdStringView(device_type->GetDescription())));

        for (const auto& device_info : device_info_list) {
            auto* device_action = new XAction(qTheme.GetConnectTypeGlyphs(device_info.connect_type),
                QString::fromStdWString(device_info.name), 
                this);

            (void)QObject::connect(&qTheme,
                &ThemeManager::CurrentThemeChanged,
                device_action,
                &XAction::OnCurrentThemeChanged);

            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_action->setChecked(false);
            device_id_action[device_info.device_id] = device_action;            

            auto trigger_callback = [device_info, this]() {
                qTheme.SetDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info.connect_type);
                device_info_ = device_info;
                AppSettings::SetValue(kAppSettingDeviceType, device_info_.device_type_id);
                AppSettings::SetValue(kAppSettingDeviceId, device_info_.device_id);
            };

            (void)QObject::connect(device_action, &QAction::triggered, trigger_callback);
            menu->addAction(device_action);

            if (device_type_id == device_info.device_type_id && device_id == device_info.device_id) {
                device_info_ = device_info;
                is_find_setting_device = true;
                device_action->setChecked(true);
                qTheme.SetDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info.connect_type);
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
        qTheme.SetDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info_.connect_type);
        device_id_action[device_info_.device_id]->setChecked(true);
        AppSettings::SetValue(kAppSettingDeviceType, device_info_.device_type_id);
        AppSettings::SetValue(kAppSettingDeviceId, device_info_.device_id);
        XAMP_LOG_DEBUG("Use default device Id : {}", device_info_.device_id);
    }
}

void Xamp::SliderAnimation(bool enable) {
    auto* animation = new QPropertyAnimation(ui_.sliderFrame, "geometry");
    const auto slider_geometry = ui_.sliderFrame->geometry();
    constexpr auto kMaxSliderWidth = 200;
    constexpr auto kMinSliderWidth = 43;
    QSize size;
    if (!enable) {
        ui_.searchFrame->hide();
        ui_.tableLabel->hide();
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->setStartValue(QRect(slider_geometry.x(), slider_geometry.y(), kMaxSliderWidth, slider_geometry.height()));
        animation->setEndValue(QRect(slider_geometry.x(), slider_geometry.y(), kMinSliderWidth, slider_geometry.height()));
        size = QSize(kMinSliderWidth, slider_geometry.height());
    }
    else {
        animation->setEasingCurve(QEasingCurve::OutCubic);
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

void Xamp::InitialController() {
    (void)QObject::connect(ui_.minWinButton, &QToolButton::pressed, [this]() {
        main_window_->showMinimized();
    });

    (void)QObject::connect(ui_.maxWinButton, &QToolButton::pressed, [this]() {
        main_window_->UpdateMaximumState();
    });

    (void)QObject::connect(ui_.mutedButton, &QToolButton::pressed, [this]() {
        VolumeControlDialog vc(player_, this);
        MoveToTopWidget(&vc, ui_.mutedButton);
        vc.exec();
    });

    (void)QObject::connect(ui_.closeButton, &QToolButton::pressed, [this]() {
        QWidget::close();
    });

    qTheme.SetBitPerfectButton(ui_, AppSettings::ValueAsBool(kEnableBitPerfect));

    (void)QObject::connect(ui_.bitPerfectButton, &QToolButton::pressed, [this]() {
	    const auto enable_or_disable = !AppSettings::ValueAsBool(kEnableBitPerfect);
        AppSettings::SetValue(kEnableBitPerfect, enable_or_disable);
        qTheme.SetBitPerfectButton(ui_, enable_or_disable);
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::LeftButtonValueChanged, [this](auto value) {
        try {
			player_->Seek(value / 1000.0);
            qTheme.SetPlayOrPauseButton(ui_, true);
            main_window_->SetTaskbarPlayingResume();
        }
        catch (const Exception & e) {
            player_->Stop(false);
            XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
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

    order_ = AppSettings::ValueAsEnum<PlayerOrder>(kAppSettingOrder);
    SetPlayerOrder();

    if (AppSettings::ValueAsBool(kAppSettingIsMuted)) {
        qTheme.SetMuted(ui_.mutedButton, true);
    }
    else {
        qTheme.SetMuted(ui_.mutedButton, false);
    }

    (void)QObject::connect(state_adapter_.get(),
                            &UIPlayerStateAdapter::stateChanged,
                            this,
                            &Xamp::OnPlayerStateChanged,
                            Qt::QueuedConnection);

    (void)QObject::connect(state_adapter_.get(),
                            &UIPlayerStateAdapter::sampleTimeChanged,
                            this,
                            &Xamp::OnSampleTimeChanged,
                            Qt::QueuedConnection);

    (void)QObject::connect(state_adapter_.get(),
                            &UIPlayerStateAdapter::deviceChanged,
                            this,
                            &Xamp::OnDeviceStateChanged,
                            Qt::QueuedConnection);

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::volumeChanged,
        this,
        &Xamp::OnVolumeChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(ui_.searchLineEdit, &QLineEdit::textChanged, [this](const auto &text) {
        emit album_page_->album()->OnSearchTextChanged(text);
    });

    (void)QObject::connect(ui_.nextButton, &QToolButton::pressed, [this]() {
        PlayNext();
    });

    (void)QObject::connect(ui_.prevButton, &QToolButton::pressed, [this]() {
        PlayPrevious();
    });

    (void)QObject::connect(ui_.eqButton, &QToolButton::pressed, [this]() {
        auto* dialog = new XDialog(this);
        auto* eq = new EqualizerDialog(dialog);
        dialog->SetContentWidget(eq);
        dialog->SetTitle(tr("Equalizer"));

        (void)QObject::connect(eq, &EqualizerDialog::BandValueChange, [](auto, auto, auto) {
            AppSettings::save();
        });

        (void)QObject::connect(eq, &EqualizerDialog::PreampValueChange, [](auto) {
            AppSettings::save();
        });

        QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
        dialog->exec();
    });

    (void)QObject::connect(ui_.repeatButton, &QToolButton::pressed, [this]() {
        order_ = GetNextOrder(order_);
        SetPlayerOrder();
    });

    (void)QObject::connect(ui_.playButton, &QToolButton::pressed, [this]() {
        PlayOrPause();
    });

    (void)QObject::connect(ui_.stopButton, &QToolButton::pressed, [this]() {
        stop();
    });

    (void)QObject::connect(ui_.artistLabel, &ClickableLabel::clicked, [this]() {
        OnArtistIdChanged(current_entity_.artist, current_entity_.cover_id, current_entity_.artist_id);
    });

    (void)QObject::connect(ui_.coverLabel, &ClickableLabel::clicked, [this]() {
        ui_.currentView->setCurrentWidget(lrc_page_);
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::ClickedTable, [this](auto table_id) {
        SetCurrentTab(table_id);
        AppSettings::SetValue(kAppSettingLastTabName, ui_.sliderBar->GetTabName(table_id));
    });

    (void)QObject::connect(ui_.sliderBar, &TabListView::TableNameChanged, [](auto table_id, const auto &name) {
        qDatabase.SetTableName(table_id, name);
    });

    QTimer::singleShot(500, [this]() {
        SliderAnimation(AppSettings::ValueAsBool(kAppSettingShowLeftList));
        });

    ui_.sliderBarButton->setIconSize(qTheme.GetTabIconSize());
   (void)QObject::connect(ui_.sliderBarButton, &QToolButton::clicked, [this]() {
	   const auto enable = !AppSettings::ValueAsBool(kAppSettingShowLeftList);
       AppSettings::SetValue(kAppSettingShowLeftList, enable);
       SliderAnimation(enable);
        });   

    auto* check_for_update = new QAction(tr("Check For Updates"), this);

#if 0
    static const QString kSoftwareUpdateUrl =
        qTEXT("https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json");
    auto* updater = QSimpleUpdater::getInstance();

    if (AppSettings::ValueAsBool(kAppSettingAutoCheckForUpdate)) {
        (void)QObject::connect(updater, &QSimpleUpdater::checkingFinished, [updater, this](auto url) {
            auto change_log = updater->getChangelog(url);

            auto html = qTEXT(R"(
            <h3>Find New Version:</h3> 			d
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

    ui_.seekSlider->setEnabled(false);
    ui_.startPosLabel->setText(FormatDuration(0));
    ui_.endPosLabel->setText(FormatDuration(0));
    ui_.searchLineEdit->setPlaceholderText(tr("Search anything"));
}

void Xamp::SetCurrentTab(int32_t table_id) {
    switch (table_id) {
    case TAB_ALBUM:
        album_page_->Refresh();
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

void Xamp::UpdateButtonState() {    
    qTheme.SetPlayOrPauseButton(ui_, player_->GetState() != PlayerState::PLAYER_STATE_PAUSED);
    preference_page_->SaveSettings();
}

void Xamp::OnCurrentThemeChanged(ThemeColor theme_color) {
	switch (theme_color) {
	case ThemeColor::DARK_THEME:
        qTheme.SetThemeColor(ThemeColor::DARK_THEME);		
        break;
    case ThemeColor::LIGHT_THEME:
        qTheme.SetThemeColor(ThemeColor::LIGHT_THEME);
        break;
	}
    Q_FOREACH(QFrame *frame, device_type_frame_) {
        qTheme.SetTextSeparator(frame);
	}
    if (AppSettings::ValueAsBool(kAppSettingIsMuted)) {
        qTheme.SetMuted(ui_.mutedButton, true);
    }
    else {
        qTheme.SetMuted(ui_.mutedButton, false);
    }
    qTheme.LoadAndApplyQssTheme();
    SetThemeColor(qTheme.palette().color(QPalette::WindowText), qTheme.GetThemeTextColor());
}

void Xamp::SetThemeColor(QColor backgroundColor, QColor color) {
    qTheme.SetBackgroundColor(ui_, backgroundColor);
    qTheme.SetWidgetStyle(ui_);
    UpdateButtonState();
    emit ThemeChanged(backgroundColor, color);
}

void Xamp::OnSearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics) {
    lrc_page_->lyrics()->SetLrc(lyrics, trlyrics);
    qDatabase.AddOrUpdateLyrc(music_id, lyrics, trlyrics);
}

void Xamp::ShortcutsPressed(const QKeySequence& shortcut) {
    XAMP_LOG_DEBUG("shortcutsPressed: {}", shortcut.toString().toStdString());

	const QMap<QKeySequence, std::function<void()>> shortcut_map {
        { QKeySequence(Qt::Key_MediaPlay), [this]() {
            PlayOrPause();
            }},
         { QKeySequence(Qt::Key_MediaStop), [this]() {
            stop();
            }},
        { QKeySequence(Qt::Key_MediaPrevious), [this]() {
            PlayPrevious();
            }},
        { QKeySequence(Qt::Key_MediaNext), [this]() {
            PlayNext();
            }},
        { QKeySequence(Qt::Key_VolumeUp), [this]() {
            SetVolume(player_->GetVolume() + 1); 
            }},
        { QKeySequence(Qt::Key_VolumeDown), [this]() {
            SetVolume(player_->GetVolume() - 1);
            }},
        { QKeySequence(Qt::Key_VolumeMute), [this]() {
            SetVolume(0);
            }},
    };

    auto key = shortcut_map.value(shortcut);
    if (key != nullptr) {
        key();
    }
}

void Xamp::GetNextPage() {
    if (stack_page_id_.isEmpty()) {
        return;
    }
    const auto idx = ui_.currentView->currentIndex();
    ui_.currentView->setCurrentIndex(idx + 1);
}

void Xamp::SetTablePlaylistView(int table_id, ConstLatin1String column_setting_name) {
	const auto playlist_id = qDatabase.FindTablePlaylistId(table_id);

    auto found = false;
    Q_FOREACH(auto idx, stack_page_id_) {
        if (auto* page = dynamic_cast<PlaylistPage*>(ui_.currentView->widget(idx))) {
            if (page->playlist()->GetPlaylistId() == playlist_id) {
                ui_.currentView->setCurrentIndex(idx);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        auto* playlist_page = NewPlaylistPage(playlist_id, column_setting_name);
        PushWidget(playlist_page);
    }
}

void Xamp::GoBackPage() {
    if (stack_page_id_.isEmpty()) {
        return;
    }
    const auto idx = ui_.currentView->currentIndex();
    ui_.currentView->setCurrentIndex(idx - 1);
}

void Xamp::SetVolume(uint32_t volume) {
    AppSettings::SetValue(kAppSettingVolume, volume);
}

void Xamp::InitialShortcut() {
    const auto* space_key = new QShortcut(QKeySequence(Qt::Key_Space), this);
    (void)QObject::connect(space_key, &QShortcut::activated, [this]() {
        PlayOrPause();
    });

    const auto* play_key = new QShortcut(QKeySequence(Qt::Key_F4), this);
    (void)QObject::connect(play_key, &QShortcut::activated, [this]() {
        PlayNextItem(1);
        });
}

bool Xamp::HitTitleBar(const QPoint& ps) const {
    return ui_.titleFrame->rect().contains(ps);
}

void Xamp::stop() {
    player_->Stop(true, true);
    SetSeekPosValue(0);
    lrc_page_->spectrum()->reset();
    ui_.seekSlider->setEnabled(false);
    playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_CLEAR);
    album_page_->album()->albumViewPage()->playlistPage()->playlist()->SetNowPlayState(PlayingState::PLAY_CLEAR);
    qTheme.SetPlayOrPauseButton(ui_, false);
}

void Xamp::PlayNext() {
    PlayNextItem(1);
}

void Xamp::PlayPrevious() {
    PlayNextItem(-1);
}

void Xamp::DeleteKeyPress() {
    if (!ui_.currentView->count()) {
        return;
    }
    auto* playlist_view = playlist_page_->playlist();
    playlist_view->RemoveSelectItems();
}

void Xamp::SetPlayerOrder() {
	const auto order = AppSettings::ValueAsEnum<PlayerOrder>(kAppSettingOrder);

    switch (order_) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        if (order_ != order) {
            XMessageBox::ShowInformation(tr("Repeat once"), kApplicationTitle, false);
        }
        qTheme.SetRepeatOncePlayOrder(ui_);
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        if (order_ != order) {
            XMessageBox::ShowInformation(tr("Repeat one"), kApplicationTitle, false);
        }
        qTheme.SetRepeatOnePlayOrder(ui_);
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        if (order_ != order) {
            XMessageBox::ShowInformation(tr("Shuffle all"), kApplicationTitle, false);
        }
        qTheme.SetShufflePlayOrder(ui_);
        break;
    default:
        break;
    }
    AppSettings::setEnumValue(kAppSettingOrder, order_);
}

void Xamp::OnSampleTimeChanged(double stream_time) {
    if (!player_ || is_seeking_) {
        return;
    }
    if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {         
        SetSeekPosValue(stream_time);
    }
}

void Xamp::SetSeekPosValue(double stream_time) {
    const auto full_text = IsMoreThan1Hours(player_->GetDuration());
    ui_.endPosLabel->setText(FormatDuration(player_->GetDuration() - stream_time, full_text));
    const auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
    ui_.seekSlider->setValue(stream_time_as_ms);
    ui_.startPosLabel->setText(FormatDuration(stream_time, full_text));
    main_window_->SetTaskbarProgress(static_cast<int32_t>(100.0 * ui_.seekSlider->value() / ui_.seekSlider->maximum()));
    lrc_page_->lyrics()->SetLrcTime(stream_time_as_ms);
}

void Xamp::PlayLocalFile(const PlayListEntity& item) {
    main_window_->SetTaskbarPlayerPlaying();
    PlayPlayListEntity(item);
}

void Xamp::PlayOrPause() {
    XAMP_LOG_DEBUG("Player state:{}", player_->GetState());

    try {
        if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
            qTheme.SetPlayOrPauseButton(ui_, false);
            player_->Pause();
            current_playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_PAUSE);
            main_window_->SetTaskbarPlayerPaused();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
            qTheme.SetPlayOrPauseButton(ui_, true);
            player_->Resume();
            current_playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_PLAYING);
            main_window_->SetTaskbarPlayingResume();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
            if (!ui_.currentView->count()) {
                return;
            }
            playlist_page_ = current_playlist_page_;
            if (const auto select_item = playlist_page_->playlist()->GetSelectItem()) {
                play_index_ = select_item.value();
            }
            play_index_ = playlist_page_->playlist()->model()->index(
                play_index_.row(), PLAYLIST_PLAYING);
            if (play_index_.row() == -1) {
                XMessageBox::ShowInformation(tr("Not found any playing item."));
                return;
            }
            playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_CLEAR);
            playlist_page_->playlist()->SetNowPlaying(play_index_, true);
            playlist_page_->playlist()->play(play_index_);
        }
    }
    catch (Exception const &e) {
        XAMP_LOG_DEBUG(e.GetStackTrace());
    }
    catch (std::exception const &e) {
        XAMP_LOG_DEBUG(e.what());
    }
    catch (...) {	    
    }
}

void Xamp::ResetSeekPosValue() {
    ui_.seekSlider->setValue(0);
    ui_.startPosLabel->setText(FormatDuration(0));
}

void Xamp::ProcessTrackInfo(const ForwardList<TrackInfo>&) const {
    album_page_->album()->Refresh();
    playlist_page_->playlist()->Reload();
}

void Xamp::SetupDsp(const PlayListEntity& item) {
    if (AppSettings::ValueAsBool(kAppSettingEnableReplayGain)) {
        const auto mode = AppSettings::ValueAsEnum<ReplayGainMode>(kAppSettingReplayGainMode);
        if (mode == ReplayGainMode::RG_ALBUM_MODE) {
            player_->GetDspManager()->AddVolume();
            player_->GetDspConfig().AddOrReplace(DspConfig::kVolume, item.album_replay_gain);
        } else if (mode == ReplayGainMode::RG_TRACK_MODE) {
            player_->GetDspManager()->AddVolume();
            player_->GetDspConfig().AddOrReplace(DspConfig::kVolume, item.track_replay_gain);
        } else {
            player_->GetDspManager()->RemoveVolume();
        }
    } else {
        player_->GetDspManager()->RemoveVolume();
    }

    if (AppSettings::ValueAsBool(kAppSettingEnableEQ)) {
        if (AppSettings::contains(kAppSettingEQName)) {
            const auto [name, settings] = 
                AppSettings::GetEqSettings();
            player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, settings);
            player_->GetDspManager()->AddEqualizer();
        }
    } else {
        player_->GetDspManager()->RemoveEqualizer();
    }
}

QString Xamp::TranslateErrorCode(const Errors error) const {
    return FromStdStringView(EnumToString(error));
}

static std::pair<DsdModes, Pcm2DsdConvertModes> GetDsdModes(const DeviceInfo & device_info,
    const Path & file_path,
    int32_t input_sample_rate,
    int32_t target_sample_rate) {
    auto convert_mode = Pcm2DsdConvertModes::PCM2DSD_NONE;
    auto dsd_modes = DsdModes::DSD_MODE_AUTO;
    const auto is_enable_sample_rate_converter = target_sample_rate > 0;
    const auto is_dsd_file = IsDsdFile(file_path);

    if (AppSettings::ValueAsBool(kEnablePcm2Dsd)
        && !is_dsd_file
        && !is_enable_sample_rate_converter
        && input_sample_rate % kPcmSampleRate441 == 0) {
        dsd_modes = DsdModes::DSD_MODE_DOP;
        convert_mode = Pcm2DsdConvertModes::PCM2DSD_DSD_DOP;
    }

    if (dsd_modes == DsdModes::DSD_MODE_AUTO) {
        if (is_dsd_file) {
            if (device_info.is_support_dsd && !is_enable_sample_rate_converter) {
                if (IsAsioDevice(device_info.device_type_id)) {
                    dsd_modes = DsdModes::DSD_MODE_NATIVE;
                }
                else {
                    dsd_modes = DsdModes::DSD_MODE_DOP;
                }
            } else {
                dsd_modes = DsdModes::DSD_MODE_DSD2PCM;
            }
        } else {
            dsd_modes = DsdModes::DSD_MODE_PCM;
        }
    }

    return { dsd_modes, convert_mode };
}

void Xamp::SetupSampleWriter(Pcm2DsdConvertModes convert_mode,
    DsdModes dsd_modes,
    int32_t input_sample_rate,
    ByteFormat byte_format,
    PlaybackFormat& playback_format) const {

	if (convert_mode == Pcm2DsdConvertModes::PCM2DSD_DSD_DOP) {
		uint32_t device_sample_rate = 0;

		auto config = JsonSettings::ValueAsMap(kPCM2DSD);
		auto dsd_times = static_cast<DsdTimes>(config[kPCM2DSDDsdTimes].toInt());
		auto pcm2dsd_writer = MakeAlign<ISampleWriter, Pcm2DsdSampleWriter>(dsd_times);
		auto* writer = dynamic_cast<Pcm2DsdSampleWriter*>(pcm2dsd_writer.get());

		CpuAffinity affinity;
		affinity.Set(2);
		affinity.Set(3);
		writer->Init(input_sample_rate, affinity, convert_mode);

		if (convert_mode == Pcm2DsdConvertModes::PCM2DSD_DSD_DOP) {
			device_sample_rate = GetDOPSampleRate(writer->GetDsdSpeed());
		} else {
			device_sample_rate = writer->GetDsdSampleRate();
		}

		player_->GetDspManager()->SetSampleWriter(std::move(pcm2dsd_writer));

		player_->PrepareToPlay(byte_format);
		player_->SetReadSampleSize(writer->GetDataSize() * 2);

		playback_format = GetPlaybackFormat(player_.get());
		playback_format.is_dsd_file = true;
		playback_format.dsd_mode = dsd_modes;
		playback_format.dsd_speed = writer->GetDsdSpeed();
		playback_format.output_format.SetSampleRate(device_sample_rate);
	} else {
		player_->GetDspManager()->SetSampleWriter();
		player_->PrepareToPlay(byte_format);
		playback_format = GetPlaybackFormat(player_.get());
	}
}

bool Xamp::ShowMeMessage(const QString& message) {
    if (AppSettings::DontShowMeAgain(message)) {
        auto [button, checked] = XMessageBox::ShowCheckBoxInformation(
            message,
            tr("Ok, and don't show again."),
            kApplicationTitle,
            false,
            QDialogButtonBox::No | QDialogButtonBox::Yes,
            QDialogButtonBox::No);
        if (checked) {
            AppSettings::AddDontShowMeAgain(message);
            return true;
        }
        return button == QDialogButtonBox::Yes;
    }
    return true;
}

void Xamp::showEvent(QShowEvent* event) {
    IXFrame::showEvent(event);
}

void Xamp::SetupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
    uint32_t &target_sample_rate,
    QString& sample_rate_converter_type) {
    sample_rate_converter_type = AppSettings::ValueAsString(kAppSettingResamplerType);

    if (!AppSettings::ValueAsBool(kEnableBitPerfect)) {
		if (AppSettings::ValueAsBool(kAppSettingResamplerEnable)) {
			if (sample_rate_converter_type == kSoxr || sample_rate_converter_type.isEmpty()) {
				QMap<QString, QVariant> soxr_settings;
				const auto setting_name = AppSettings::ValueAsString(kAppSettingSoxrSettingName);
				soxr_settings = JsonSettings::GetValue(kSoxr).toMap()[setting_name].toMap();
				target_sample_rate = soxr_settings[kResampleSampleRate].toUInt();

				initial_sample_rate_converter = [=]() {
					player_->GetDspManager()->AddPreDSP(MakeSoxrSampleRateConverter(soxr_settings));
				};
			}
			else if (sample_rate_converter_type == kR8Brain) {
				auto config = JsonSettings::ValueAsMap(kR8Brain);
				target_sample_rate = config[kResampleSampleRate].toUInt();

				initial_sample_rate_converter = [=]() {
					player_->GetDspManager()->AddPreDSP(MakeR8BrainSampleRateConverter());
				};
			}
		}
	}
}

void Xamp::play(const PlayListEntity& item) {
    auto open_done = false;

    ui_.seekSlider->setEnabled(true);

    XAMP_ON_SCOPE_EXIT(
        if (open_done) {
            return;
        }
        ResetSeekPosValue();
        ui_.seekSlider->setEnabled(false);
        player_->Stop(false, true);
        );

    QString sample_rate_converter_type;
    PlaybackFormat playback_format;
    uint32_t target_sample_rate = 0;
    auto byte_format = ByteFormat::SINT32;

    std::function<void()> sample_rate_converter_factory;
    SetupSampleRateConverter(sample_rate_converter_factory, target_sample_rate, sample_rate_converter_type);

    const auto [open_dsd_mode, convert_mode] = GetDsdModes(device_info_,
        item.file_path.toStdWString(),
        item.sample_rate,
        target_sample_rate);

    try {
        player_->Open(item.file_path.toStdWString(),
            device_info_, 
            target_sample_rate,
            open_dsd_mode);

        if (!sample_rate_converter_factory) {
            player_->GetDspManager()->RemoveSampleRateConverter();
            player_->GetDspManager()->RemoveVolume();
            player_->GetDspManager()->RemoveEqualizer();

            if (player_->GetInputFormat().GetByteFormat() == ByteFormat::SINT16) {
                byte_format = ByteFormat::SINT16;
            }
        } else {
            if (player_->GetInputFormat().GetSampleRate() == target_sample_rate) {
                player_->GetDspManager()->RemoveSampleRateConverter();
            }
            else {
                sample_rate_converter_factory();
            }
            SetupDsp(item);
        }

        // note: Only PCM dsd modes enable compressor.
        if (player_->GetDsdModes() == DsdModes::DSD_MODE_PCM) {
            player_->GetDspManager()->AddCompressor();
        }

        if (device_info_.connect_type == DeviceConnectType::BLUE_TOOTH) {
            if (player_->GetInputFormat() != AudioFormat::k16BitPCM441Khz) {
                const auto message = 
                    qSTR("Playing blue-tooth device need set %1bit/%2Khz to 16bit/44.1Khz.")
                    .arg(player_->GetInputFormat().GetBitsPerSample())
                    .arg(FormatSampleRate(player_->GetInputFormat().GetSampleRate()));
                ShowMeMessage(message);
            }
            byte_format = ByteFormat::SINT16;
        }

        if (convert_mode == Pcm2DsdConvertModes::PCM2DSD_DSD_DOP) {
            sample_rate_converter_type = qEmptyString;
        }

        SetupSampleWriter(convert_mode,
            open_dsd_mode,
            item.sample_rate,
            byte_format,
            playback_format);

        playback_format.bit_rate = item.bit_rate;
        if (sample_rate_converter_type == kR8Brain) {
            player_->SetReadSampleSize(kR8brainBufferSize);
        }

        player_->BufferStream();
        open_done = true;
    }
    catch (const Exception & e) {        
        XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
        XAMP_LOG_DEBUG(e.GetStackTrace());
    }
    catch (const std::exception & e) {
        XMessageBox::ShowError(qTEXT(e.what()));
    }
    catch (...) {        
        XMessageBox::ShowError(tr("Unknown error"));
    }

    UpdateUi(item, playback_format, open_done);
}

void Xamp::UpdateUi(const PlayListEntity& item, const PlaybackFormat& playback_format, bool open_done) {
    auto* cur_page = current_playlist_page_;

    QString ext = item.file_extension;
    if (item.file_extension.isEmpty()) {
        ext = qTEXT(".m4a");
    }
	
    qTheme.SetPlayOrPauseButton(ui_, open_done);
    lrc_page_->spectrum()->reset();
	
    if (open_done) {
        auto max_duration_ms = Round(player_->GetDuration()) * 1000;
        ui_.seekSlider->SetRange(0, max_duration_ms - 1000);
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(FormatDuration(0));
        ui_.endPosLabel->setText(FormatDuration(player_->GetDuration()));
        cur_page->format()->setText(Format2String(playback_format, ext));

        artist_info_page_->SetArtistId(item.artist,
            qDatabase.GetArtistCoverId(item.artist_id),
            item.artist_id);
        album_page_->album()->SetPlayingAlbumId(item.album_id);
        UpdateButtonState();
        emit NowPlaying(item.artist, item.title);
    } else {
        cur_page->playlist()->Reload();
    }

    SetCover(item.cover_id, cur_page);

    ui_.titleLabel->setText(item.title);
    ui_.artistLabel->setText(item.artist);

    cur_page->title()->setText(item.title);    
    lrc_page_->lyrics()->LoadLrcFile(item.file_path);
	
    lrc_page_->title()->setText(item.title);
    lrc_page_->album()->setText(item.album);
    lrc_page_->artist()->setText(item.artist);

    if (open_done) {
        ui_.formatLabel->setText(Format2String(playback_format, ext));
        if (playback_format.file_format > AudioFormat::k16BitPCM441Khz) {
            ui_.hiResLabel->show();
            ui_.hiResLabel->setIcon(qTheme.GetHiResIcon());
        } else {
            ui_.hiResLabel->hide();
        }        
        player_->Play();
    }

    podcast_page_->format()->setText(qEmptyString);

    auto lyrc_opt = qDatabase.GetLyrc(item.music_id);
    if (!lyrc_opt) {
        emit SearchLyrics(item.music_id, item.title, item.artist);
    } else {
        OnSearchLyricsCompleted(item.music_id, std::get<0>(lyrc_opt.value()), std::get<1>(lyrc_opt.value()));
    }
}

void Xamp::OnUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info) {
    const auto disc_id = QString::fromStdString(mb_disc_id_info.disc_id);
    const auto album = QString::fromStdWString(mb_disc_id_info.album);
    const auto artist = QString::fromStdWString(mb_disc_id_info.artist);

    if (!album.isEmpty()) {
        qDatabase.UpdateAlbumByDiscId(disc_id, album);
    }
    if (!artist.isEmpty()) {
        qDatabase.UpdateArtistByDiscId(disc_id, artist);
    }

	const auto album_id = qDatabase.GetAlbumIdByDiscId(disc_id);

    if (!mb_disc_id_info.tracks.empty()) {
        qDatabase.ForEachAlbumMusic(album_id, [&mb_disc_id_info](const auto& entity) {
            qDatabase.UpdateMusicTitle(entity.music_id, QString::fromStdWString(mb_disc_id_info.tracks.front().title));
            });
    }    

    if (!album.isEmpty()) {
        cd_page_->playlistPage()->title()->setText(album);
        cd_page_->playlistPage()->playlist()->Reload();
    }

    if (const auto album_stats = qDatabase.GetAlbumStats(album_id)) {
        cd_page_->playlistPage()->format()->setText(tr("%1 Songs, %2, %3")
            .arg(QString::number(album_stats.value().songs))
            .arg(FormatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year)));
    }
}

void Xamp::OnUpdateDiscCover(const QString& disc_id, const QString& cover_id) {
	const auto album_id = qDatabase.GetAlbumIdByDiscId(disc_id);
    qDatabase.SetAlbumCover(album_id, cover_id);
    SetCover(cover_id, cd_page_->playlistPage());
}

void Xamp::OnUpdateCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos) {
    const auto album_id = qDatabase.GetAlbumIdByDiscId(disc_id);
    qDatabase.RemoveAlbum(album_id);

    cd_page_->playlistPage()->playlist()->RemoveAll();
    cd_page_->playlistPage()->playlist()->ProcessTrackInfo(track_infos);
    cd_page_->showPlaylistPage(true);
}

void Xamp::DrivesChanges(const QList<DriveInfo>& drive_infos) {
    cd_page_->playlistPage()->playlist()->RemoveAll();
    cd_page_->playlistPage()->playlist()->Reload();
    emit FetchCdInfo(drive_infos.first());
}

void Xamp::DrivesRemoved(const DriveInfo& /*drive_info*/) {
    cd_page_->playlistPage()->playlist()->RemoveAll();
    cd_page_->playlistPage()->playlist()->Reload();
    cd_page_->showPlaylistPage(false);
}

void Xamp::SetCover(const QString& cover_id, PlaylistPage* page) {
    auto found_cover = !current_entity_.cover_id.isEmpty();

    if (cover_id != qPixmapCache.GetUnknownCoverId()) {
        const auto cover = qPixmapCache.find(cover_id, false);
        if (!cover.isNull()) {
            found_cover = true;
            SetPlaylistPageCover(&cover, page);
            emit BlurImage(cover_id, cover.copy(), size());
        }

    	if (lrc_page_ != nullptr) {
            lrc_page_->ClearBackground();
        }
    }

    if (!found_cover) {
        SetPlaylistPageCover(nullptr, page);
        if (lrc_page_ != nullptr) {
            lrc_page_->SetBackgroundColor(qTheme.BackgroundColor());
        }
    }
}

PlaylistPage* Xamp::CurrentPlyalistPage() {
    current_playlist_page_ = dynamic_cast<PlaylistPage*>(ui_.currentView->currentWidget());
    if (!current_playlist_page_) {
        current_playlist_page_ = playlist_page_;
    }
    return current_playlist_page_;
}

void Xamp::PlayPlayListEntity(const PlayListEntity& item) {
    current_playlist_page_ = qobject_cast<PlaylistPage*>(sender());
    main_window_->SetTaskbarPlayerPlaying();
    current_entity_ = item;
    play(item);
    update();
}

void Xamp::PlayNextItem(int32_t forward) {
    if (!current_playlist_page_) {
        CurrentPlyalistPage();
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
            play_index_ = playlist_view->GetNextIndex(forward);
            if (play_index_.row() == -1) {
                return;
            }
            break;
        case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
            play_index_ = playlist_view->GetShuffleIndex();
            break;
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        default:
            break;
        }

        if (!play_index_.isValid()) {
            XMessageBox::ShowInformation(tr("Not found any playlist item."));
            return;
        }
    } else {
        play_index_ = playlist_view->model()->index(0, 0);
    }

    try {
        playlist_view->play(play_index_);
    }
    catch (Exception const &e) {
        XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
        return;
    }
    playlist_view->FastReload();
}

void Xamp::OnArtistIdChanged(const QString& artist, const QString& /*cover_id*/, int32_t artist_id) {
    artist_info_page_->SetArtistId(artist, qDatabase.GetArtistCoverId(artist_id), artist_id);
    ui_.currentView->setCurrentWidget(artist_info_page_);
}

void Xamp::AddPlaylistItem(const ForwardList<int32_t>& music_ids, const ForwardList<PlayListEntity> & entities) {
    auto playlist_view = playlist_page_->playlist();
    qDatabase.AddMusicToPlaylist(music_ids, playlist_view->GetPlaylistId());
    playlist_view->Reload();
}

void Xamp::OnClickedAlbum(const QString& album, int32_t album_id, const QString& cover_id) {
    album_page_->album()->HideWidget();
    album_page_->album()->albumViewPage()->SetPlaylistMusic(album, album_id, cover_id);
    ui_.currentView->setCurrentWidget(album_page_);
}

void Xamp::SetPlaylistPageCover(const QPixmap* cover, PlaylistPage* page) {
    if (!cover) {
        cover = &qTheme.UnknownCover();
    }

	if (!page) {
        page = current_playlist_page_;
	}

    const auto ui_cover = image_utils::RoundImage(
        image_utils::ResizeImage(*cover, ui_.coverLabel->size(), false),
        image_utils::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);

    page->SetCover(cover);

    if (lrc_page_ != nullptr) {
        lrc_page_->SetCover(image_utils::ResizeImage(*cover, lrc_page_->cover()->size(), true));
    }   
}

void Xamp::OnPlayerStateChanged(xamp::player::PlayerState play_state) {
    if (!player_) {
        return;
    }
    if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
        main_window_->ResetTaskbarProgress();
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(FormatDuration(0));
        PlayNextItem(1);
        PayNextMusic();
    }
}

void Xamp::InitialPlaylist() {
    lrc_page_ = new LrcPage(this);
    album_page_ = new AlbumArtistPage(this);
    artist_info_page_ = new ArtistInfoPage(this);
    preference_page_ = new PreferencePage(this);
    about_page_ = new AboutPage(this);

    ui_.sliderBar->AddTab(tr("Playlists"), TAB_PLAYLIST, qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST));
    ui_.sliderBar->AddTab(tr("File Explorer"), TAB_FILE_EXPLORER, qTheme.GetFontIcon(Glyphs::ICON_DESKTOP));
    ui_.sliderBar->AddTab(tr("Lyrics"), TAB_LYRICS, qTheme.GetFontIcon(Glyphs::ICON_SUBTITLE));
    ui_.sliderBar->AddTab(tr("Podcast"), TAB_PODCAST, qTheme.GetFontIcon(Glyphs::ICON_PODCAST));
    ui_.sliderBar->AddTab(tr("Albums"), TAB_ALBUM, qTheme.GetFontIcon(Glyphs::ICON_ALBUM));
    ui_.sliderBar->AddTab(tr("Artists"), TAB_ARTIST, qTheme.GetFontIcon(Glyphs::ICON_ARTIST));
    ui_.sliderBar->AddTab(tr("Settings"), TAB_SETTINGS, qTheme.GetFontIcon(Glyphs::ICON_SETTINGS));
    ui_.sliderBar->AddTab(tr("CD"), TAB_CD, qTheme.GetFontIcon(Glyphs::ICON_CD));
    ui_.sliderBar->AddTab(tr("About"), TAB_ABOUT, qTheme.GetFontIcon(Glyphs::ICON_ABOUT));
    ui_.sliderBar->setCurrentIndex(ui_.sliderBar->model()->index(0, 0));

    qDatabase.ForEachTable([this](auto table_id,
        auto /*table_index*/,
        auto playlist_id,
        const auto& name) {
            if (name.isEmpty()) {
                return;
            }

            ui_.sliderBar->AddTab(name, table_id, qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST));

            if (!playlist_page_) {
                playlist_page_ = NewPlaylistPage(playlist_id, qEmptyString);
                playlist_page_->playlist()->SetPlaylistId(playlist_id, name);
            }

            if (playlist_page_->playlist()->GetPlaylistId() != playlist_id) {
                playlist_page_ = NewPlaylistPage(playlist_id, qEmptyString);
                playlist_page_->playlist()->SetPlaylistId(playlist_id, name);
            }
        });

    if (!playlist_page_) {
        auto playlist_id = kDefaultPlaylistId;
        if (!qDatabase.IsPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.AddPlaylist(qEmptyString, 0);
        }
        playlist_page_ = NewPlaylistPage(kDefaultPlaylistId, kAppSettingPlaylistColumnName);
        ConnectPlaylistPageSignal(playlist_page_);
        playlist_page_->playlist()->SetHeaderViewHidden(false);
    }

    if (!podcast_page_) {
        auto playlist_id = kDefaultPodcastPlaylistId;
        if (!qDatabase.IsPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.AddPlaylist(qEmptyString, 1);
        }
        podcast_page_ = NewPlaylistPage(playlist_id, kAppSettingPodcastPlaylistColumnName);
        podcast_page_->playlist()->SetPodcastMode();
        podcast_page_->playlist()->SetHeaderViewHidden(false);
        ConnectPlaylistPageSignal(podcast_page_);
    }

    if (!file_system_view_page_) {
        file_system_view_page_ = new FileSystemViewPage(this);
        auto playlist_id = kDefaultFileExplorerPlaylistId;
        if (!qDatabase.IsPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.AddPlaylist(qEmptyString, 2);
        }
        file_system_view_page_->playlistPage()->playlist()->SetPlaylistId(playlist_id, kAppSettingFileSystemPlaylistColumnName);
        file_system_view_page_->playlistPage()->playlist()->SetHeaderViewHidden(false);
        SetCover(qEmptyString, file_system_view_page_->playlistPage());
        ConnectPlaylistPageSignal(file_system_view_page_->playlistPage());
    }

    if (!cd_page_) {
        auto playlist_id = kDefaultCdPlaylistId;
        if (!qDatabase.IsPlaylistExist(playlist_id)) {
            playlist_id = qDatabase.AddPlaylist(qEmptyString, 4);
        }
        cd_page_ = new CdPage(this);
        cd_page_->playlistPage()->playlist()->SetPlaylistId(playlist_id, kAppSettingCdPlaylistColumnName);
        cd_page_->playlistPage()->playlist()->SetHeaderViewHidden(false);
        SetCover(qEmptyString, cd_page_->playlistPage());
        ConnectPlaylistPageSignal(cd_page_->playlistPage());
    }

    current_playlist_page_ = playlist_page_;

    (void)QObject::connect(&qPixmapCache,
        &PixmapCache::ProcessImage,
        background_worker_,
        &BackgroundWorker::OnProcessImage);

    (void)QObject::connect(this,
        &Xamp::BlurImage,
        background_worker_,
        &BackgroundWorker::OnBlurImage);

    (void)QObject::connect(this,
        &Xamp::FetchCdInfo,
        background_worker_,
        &BackgroundWorker::OnFetchCdInfo);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::OnReadCdTrackInfo,
        this,
        &Xamp::OnUpdateCdTrackInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::OnMbDiscInfo,
        this,
        &Xamp::OnUpdateMbDiscInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_,
		&BackgroundWorker::OnDiscCover,
        this,
        &Xamp::OnUpdateDiscCover,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_view_page_,
        &FileSystemViewPage::addDirToPlaylist,
        this,
        &Xamp::AppendToPlaylist);    

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        album_page_,
        &AlbumArtistPage::OnThemeChanged);

    if (!qDatabase.IsPlaylistExist(kDefaultAlbumPlaylistId)) {
        qDatabase.AddPlaylist(qEmptyString, 1);
    }
    ConnectPlaylistPageSignal(album_page_->album()->albumViewPage()->playlistPage());

    (void)QObject::connect(this,
        &Xamp::SearchLyrics,
        background_worker_,
        &BackgroundWorker::OnSearchLyrics);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::SearchLyricsCompleted,
        this,
        &Xamp::OnSearchLyricsCompleted);

    (void)QObject::connect(this,
        &Xamp::ReadTrackInfo,
        background_worker_,
        &BackgroundWorker::OnReadTrackInfo);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::ReadTrackInfo,
        background_worker_,
        &BackgroundWorker::OnReadTrackInfo);

    PushWidget(playlist_page_);
    PushWidget(lrc_page_);
    PushWidget(album_page_);
    PushWidget(artist_info_page_);
    PushWidget(podcast_page_);
    PushWidget(file_system_view_page_);
    PushWidget(preference_page_);
    PushWidget(cd_page_);
    PushWidget(about_page_);

    ui_.currentView->setCurrentIndex(0);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::ClickedArtist,
        this,
        &Xamp::OnArtistIdChanged);

    (void)QObject::connect(artist_info_page_->album(),
        &AlbumView::ClickedAlbum,
        this,
        &Xamp::OnClickedAlbum);

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        album_page_->album(),
        &AlbumView::OnThemeChanged);

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        artist_info_page_,
        &ArtistInfoPage::OnThemeChanged);

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        lrc_page_,
        &LrcPage::OnThemeChanged);

    (void)QObject::connect(background_worker_,
        &BackgroundWorker::BlurImage,
        lrc_page_,
        &LrcPage::SetBackground);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::AddPlaylist,
        this,
        &Xamp::AddPlaylistItem);

    (void)QObject::connect(artist_info_page_->album(),
        &AlbumView::AddPlaylist,
        this,
        &Xamp::AddPlaylistItem);
}

void Xamp::AppendToPlaylist(const QString& file_name) {
    try {
        playlist_page_->playlist()->append(file_name);
        album_page_->Refresh();
    }
    catch (const Exception& e) {
        XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
    }
}

void Xamp::AddItem(const QString& file_name) {
	const auto add_playlist = dynamic_cast<PlaylistPage*>(
        ui_.currentView->currentWidget()) != nullptr;

    if (add_playlist) {
        AppendToPlaylist(file_name);
    }
    else {
        ExtractFile(file_name);
    }
}

void Xamp::PushWidget(QWidget* widget) {
	const auto id = ui_.currentView->addWidget(widget);
    stack_page_id_.push(id);
    ui_.currentView->setCurrentIndex(id);
}

QWidget* Xamp::TopWidget() {
    if (!stack_page_id_.isEmpty()) {
        return ui_.currentView->widget(stack_page_id_.top());
    }
    return nullptr;
}

QWidget* Xamp::PopWidget() {
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

void Xamp::EncodeAacFile(const PlayListEntity& item, const EncodingProfile& profile) {
    auto last_dir = AppSettings::ValueAsString(kDefaultDir);

    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    const auto file_name = QFileDialog::getSaveFileName(this,
        tr("Save AAC file"),
        save_file_name,
        tr("AAC Files (*.m4a)"));

    if (file_name.isNull()) {
        return;
    }

    QDir current_dir;
    AppSettings::SetValue(kDefaultDir, current_dir.absoluteFilePath(file_name));

    const auto dialog = MakeProgressDialog(
        tr("Export progress dialog"),
        tr("Export '") + item.title + tr("' to aac file"),
        tr("Cancel"));

    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));

    TrackInfo track_info;
    track_info.album = item.album.toStdWString();
    track_info.artist = item.artist.toStdWString();
    track_info.title = item.title.toStdWString();
    track_info.track = item.track;

    AnyMap config;
    config.AddOrReplace(FileEncoderConfig::kInputFilePath, Path(item.file_path.toStdWString()));
    config.AddOrReplace(FileEncoderConfig::kOutputFilePath, Path(file_name.toStdWString()));
    config.AddOrReplace(FileEncoderConfig::kEncodingProfile, profile);

    try {
        auto encoder = StreamFactory::MakeAACEncoder();
        read_utiltis::EncodeFile(config,
            encoder,
            [&](auto progress) -> bool {
                dialog->SetValue(progress);
                qApp->processEvents();
                return dialog->WasCanceled() != true;
            }, track_info);
    }
    catch (Exception const& e) {
        XMessageBox::ShowError(qTEXT(e.what()));
    }
}

void Xamp::EncodeWavFile(const PlayListEntity& item) {
    auto last_dir = AppSettings::ValueAsString(kDefaultDir);

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
    AppSettings::SetValue(kDefaultDir, current_dir.absoluteFilePath(file_name));

    const auto dialog = MakeProgressDialog(
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
        read_utiltis::EncodeFile(item.file_path.toStdWString(),
            file_name.toStdWString(),
            encoder,
            command,
            [&](auto progress) -> bool {
                dialog->SetValue(progress);
                qApp->processEvents();
                return dialog->WasCanceled() != true;
            }, metadata);
    }
    catch (Exception const& e) {
        XMessageBox::ShowError(qTEXT(e.what()));
    }
}

void Xamp::EncodeFlacFile(const PlayListEntity& item) {
    auto last_dir = AppSettings::ValueAsString(kDefaultDir);

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
    AppSettings::SetValue(kDefaultDir, current_dir.absoluteFilePath(file_name));

    const auto dialog = MakeProgressDialog(
        tr("Export progress dialog"),
         tr("Export '") + item.title + tr("' to flac file"),
         tr("Cancel"));

    TrackInfo metadata;
    metadata.album = item.album.toStdWString();
    metadata.artist = item.artist.toStdWString();
    metadata.title = item.title.toStdWString();
    metadata.track = item.track;

    const auto command
    	= qSTR("-%1 -V").arg(AppSettings::GetValue(kFlacEncodingLevel).toInt()).toStdWString();

    try {
        auto encoder = StreamFactory::MakeFlacEncoder();
        read_utiltis::EncodeFile(item.file_path.toStdWString(),
            file_name.toStdWString(),
            encoder,
            command,
            [&](auto progress) -> bool {
                dialog->SetValue(progress);
                qApp->processEvents();
                return dialog->WasCanceled() != true;
            }, metadata);
    }
    catch (Exception const& e) {
        XMessageBox::ShowError(qTEXT(e.what()));
    }
}

void Xamp::ConnectPlaylistPageSignal(PlaylistPage* playlist_page) {
    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::AddPlaylistItemFinished,
        album_page_,
        &AlbumArtistPage::Refresh);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::PlayMusic,
        playlist_page,
        &PlaylistPage::PlayMusic);

    (void)QObject::connect(playlist_page,
        &PlaylistPage::PlayMusic,
        this,
        &Xamp::PlayPlayListEntity);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::EncodeFlacFile,
        this,
        &Xamp::EncodeFlacFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::EncodeAacFile,
        this,
        &Xamp::EncodeAacFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::EncodeWavFile,
        this,
        &Xamp::EncodeWavFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::ReadReplayGain,
        background_worker_,
        &BackgroundWorker::OnReadReplayGain);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::ReadTrackInfo,
        background_worker_,
        &BackgroundWorker::OnReadTrackInfo);

    if (playlist_page->playlist()->IsPodcastMode()) {
        (void)QObject::connect(playlist_page->playlist(),
            &PlayListTableView::FetchPodcast,
            background_worker_,
            &BackgroundWorker::OnFetchPodcast);

        (void)QObject::connect(background_worker_,
            &BackgroundWorker::FetchPodcastCompleted,
            playlist_page->playlist(),
            &PlayListTableView::OnFetchPodcastCompleted,
            Qt::QueuedConnection);

        (void)QObject::connect(background_worker_,
            &BackgroundWorker::FetchPodcastError,
            playlist_page->playlist(),
            &PlayListTableView::OnFetchPodcastError,
            Qt::QueuedConnection);
    }
    
    (void)QObject::connect(background_worker_,
        &BackgroundWorker::ReadReplayGain,
        playlist_page->playlist(),
        &PlayListTableView::UpdateReplayGain,
        Qt::QueuedConnection);

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        playlist_page->playlist(),
        &PlayListTableView::OnThemeColorChanged);

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        playlist_page,
        &PlaylistPage::OnThemeColorChanged);
}

PlaylistPage* Xamp::NewPlaylistPage(int32_t playlist_id, const QString& column_setting_name) {
	auto* playlist_page = new PlaylistPage(this);
    ui_.currentView->addWidget(playlist_page);    
    playlist_page->playlist()->SetPlaylistId(playlist_id, column_setting_name);
    return playlist_page;
}

void Xamp::AddDropFileItem(const QUrl& url) {
    AddItem(url.toLocalFile());
}

void Xamp::ExtractFile(const QString& file_path) {
    const auto adapter = QSharedPointer<DatabaseFacade>(new DatabaseFacade());
    (void)QObject::connect(adapter.get(),
        &DatabaseFacade::ReadCompleted,
        this,
        &Xamp::ProcessTrackInfo);
    emit ReadTrackInfo(adapter, file_path, playlist_page_->playlist()->GetPlaylistId(), false);
}
