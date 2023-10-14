#include <FramelessHelper/Widgets/framelesswidgetshelper.h>
#include <xamp.h>

#include <QShortcut>
#include <QToolTip>
#include <QWidgetAction>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

#include <QSimpleUpdater.h>

#include <base/logger_impl.h>
#include <base/scopeguard.h>
#include <base/dsd_utils.h>

#include <stream/api.h>
#include <stream/bassaacfileencoder.h>
#include <stream/idspmanager.h>
#include <stream/r8brainresampler.h>
#include <stream/compressorparameters.h>
#include <stream/isameplewriter.h>

#include <player/ebur128reader.h>

#include <output_device/iaudiodevicemanager.h>
#include <output_device/api.h>
#include <player/api.h>

#include <widget/widget_shared.h>
#include <widget/albumartistpage.h>
#include <widget/albumview.h>
#include <widget/appsettings.h>
#include <widget/xmessage.h>
#include <widget/backgroundworker.h>
#include <widget/database.h>
#include <widget/equalizerview.h>
#include <widget/filesystemviewpage.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/playlistpage.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlisttableview.h>
#include <widget/mbdiscid_uiltis.h>
#include <widget/spectrumwidget.h>
#include <widget/xdialog.h>
#include <widget/pendingplaylistpage.h>
#include <widget/xprogressdialog.h>
#include <widget/extractfileworker.h>
#include <widget/findalbumcoverworker.h>
#include <widget/imagecache.h>
#include <widget/image_utiltis.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/actionmap.h>
#include <widget/http.h>
#include <widget/jsonsettings.h>
#include <widget/read_until.h>
#include <widget/aboutpage.h>
#include <widget/cdpage.h>
#include <widget/preferencepage.h>
#include <widget/maskwidget.h>

#include <xmessagebox.h>
#include <thememanager.h>
#include <version.h>

namespace {
    void ShowMeMessage(const QString& message) {
        if (qAppSettings.DontShowMeAgain(message)) {
            auto [button, checked] = XMessageBox::ShowCheckBoxInformation(
                message,
                qApp->tr("Ok, and don't show again."),
                kApplicationTitle,
                false);
            if (checked) {
                qAppSettings.AddDontShowMeAgain(message);
            }
        }
    }

    void SetShufflePlayOrder(Ui::XampWindow& ui) {
        ui.repeatButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_SHUFFLE_PLAY_ORDER));
    }

    void SetRepeatOnePlayOrder(Ui::XampWindow& ui) {
        ui.repeatButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_REPEAT_ONE_PLAY_ORDER));
    }

    void SetRepeatOncePlayOrder(Ui::XampWindow& ui) {
        ui.repeatButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_REPEAT_ONCE_PLAY_ORDER));
    }

    void SetThemeIcon(Ui::XampWindow& ui) {
        qTheme.SetTitleBarButtonStyle(ui.closeButton, ui.minWinButton, ui.maxWinButton);

        const QColor hover_color = qTheme.GetHoverColor();

        ui.logoButton->setStyleSheet(qSTR(R"(
                                         QToolButton#logoButton {
                                         border: none;
                                         image: url(":/xamp/xamp.ico");
                                         background-color: transparent;
                                         }
										)"));
        constexpr auto kMaxTitleIcon = 20;
        ui.logoButton->setIconSize(QSize(kMaxTitleIcon, kMaxTitleIcon));

        ui.stopButton->setStyleSheet(qSTR(R"(
                                         QToolButton#stopButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
        ui.stopButton->setIcon(qTheme.GetFontIcon(ICON_STOP_PLAY));

        ui.nextButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
        ui.nextButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_PLAY_FORWARD));

        ui.prevButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
        ui.prevButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_PLAY_BACKWARD));

        ui.selectDeviceButton->setStyleSheet(qTEXT(R"(
                                                QToolButton#selectDeviceButton {                                                
                                                border: none;
                                                background-color: transparent;                                                
                                                }
                                                QToolButton#selectDeviceButton::menu-indicator { image: none; }
                                                )"));
        ui.selectDeviceButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_SPEAKER));

        ui.mutedButton->setStyleSheet(qSTR(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/%1/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )").arg(qTheme.GetThemeColorPath()));

        ui.eqButton->setStyleSheet(qTEXT(R"(
                                         QToolButton#eqButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
        ui.eqButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_EQUALIZER));

        ui.pendingPlayButton->setStyleSheet(qTEXT(R"(
                                            QToolButton#pendingPlayButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));
        ui.pendingPlayButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST_ORDER));

        ui.repeatButton->setStyleSheet(qTEXT(R"(
    QToolButton#repeatButton {
    border: none;
    background: transparent;
    }
    )"
        ));

        ui.menuButton->setStyleSheet(qSTR(R"(
                                         QToolButton#menuButton {                                                
                                                border: none;
                                                background-color: transparent;                                                
                                         }
                                         QToolButton#menuButton::menu-indicator { image: none; }
										 QToolButton#menuButton:hover {												
											background-color: %1;
											border-radius: 10px;								 
									     }
                                         )").arg(ColorToString(qTheme.GetHoverColor())));

        ui.menuButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_MORE));
    }

    void SetRepeatButtonIcon(Ui::XampWindow& ui, PlayerOrder order) {
        switch (order) {
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
            SetRepeatOncePlayOrder(ui);
            break;
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
            SetRepeatOnePlayOrder(ui);
            break;
        case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
            SetShufflePlayOrder(ui);
            break;
        default:
            break;
        }
    }

    void SetNaviBarTheme(TabListView* navi_bar) {
        QString tab_left_color;

        switch (qTheme.GetThemeColor()) {
        case ThemeColor::DARK_THEME:
            tab_left_color = qTEXT("42, 130, 218");
            break;
        case ThemeColor::LIGHT_THEME:
            tab_left_color = qTEXT("42, 130, 218");
            break;
        }

        navi_bar->setStyleSheet(qSTR(R"(
	QListView#naviBar {
		border: none; 
	}
	QListView#naviBar::item {
		border: 0px;
		padding-left: 6px;
	}
	QListView#naviBar::item:hover {
		border-radius: 2px;
	}
	QListView#naviBar::item:selected {
		padding-left: 4px;		
		border-left-width: 2px;
		border-left-style: solid;
		border-left-color: rgb(%1);
	}	
	)").arg(tab_left_color));
    }

    void SetWidgetStyle(Ui::XampWindow& ui) {
        ui.selectDeviceButton->setIconSize(QSize(32, 32));

        ui.playButton->setStyleSheet(qTEXT(R"(
                                            QToolButton#playButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));

        QFont duration_font(qTEXT("MonoFont"));
        duration_font.setPointSize(8);
        duration_font.setWeight(QFont::Bold);

        ui.startPosLabel->setStyleSheet(qTEXT(R"(
                                           QLabel#startPosLabel {
                                           color: gray;
                                           background-color: transparent;
                                           }
                                           )"));

        ui.endPosLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#endPosLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));
        ui.startPosLabel->setFont(duration_font);
        ui.endPosLabel->setFont(duration_font);

        ui.titleFrameLabel->setStyleSheet(qSTR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
    }
    )"));

        auto f = ui.tableLabel->font();
        f.setPointSize(11);
        ui.tableLabel->setFont(f);
        ui.tableLabel->setStyleSheet(qSTR(R"(
    QLabel#tableLabel {
    border: none;
    background: transparent;
	color: gray;
    }
    )"));

        if (qTheme.GetThemeColor() == ThemeColor::DARK_THEME) {
            ui.titleLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

            ui.formatLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#formatLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

            ui.currentView->setStyleSheet(qSTR(R"(
			QStackedWidget#currentView {
				padding: 0px;
				background-color: #121212;				
				border-top-left-radius: 0px;
            }			
            )"));

            ui.bottomFrame->setStyleSheet(
                qTEXT(R"(
            QFrame#bottomFrame{
                border-top: 1px solid black;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
        }
        else {
            ui.titleLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#titleLabel {
                                         color: black;
                                         background-color: transparent;
                                         }
                                         )"));

            ui.formatLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#formatLabel {
                                         color: black;
                                         background-color: transparent;
                                         }
                                         )"));

            ui.currentView->setStyleSheet(qTEXT(R"(
			QStackedWidget#currentView {
				background-color: #f9f9f9;
				border-top-left-radius: 0px;
            }			
            )"));
            
            ui.bottomFrame->setStyleSheet(
                qTEXT(R"(
            QFrame#bottomFrame {
                border-top: 1px solid #eaeaea;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
        }

        ui.artistLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#artistLabel {
                                         color: rgb(250, 88, 106);
                                         background-color: transparent;
                                         }
                                         )"));

        SetNaviBarTheme(ui.naviBar);
        qTheme.SetSliderTheme(ui.seekSlider);

        ui.deviceDescLabel->setStyleSheet(qTEXT("background: transparent;"));

        SetThemeIcon(ui);
        ui.sliderFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));
        ui.sliderFrame2->setStyleSheet(qTEXT("background: transparent; border: none;"));
        ui.currentViewFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));
    }

    DsdModes GetDsdModes(const DeviceInfo& device_info,
        const Path& file_path,
        uint32_t input_sample_rate,
        uint32_t target_sample_rate) {
        auto dsd_modes = DsdModes::DSD_MODE_AUTO;
        const auto is_enable_sample_rate_converter = target_sample_rate > 0;
        const auto is_dsd_file = IsDsdFile(file_path);

        if (!is_dsd_file
            && !is_enable_sample_rate_converter
            && input_sample_rate % kPcmSampleRate441 == 0) {
            dsd_modes = DsdModes::DSD_MODE_AUTO;
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
                }
                else {
                    dsd_modes = DsdModes::DSD_MODE_DSD2PCM;
                }
            }
            else {
                dsd_modes = DsdModes::DSD_MODE_PCM;
            }
        }

        return dsd_modes;
    }
}

Xamp::Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer>& player)
    : IXFrame(parent)
    , is_seeking_(false)
    , trigger_upgrade_action_(false)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , main_window_(nullptr)
    , current_playlist_page_(nullptr)
	, lrc_page_(nullptr)
	, playlist_page_(nullptr)
	, music_page_(nullptr)
	, cd_page_(nullptr)
	, album_page_(nullptr)
	, file_system_view_page_(nullptr)
	, background_worker_(nullptr)
    , state_adapter_(std::make_shared<UIPlayerStateAdapter>())
	, player_(player) {
    ui_.setupUi(this);
}

Xamp::~Xamp() {
    cleanup();
}

void Xamp::SetXWindow(IXMainWindow* main_window) {
    FramelessWidgetsHelper::get(this)->setTitleBarWidget(ui_.titleFrame);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.minWinButton, SystemButtonType::Minimize);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.maxWinButton, SystemButtonType::Maximize);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.closeButton, SystemButtonType::Close);
    FramelessWidgetsHelper::get(this)->setHitTestVisible(ui_.menuButton);

    main_window_ = main_window;

    (void)QObject::connect(ui_.minWinButton, &QToolButton::clicked, [this]() {
        main_window_->showMinimized();
        });

    (void)QObject::connect(ui_.maxWinButton, &QToolButton::clicked, [this]() {
        main_window_->UpdateMaximumState();
        });

    (void)QObject::connect(ui_.closeButton, &QToolButton::clicked, [this]() {
        QWidget::close();
        });

    background_worker_.reset(new BackgroundWorker());
    background_worker_->moveToThread(&background_thread_);
    background_thread_.start(QThread::LowestPriority);

    find_album_cover_worker_.reset(new FindAlbumCoverWorker());
    find_album_cover_worker_->moveToThread(&find_album_cover_thread_);
    find_album_cover_thread_.start(QThread::LowestPriority);

    extract_file_worker_.reset(new ExtractFileWorker());
    extract_file_worker_->moveToThread(&extract_file_thread_);
    extract_file_thread_.start(QThread::LowestPriority);

    player_->Startup(state_adapter_);

    InitialUi();
    InitialController();
    InitialPlaylist();
    InitialShortcut();
    InitialSpectrum();

    SetPlaylistPageCover(nullptr, playlist_page_.get());
    SetPlaylistPageCover(nullptr, cd_page_->playlistPage());
    SetPlaylistPageCover(nullptr, file_system_view_page_->playlistPage());

    playlist_page_->HidePlaybackInformation(false);
    cd_page_->playlistPage()->HidePlaybackInformation(true);
    file_system_view_page_->playlistPage()->HidePlaybackInformation(false);

    const auto tab_name = qAppSettings.ValueAsString(kAppSettingLastTabName);
    const auto tab_id = ui_.naviBar->GetTabId(tab_name);
    if (tab_id != -1) {
        ui_.naviBar->setCurrentIndex(ui_.naviBar->model()->index(tab_id, 0));
        SetCurrentTab(tab_id);
    }

    (void)QObject::connect(album_page_->album(), &AlbumView::LoadCompleted,
        this, &Xamp::ProcessTrackInfo);

    (void)QObject::connect(this,
        &Xamp::Translation,
        background_worker_.get(),
        &BackgroundWorker::OnTranslation);

    (void)QObject::connect(background_worker_.get(), 
        &BackgroundWorker::TranslationCompleted,
        this,
        &Xamp::OnTranslationCompleted);

    (void)QObject::connect(find_album_cover_worker_.get(),
        &FindAlbumCoverWorker::SetAlbumCover,
        this, 
        &Xamp::OnSetAlbumCover);

    (void)QObject::connect(&qTheme, 
        &ThemeManager::CurrentThemeChanged, 
        this, 
        &Xamp::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        ui_.naviBar,
        &TabListView::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        album_page_->album(),
        &AlbumView::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        cd_page_.get(),
        &CdPage::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        ui_.mutedButton,
        &VolumeButton::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        lrc_page_.get(),
        &LrcPage::OnCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::CurrentThemeChanged,
        album_page_.get(),
        &AlbumArtistPage::OnCurrentThemeChanged);

    order_ = qAppSettings.ValueAsEnum<PlayerOrder>(kAppSettingOrder);
    SetPlayerOrder();
    InitialDeviceList();

    (void)QObject::connect(file_system_view_page_.get(),
        &FileSystemViewPage::ExtractFile,
        extract_file_worker_.get(),
        &ExtractFileWorker::OnExtractFile,
        Qt::QueuedConnection);

    (void)QObject::connect(this,
        &Xamp::ExtractFile,
        extract_file_worker_.get(),
        &ExtractFileWorker::OnExtractFile,
        Qt::QueuedConnection);

   (void)QObject::connect(album_page_->album(),
        &AlbumView::ExtractFile,
        extract_file_worker_.get(),
        &ExtractFileWorker::OnExtractFile,
        Qt::QueuedConnection);    

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::ReadFileProgress,
        this,
        &Xamp::OnReadFileProgress,
        Qt::QueuedConnection);   

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::FoundFileCount,
        this,
        &Xamp::OnFoundFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::CalculateEta,
        this,
        &Xamp::OnCalculateEta,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::ReadFilePath,
        this,
        &Xamp::OnReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::InsertDatabase,
        this,
        &Xamp::OnInsertDatabase,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::ReadFileStart,
        this,
        &Xamp::OnReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::ReadCompleted,
        this,
        &Xamp::OnReadCompleted,
        Qt::QueuedConnection);

    constexpr auto platform_key = qTEXT("windows");

    auto* updater = QSimpleUpdater::getInstance();

    updater->setPlatformKey(kSoftwareUpdateUrl, platform_key);
    updater->setModuleVersion(kSoftwareUpdateUrl, kApplicationVersion);
    updater->setUseCustomAppcast(kSoftwareUpdateUrl, true);

    (void)QObject::connect(updater,
        &QSimpleUpdater::appcastDownloaded,
        [updater, platform_key, this](const QString& url, const QByteArray& reply) {
        const auto document = QJsonDocument::fromJson(reply);
        const auto updates = document.object().value(qTEXT("updates")).toObject();
        const auto platform = updates.value(platform_key).toObject();
        const auto changelog = platform.value(qTEXT("changelog")).toString();
        const auto latest_version = platform.value(qTEXT("latest-version")).toString();

        Version latest_version_value;
        if (!ParseVersion(latest_version, latest_version_value)) {
            return;
        }
        
        if (trigger_upgrade_action_) {
            const auto download_url = platform.value(qTEXT("download-url")).toString();
            XMessageBox::ShowInformation(changelog, qTEXT("New version!"), false);
        }

        if (latest_version_value > kApplicationVersionValue) {
            trigger_upgrade_action_ = true;
        }
        });
    
    updater->checkForUpdates(kSoftwareUpdateUrl);

    auto* menu = new XMenu();
    qTheme.SetMenuStyle(menu);
    const auto * preference_action = menu->addAction(qTheme.GetFontIcon(Glyphs::ICON_SETTINGS), tr("Preference"));
    (void)QObject::connect(preference_action, &QAction::triggered, [this]() {
        auto* dialog = new XDialog(this);
        auto* preference_page = new PreferencePage(dialog);
        preference_page->LoadSettings();
        dialog->SetContentWidget(preference_page);
        dialog->SetIcon(qTheme.GetFontIcon(Glyphs::ICON_SETTINGS));
        dialog->SetTitle(tr("Preference"));
        dialog->exec();
        preference_page->SaveAll();
        });
    menu->addSeparator();
    const auto* check_for_updates_action = menu->addAction(tr("Check For Updates"));
    (void)QObject::connect(check_for_updates_action, &QAction::triggered, [updater, this]() {
        updater->checkForUpdates(kSoftwareUpdateUrl);
        });
    const auto* about_action = menu->addAction(qTheme.GetFontIcon(Glyphs::ICON_ABOUT),tr("About"));
    (void)QObject::connect(about_action, &QAction::triggered, [this]() {
        auto* dialog = new XDialog(this);
        auto* about_page = new AboutPage(dialog);
        dialog->SetContentWidget(about_page);
        dialog->SetIcon(qTheme.GetFontIcon(Glyphs::ICON_ABOUT));
        dialog->SetTitle(tr("About"));
        dialog->exec();
        });
    ui_.menuButton->setPopupMode(QToolButton::InstantPopup);
    ui_.menuButton->setMenu(menu);
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
    if (!qAppSettings.ValueAsBool(kAppSettingEnableSpectrum)) {
        return;
    }

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::fftResultChanged,
        lrc_page_->spectrum(),
        &SpectrumWidget::OnFftResultChanged,
        Qt::QueuedConnection);
}

void Xamp::UpdateMaximumState(bool is_maximum) {
    lrc_page_->SetFullScreen(is_maximum);
    qTheme.UpdateMaximumIcon(ui_.maxWinButton, is_maximum);
}

void Xamp::FocusIn() {
}

void Xamp::FocusOut() {
}

void Xamp::closeEvent(QCloseEvent* event) {
    if (XMessageBox::ShowYesOrNo(tr("Do you want to exit the app ?")) == QDialogButtonBox::No) {
        event->ignore();
        return;
    }
    cleanup();
    window()->close();
}

void Xamp::cleanup() {
    if (player_ != nullptr) {
        player_->Destroy();
        player_.reset();
        XAMP_LOG_DEBUG("Player destroy!");
    }

    auto worker_cancel_requested = [](auto* worker) {
        if (worker != nullptr) {
            worker->OnCancelRequested();
        }
    };

    auto quit_and_wait_thread = [](auto& thread) {
        if (!thread.isFinished()) {
            thread.requestInterruption();
            thread.quit();
            thread.wait();
        }
    };

    worker_cancel_requested(background_worker_.get());
    worker_cancel_requested(find_album_cover_worker_.get());
    worker_cancel_requested(extract_file_worker_.get());

    quit_and_wait_thread(background_thread_);
    quit_and_wait_thread(find_album_cover_thread_);
    quit_and_wait_thread(extract_file_thread_);

    if (main_window_ != nullptr) {
        main_window_->SaveGeometry();
    }

    XAMP_LOG_DEBUG("Xamp cleanup!");
}

void Xamp::InitialUi() {
    QFont f(qTEXT("DisplayFont"));
    f.setWeight(QFont::Bold);
    f.setPointSize(qTheme.GetFontSize(6));
    ui_.titleLabel->setFont(f);

    f.setWeight(QFont::Normal);
    f.setPointSize(qTheme.GetFontSize(6));
    ui_.artistLabel->setFont(f);    
    ui_.bitPerfectButton->setFont(f);

    QFont format_font(qTEXT("FormatFont"));
    format_font.setWeight(QFont::Normal);
    format_font.setPointSize(qTheme.GetFontSize(6));
    ui_.formatLabel->setFont(format_font);

    QToolTip::hideText();

    f.setWeight(QFont::DemiBold);
    f.setPointSize(10);
    ui_.titleFrameLabel->setFont(f);
    ui_.titleFrameLabel->setText(kApplicationTitle);
    ui_.titleFrameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QFont mono_font(qTEXT("MonoFont"));
    mono_font.setPointSize(qTheme.GetDefaultFontSize());
    ui_.startPosLabel->setFont(mono_font);
    ui_.endPosLabel->setFont(mono_font);
    ui_.seekSlider->setDisabled(true);

    ui_.mutedButton->SetPlayer(player_);
    ui_.coverLabel->setAttribute(Qt::WA_StaticContents);
    qTheme.SetPlayOrPauseButton(ui_.playButton, false);

    ui_.stopButton->hide();
}

void Xamp::OnVolumeChanged(float volume) {
    SetVolume(static_cast<int32_t>(volume * 100.0f));
}

void Xamp::OnDeviceStateChanged(DeviceState state) {
    XAMP_LOG_DEBUG("OnDeviceStateChanged: {}", state);

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
    f.setPointSize(qTheme.GetFontSize(8));
    f.setBold(true);
    desc_label->setFont(f);
    desc_label->setAlignment(Qt::AlignCenter);

    auto* device_type_frame = new QFrame();
    qTheme.SetTextSeparator(device_type_frame);

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

void Xamp::WaitForReady() {
    FramelessWidgetsHelper::get(this)->waitForReady();
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

    const auto device_type_id = qAppSettings.ValueAsId(kAppSettingDeviceType);
    const auto device_id = qAppSettings.ValueAsString(kAppSettingDeviceId).toStdString();
    const auto & device_manager = player_->GetAudioDeviceManager();

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
        menu->addAction(CreateDeviceMenuWidget(FromStdStringView(device_type->GetDescription())));
       
        for (const auto& device_info : device_info_list) {
            auto* device_action = new QAction(QString::fromStdWString(device_info.name), this);
            device_action_group->addAction(device_action);
            device_action->setCheckable(true);
            device_action->setChecked(false);
            device_id_action[device_info.device_id] = device_action;

            if (device_info_) {
                max_width = std::max(metrics.horizontalAdvance(QString::fromStdWString(device_info_.value().name)), max_width);
            }            

            auto trigger_callback = [device_info, this]() {
                qTheme.SetDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info.connect_type);
                device_info_ = device_info;
                ui_.deviceDescLabel->setText(QString::fromStdWString(device_info_.value().name));
                qAppSettings.SetValue(kAppSettingDeviceType, device_info_.value().device_type_id);
                qAppSettings.SetValue(kAppSettingDeviceId, device_info_.value().device_id);
            };

            (void)QObject::connect(device_action, &QAction::triggered, trigger_callback);
            menu->addAction(device_action);

            if (device_type_id == device_info.device_type_id && device_id == device_info.device_id) {
                device_info_ = device_info;
                is_find_setting_device = true;
                device_action->setChecked(true);
                // BUG! not show current connect icon.
                qTheme.SetDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info.connect_type);
                ui_.deviceDescLabel->setMinimumWidth(max_width + 60);
                ui_.deviceDescLabel->setText(QString::fromStdWString(device_info_.value().name));
            }
        }

        if (!is_find_setting_device) {
            auto itr = std::ranges::find_if(device_info_list, [](const auto& info) {
                return info.is_default_device && !IsExclusiveDevice(info);
            });
            if (itr != device_info_list.end()) {
                init_device_info = (*itr);
            }
        }
    }

    if (!is_find_setting_device) {
        device_info_ = init_device_info;
        qTheme.SetDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info_.value().connect_type);
        device_id_action[device_info_.value().device_id]->setChecked(true);
        qAppSettings.SetValue(kAppSettingDeviceType, device_info_.value().device_type_id);
        qAppSettings.SetValue(kAppSettingDeviceId, device_info_.value().device_id);
        XAMP_LOG_DEBUG("Use default device Id : {}", device_info_.value().device_id);
    }
}

void Xamp::InitialController() {    
    (void)QObject::connect(ui_.mutedButton, &QToolButton::clicked, [this]() {
        if (!player_->IsMute()) {
            SetVolume(0);
        } else {
            SetVolume(player_->GetVolume());
        }
    });

    qTheme.SetHeartButton(ui_.heartButton);
    qTheme.SetBitPerfectButton(ui_.bitPerfectButton, qAppSettings.ValueAsBool(kEnableBitPerfect));

    (void)QObject::connect(ui_.heartButton, &QToolButton::clicked, [this]() {
        if (!current_entity_) {
            return;
        }

        if (!playlist_page_) {
            return;
        }

        current_entity_.value().heart = !(current_entity_.value().heart);

        qMainDb.UpdateMusicHeart(current_entity_.value().music_id, current_entity_.value().heart);
        qTheme.SetHeartButton(ui_.heartButton, current_entity_.value().heart);
        playlist_page_->SetHeart(current_entity_.value().heart);
        playlist_page_->playlist()->Reload();
        });

    (void)QObject::connect(ui_.bitPerfectButton, &QToolButton::clicked, [this]() {
	    const auto enable_or_disable = !qAppSettings.ValueAsBool(kEnableBitPerfect);
        qAppSettings.SetValue(kEnableBitPerfect, enable_or_disable);
        qTheme.SetBitPerfectButton(ui_.bitPerfectButton, enable_or_disable);
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::LeftButtonValueChanged, [this](auto value) {
        try {
			is_seeking_ = true;
			player_->Seek(value / 1000.0);
            qTheme.SetPlayOrPauseButton(ui_.playButton, true);
            main_window_->SetTaskbarPlayingResume();
        }
        catch (const Exception & e) {
            player_->Stop(false);
            XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
        }
        is_seeking_ = false;
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::sliderReleased, [this]() {
        is_seeking_ = false;
        XAMP_LOG_DEBUG("SeekSlider released!");
    });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::sliderPressed, [this]() {
        is_seeking_ = false;
        XAMP_LOG_DEBUG("sliderPressed pressed!");
    });    

    if (qAppSettings.ValueAsBool(kAppSettingIsMuted)) {
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

    (void)QObject::connect(ui_.nextButton, &QToolButton::clicked, [this]() {
        PlayNext();
    });

    (void)QObject::connect(ui_.prevButton, &QToolButton::clicked, [this]() {
        PlayPrevious();
    });

    (void)QObject::connect(ui_.pendingPlayButton, &QToolButton::clicked, [this]() {
        SetPlayerOrder(true);
        auto* dialog = new XDialog(this);
        auto* page = new PendingPlaylistPage(current_playlist_page_->playlist()->GetPendingPlayIndexes(), dialog);
        dialog->SetContentWidget(page, true);
        dialog->SetTitle(tr("Pending playlist"));
        dialog->SetIcon(qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST_ORDER));
        (void)QObject::connect(page,
            &PendingPlaylistPage::PlayMusic,
            current_playlist_page_->playlist(),
            &PlayListTableView::PlayIndex);
        page->playlist()->Reload();
        auto center_pos = ui_.pendingPlayButton->mapToGlobal(ui_.pendingPlayButton->rect().topRight());
        const auto sz = dialog->size();
        center_pos.setX(center_pos.x() - sz.width());
        center_pos.setY(center_pos.y() - sz.height());
        center_pos = dialog->mapFromGlobal(center_pos);
        center_pos = dialog->mapToParent(center_pos);
        dialog->move(center_pos);        
        dialog->exec();
        });

    (void)QObject::connect(ui_.eqButton, &QToolButton::clicked, [this]() {
        if (player_->GetDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->GetDsdModes() == DsdModes::DSD_MODE_NATIVE) {
            return;
        }

        auto* dialog = new XDialog(this);
        auto* eq = new EqualizerView(dialog);
        dialog->SetContentWidget(eq, false);
        dialog->SetIcon(qTheme.GetFontIcon(Glyphs::ICON_EQUALIZER));
        dialog->SetTitle(tr("EQ"));
        eq->setMinimumWidth(800);

        (void)QObject::connect(eq, &EqualizerView::BandValueChange, [](auto, auto, auto) {
            qAppSettings.save();
        });

        (void)QObject::connect(eq, &EqualizerView::PreampValueChange, [](auto) {
            qAppSettings.save();
        });
        dialog->exec();
    });

    (void)QObject::connect(ui_.repeatButton, &QToolButton::clicked, [this]() {
        order_ = GetNextOrder(order_);
        SetPlayerOrder(true);
    });

    (void)QObject::connect(ui_.playButton, &QToolButton::clicked, [this]() {
        PlayOrPause();
    });

    (void)QObject::connect(ui_.stopButton, &QToolButton::clicked, [this]() {
        StopPlay();
    });

    (void)QObject::connect(ui_.artistLabel, &ClickableLabel::clicked, [this]() {
        if (current_entity_) {
            OnArtistIdChanged(current_entity_.value().artist,
                current_entity_.value().cover_id,
                current_entity_.value().artist_id);
        }        
    });

    (void)QObject::connect(ui_.coverLabel, &ClickableLabel::clicked, [this]() {
        ui_.currentView->setCurrentWidget(lrc_page_.get());
    });

    (void)QObject::connect(ui_.naviBar, &TabListView::ClickedTable, [this](auto table_id) {
        SetCurrentTab(table_id);
        qAppSettings.SetValue(kAppSettingLastTabName, ui_.naviBar->GetTabName(table_id));
    });

    (void)QObject::connect(ui_.naviBar, &TabListView::TableNameChanged, [](auto table_id, const auto &name) {
        qMainDb.SetTableName(table_id, name);
    });

    ui_.seekSlider->setEnabled(false);
    ui_.startPosLabel->setText(FormatDuration(0));
    ui_.endPosLabel->setText(FormatDuration(0));
}

void Xamp::SetCurrentTab(int32_t table_id) {
    switch (table_id) {
    case TAB_MUSIC_LIBRARY:
        album_page_->Refresh();
        ui_.currentView->setCurrentWidget(album_page_.get());
        break;
    case TAB_FILE_EXPLORER:
        ui_.currentView->setCurrentWidget(file_system_view_page_.get());
        break;
    case TAB_PLAYLIST:
        ui_.currentView->setCurrentWidget(playlist_page_.get());
        playlist_page_->playlist()->Reload();
        break;
    case TAB_LYRICS:
        ui_.currentView->setCurrentWidget(lrc_page_.get());
        break;
    case TAB_CD:
        ui_.currentView->setCurrentWidget(cd_page_.get());
        break;
    }
}

void Xamp::UpdateButtonState() {
    qTheme.SetPlayOrPauseButton(ui_.playButton, player_->GetState() != PlayerState::PLAYER_STATE_PAUSED);
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

    if (qAppSettings.ValueAsBool(kAppSettingIsMuted)) {
        qTheme.SetMuted(ui_.mutedButton, true);
    }
    else {
        qTheme.SetMuted(ui_.mutedButton, false);
    }

    qTheme.LoadAndApplyQssTheme();
    SetThemeIcon(ui_);
    SetRepeatButtonIcon(ui_, order_);
    SetThemeColor(qTheme.GetBackgroundColor(), qTheme.GetThemeTextColor());

    lrc_page_->SetCover(qTheme.GetUnknownCover());

    SetPlaylistPageCover(nullptr, playlist_page_.get());
    SetPlaylistPageCover(nullptr, cd_page_->playlistPage());
    SetPlaylistPageCover(nullptr, file_system_view_page_->playlistPage());
}

void Xamp::SetThemeColor(QColor background_color, QColor color) {
    qTheme.SetBackgroundColor(background_color);
    SetWidgetStyle(ui_);
    UpdateButtonState();
    emit ThemeChanged(background_color, color);
}

void Xamp::OnSearchArtistCompleted(const QString& artist, const QByteArray& image) {
    QPixmap cover;
    if (cover.loadFromData(image)) {        
        qMainDb.UpdateArtistCoverId(qMainDb.AddOrUpdateArtist(artist), qPixmapCache.AddImage(cover));
    }
    emit Translation(artist, qTEXT("ja"), qTEXT("en"));
    //emit Translation(artist, qTEXT("en"), qTEXT("ja"));
    album_page_->Refresh();
}

void Xamp::OnSearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics) {
    lrc_page_->lyrics()->SetLrc(lyrics, trlyrics);
    qMainDb.AddOrUpdateLyrc(music_id, lyrics, trlyrics);
}

void Xamp::SetFullScreen() {
    if (qAppSettings.ValueAsBool(kAppSettingEnterFullScreen)) {
        SetCurrentTab(TAB_LYRICS);
        ui_.bottomFrame->setHidden(true);
        ui_.sliderFrame->setHidden(true);
        ui_.titleFrame->setHidden(true);
        ui_.verticalSpacer_3->changeSize(0, 0);
        lrc_page_->SetFullScreen(true);
        qAppSettings.SetValue<bool>(kAppSettingEnterFullScreen, false);
    }
    else {
        ui_.bottomFrame->setHidden(false);
        ui_.sliderFrame->setHidden(false);
        ui_.titleFrame->setHidden(false);
        ui_.verticalSpacer_3->changeSize(20, 15, QSizePolicy::Minimum, QSizePolicy::Fixed);
        lrc_page_->SetFullScreen(false);
        qAppSettings.SetValue<bool>(kAppSettingEnterFullScreen, true);
    }
}

void Xamp::ShortcutsPressed(const QKeySequence& shortcut) {
    XAMP_LOG_DEBUG("shortcutsPressed: {}", shortcut.toString().toStdString());

	const QMap<QKeySequence, std::function<void()>> shortcut_map {
        { QKeySequence(Qt::Key_MediaPlay), [this]() {
            PlayOrPause();
            }},
         { QKeySequence(Qt::Key_MediaStop), [this]() {
            StopPlay();
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
        {
        	QKeySequence(Qt::Key_VolumeMute),
        	[this]() {
				SetVolume(0);
        	},
        },
        {
            QKeySequence(Qt::Key_F11),
            [this]() {
                SetFullScreen();
            },
        }
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
	const auto playlist_id = qMainDb.FindTablePlaylistId(table_id);

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
    qAppSettings.SetValue(kAppSettingVolume, volume);
    qTheme.SetMuted(ui_.mutedButton, volume == 0);
    ui_.mutedButton->OnVolumeChanged(volume);
    ui_.mutedButton->ShowDialog();
}

void Xamp::InitialShortcut() {
    const auto* space_key = new QShortcut(QKeySequence(Qt::Key_Space), this);
    (void)QObject::connect(space_key, &QShortcut::activated, [this]() {
        PlayOrPause();
    });
}

void Xamp::StopPlay() {
    player_->Stop(true, true);
    SetSeekPosValue(0);
    lrc_page_->spectrum()->Reset();
    ui_.seekSlider->setEnabled(false);
    playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_CLEAR);
    album_page_->album()->albumViewPage()->playlistPage()->playlist()->SetNowPlayState(PlayingState::PLAY_CLEAR);
    qTheme.SetPlayOrPauseButton(ui_.playButton, true); // note: Stop play must set true.
    current_entity_ = std::nullopt;
    main_window_->SetTaskbarPlayerStop();
}

void Xamp::PlayNext() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED 
        || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        PlayNextItem(1);
    } else {
        StopPlay();
    }
}

void Xamp::PlayPrevious() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED 
        || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        PlayNextItem(-1);
    }
    else {
        StopPlay();
    }
}

void Xamp::DeleteKeyPress() {
    if (!ui_.currentView->count()) {
        return;
    }
    auto* playlist_view = playlist_page_->playlist();
    playlist_view->RemoveSelectItems();
}

void Xamp::SetPlayerOrder(bool emit_order) {
    SetRepeatButtonIcon(ui_, order_);
    if (emit_order) {
        emit ChangePlayerOrder(order_);
    }
    qAppSettings.SetEnumValue(kAppSettingOrder, order_);
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
            qTheme.SetPlayOrPauseButton(ui_.playButton, false);
            player_->Pause();
            current_playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_PAUSE);
            main_window_->SetTaskbarPlayerPaused();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
            qTheme.SetPlayOrPauseButton(ui_.playButton, true);
            player_->Resume();
            current_playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_PLAYING);
            main_window_->SetTaskbarPlayingResume();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
            if (!ui_.currentView->count()) {
                return;
            }
            if (const auto select_item = current_playlist_page_->playlist()->GetSelectItem()) {
                play_index_ = select_item.value();
            }
            play_index_ = current_playlist_page_->playlist()->model()->index(
                play_index_.row(), PLAYLIST_PLAYING);
            if (play_index_.row() == -1) {
                XMessageBox::ShowInformation(tr("Not found any playing item."));
                return;
            }
            current_playlist_page_->playlist()->SetNowPlayState(PlayingState::PLAY_CLEAR);
            current_playlist_page_->playlist()->SetNowPlaying(play_index_, true);
            current_playlist_page_->playlist()->PlayIndex(play_index_);
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

void Xamp::ProcessTrackInfo(int32_t total_album, int32_t total_tracks) const {
    album_page_->album()->Refresh();
    playlist_page_->playlist()->Reload();
}

void Xamp::SetupDsp(const PlayListEntity& item) const {
    if (qAppSettings.ValueAsBool(kAppSettingEnableEQ)) {
        if (qAppSettings.contains(kAppSettingEQName)) {
            const auto [name, settings] =
                qAppSettings.GetEqSettings();
            player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, settings);
            player_->GetDspManager()->AddEqualizer();
            //player_->GetDspManager()->AddParametricEq();
        }
    }
    else {
        player_->GetDspManager()->RemoveEqualizer();
        //player_->GetDspManager()->RemoveParametricEq();
    }

    if (player_->GetDsdModes() == DsdModes::DSD_MODE_PCM) {
        CompressorParameters parameters;
        if (qAppSettings.ValueAsBool(kAppSettingEnableReplayGain)) {
            if (item.track_loudness != 0.0) {
                parameters.gain = Ebur128Reader::GetEbur128Gain(item.track_loudness, -1.0) * -1;
            }
        }
        player_->GetDspConfig().AddOrReplace(DspConfig::kCompressorParameters, parameters);
        player_->GetDspManager()->AddCompressor();
    }
    
    if (qAppSettings.ValueAsBool(kAppSettingEnableReplayGain)) {
        const auto mode = qAppSettings.ValueAsEnum<ReplayGainMode>(kAppSettingReplayGainMode);
        if (mode == ReplayGainMode::RG_ALBUM_MODE) {
            player_->GetDspManager()->AddVolumeControl();
            player_->GetDspConfig().AddOrReplace(DspConfig::kVolume, item.album_replay_gain);
        } else if (mode == ReplayGainMode::RG_TRACK_MODE) {
            player_->GetDspManager()->AddVolumeControl();
            player_->GetDspConfig().AddOrReplace(DspConfig::kVolume, item.track_replay_gain);
        } else {
            player_->GetDspManager()->RemoveVolumeControl();
        }
    } else {
        player_->GetDspManager()->RemoveVolumeControl();
    }
}

QString Xamp::TranslateErrorCode(const Errors error) const {
    return FromStdStringView(EnumToString(error));
}

void Xamp::SetupSampleWriter(ByteFormat byte_format,
    PlaybackFormat& playback_format) const {
    player_->GetDspManager()->SetSampleWriter();
    player_->PrepareToPlay(byte_format);
    playback_format = GetPlaybackFormat(player_.get());
}

void Xamp::showEvent(QShowEvent* event) {
    IXFrame::showEvent(event);
}

void Xamp::SetupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
    uint32_t &target_sample_rate,
    QString& sample_rate_converter_type) {
    sample_rate_converter_type = qAppSettings.ValueAsString(kAppSettingResamplerType);

    if (qAppSettings.ValueAsBool(kEnableBitPerfect)) {
        return;
	}

    if (!qAppSettings.ValueAsBool(kAppSettingResamplerEnable)) {
        return;
    }

    if (sample_rate_converter_type == kSoxr || sample_rate_converter_type.isEmpty()) {
        QMap<QString, QVariant> soxr_settings;
        const auto setting_name = qAppSettings.ValueAsString(kAppSettingSoxrSettingName);
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

void Xamp::PlayEntity(const PlayListEntity& entity) {
    if (!device_info_) {
        ShowMeMessage(tr("Not found any audio device, please check your audio device."));
        return;
    }

    lrc_page_->spectrum()->Reset();

    auto open_done = false;

    ui_.seekSlider->setEnabled(true);

    XAMP_ON_SCOPE_EXIT(
        if (open_done) {
            return;
        }
        current_entity_ = std::nullopt;
        ResetSeekPosValue();
        ui_.seekSlider->setEnabled(false);
        player_->Stop(false, true);
        );

    QString sample_rate_converter_type;
    PlaybackFormat playback_format;
    uint32_t target_sample_rate = 0;
    auto byte_format = ByteFormat::SINT32;
    std::function<void()> sample_rate_converter_factory;
    const auto enable_bit_perfect = qAppSettings.ValueAsBool(kEnableBitPerfect);

    player_->Stop();

    if (enable_bit_perfect) {
        player_->GetDspManager()->RemoveSampleRateConverter();
        player_->GetDspManager()->RemoveVolumeControl();
        player_->GetDspManager()->RemoveEqualizer();
        player_->GetDspManager()->RemoveCompressor();
    } else {
        SetupSampleRateConverter(sample_rate_converter_factory, 
            target_sample_rate, 
            sample_rate_converter_type);
    }

    if (device_info_.value().connect_type == DeviceConnectType::CONNECT_TYPE_BLUE_TOOTH) {
        AudioFormat default_format;
        if (device_info_.value().default_format) {
            default_format = device_info_.value().default_format.value();
        } else {
            default_format = AudioFormat::k16BitPCM441Khz;
        }
        if (entity.sample_rate != default_format.GetSampleRate()) {
            const auto message =
                qSTR("Playing blue-tooth device need set %1 to %2.")
                .arg(FormatSampleRate(entity.sample_rate))
                .arg(FormatSampleRate(default_format.GetSampleRate()));
            ShowMeMessage(message);
            player_->GetDspManager()->RemoveSampleRateConverter();
            target_sample_rate = default_format.GetSampleRate();
            sample_rate_converter_type = kSoxr;
            player_->GetDspManager()->AddPreDSP(MakeSampleRateConverter(target_sample_rate));
        }
        byte_format = ByteFormat::SINT16;
    }
    
    const auto open_dsd_mode = GetDsdModes(device_info_.value(),
        entity.file_path.toStdWString(),
        entity.sample_rate,
        target_sample_rate);

    try {
        player_->Open(entity.file_path.toStdWString(),
            device_info_.value(),
            target_sample_rate,
            open_dsd_mode);

        if (!enable_bit_perfect) {
            if (sample_rate_converter_factory != nullptr) {
                if (player_->GetInputFormat().GetSampleRate() == target_sample_rate) {
                    player_->GetDspManager()->RemoveSampleRateConverter();
                }
                else {
                    sample_rate_converter_factory();
                }
            }           
            SetupDsp(entity);
        } else {
            if (player_->GetInputFormat().GetByteFormat() == ByteFormat::SINT16) {
                byte_format = ByteFormat::SINT16;
            }
        }

        SetupSampleWriter(byte_format, playback_format);

        playback_format.bit_rate = entity.bit_rate;
        if (sample_rate_converter_type == kR8Brain) {
            player_->SetReadSampleSize(kR8brainBufferSize);
        }

        if (player_->GetDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->GetDsdModes() == DsdModes::DSD_MODE_NATIVE) {
            const auto message =
                qSTR("Play DSD file need set 100% volume.");
            ShowMeMessage(message);
            player_->SetVolume(100);
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

    if (open_done) {
        UpdateUi(entity, playback_format, open_done);
    }
    else {
        current_playlist_page_->playlist()->Reload();
    }    
}

void Xamp::UpdateUi(const PlayListEntity& item, const PlaybackFormat& playback_format, bool open_done) {
    QString ext = item.file_extension;
    if (item.file_extension.isEmpty()) {
        ext = qTEXT(".m4a");
    }
	
    qTheme.SetPlayOrPauseButton(ui_.playButton, open_done);

    const int64_t max_duration_ms = Round(player_->GetDuration()) * 1000;
    ui_.seekSlider->SetRange(0, max_duration_ms - 1000);
    ui_.seekSlider->setValue(0);
    
    current_playlist_page_->format()->setText(Format2String(playback_format, ext));
    current_playlist_page_->title()->setText(item.title);
    current_playlist_page_->SetHeart(item.heart);

    album_page_->album()->SetPlayingAlbumId(item.album_id);
    UpdateButtonState();
    emit NowPlaying(item.artist, item.title);

    SetCover(item.cover_id, current_playlist_page_);

    const QFontMetrics title_metrics(ui_.titleLabel->font());
    const QFontMetrics artist_metrics(ui_.artistLabel->font());
    
    ui_.titleLabel->setText(title_metrics.elidedText(item.title, Qt::ElideRight, ui_.titleLabel->width()));
    ui_.artistLabel->setText(artist_metrics.elidedText(item.artist, Qt::ElideRight, ui_.artistLabel->width()));
    ui_.formatLabel->setText(Format2String(playback_format, ext));
    
    lrc_page_->lyrics()->LoadLrcFile(item.file_path);	
    lrc_page_->title()->setText(item.title);
    lrc_page_->album()->setText(item.album);
    lrc_page_->artist()->setText(item.artist);
    lrc_page_->format()->setText(Format2String(playback_format, ext));
    lrc_page_->spectrum()->SetFftSize(state_adapter_->GetFftSize());
    lrc_page_->spectrum()->SetSampleRate(playback_format.output_format.GetSampleRate());

    auto lyrc_opt = qMainDb.GetLyrc(item.music_id);
    if (!lyrc_opt) {
        emit SearchLyrics(item.music_id, item.title, item.artist);
    } else {
        OnSearchLyricsCompleted(item.music_id, std::get<0>(lyrc_opt.value()), std::get<1>(lyrc_opt.value()));
    }

    qTheme.SetHeartButton(ui_.heartButton, current_entity_.value().heart);

    player_->Play();
}

void Xamp::OnUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info) {
    const auto disc_id = QString::fromStdString(mb_disc_id_info.disc_id);
    const auto album = QString::fromStdWString(mb_disc_id_info.album);
    const auto artist = QString::fromStdWString(mb_disc_id_info.artist);
    
    if (!album.isEmpty()) {
        qMainDb.UpdateAlbumByDiscId(disc_id, album);
    }
    if (!artist.isEmpty()) {
        qMainDb.UpdateArtistByDiscId(disc_id, artist);
    }

	const auto album_id = qMainDb.GetAlbumIdByDiscId(disc_id);

    if (!mb_disc_id_info.tracks.empty()) {
        QList<PlayListEntity> entities;
        qMainDb.ForEachAlbumMusic(album_id, [&entities](const auto& entity) {
            entities.append(entity);
            });
        std::ranges::sort(entities, [](const auto& a, const auto& b) {
            return b.track > a.track;
            });
        auto i = 0;
        Q_FOREACH(const auto track, mb_disc_id_info.tracks) {
            qMainDb.UpdateMusicTitle(entities[i++].music_id, QString::fromStdWString(track.title));
        }
    }    

    if (!album.isEmpty()) {
        cd_page_->playlistPage()->title()->setText(album);
        cd_page_->playlistPage()->playlist()->Reload();
    }

    if (const auto album_stats = qMainDb.GetAlbumStats(album_id)) {
        cd_page_->playlistPage()->format()->setText(tr("%1 Songs, %2, %3")
            .arg(QString::number(album_stats.value().songs))
            .arg(FormatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year)));
    }
}

void Xamp::OnUpdateDiscCover(const QString& disc_id, const QString& cover_id) {
	const auto album_id = qMainDb.GetAlbumIdByDiscId(disc_id);
    qMainDb.SetAlbumCover(album_id, cover_id);
}

void Xamp::OnUpdateCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos) {
    const auto album_id = qMainDb.GetAlbumIdByDiscId(disc_id);
    qMainDb.RemoveAlbum(album_id);
    cd_page_->playlistPage()->playlist()->RemoveAll();
    DatabaseFacade facade;
    facade.InsertTrackInfo(track_infos, kDefaultCdPlaylistId);    
    emit Translation(GetStringOrEmptyString(track_infos.front().artist), qTEXT("ja"), qTEXT("en"));
    cd_page_->playlistPage()->playlist()->Reload();
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
    auto found_cover = false;
    const auto cover = qPixmapCache.GetOrDefault(cover_id, true);

    if (!cover_id.isEmpty() && cover_id != qPixmapCache.GetUnknownCoverId()) {
        found_cover = true;
    }

    if (!found_cover) {
        SetPlaylistPageCover(nullptr, page);
    } else {
        SetPlaylistPageCover(&cover, page);
        emit BlurImage(cover_id, cover, QSize(500, 500));
    }

    if (lrc_page_ != nullptr) {
        lrc_page_->ClearBackground();
        lrc_page_->SetCover(image_utils::ResizeImage(cover, lrc_page_->cover()->size(), true));
        lrc_page_->AddCoverShadow(found_cover);        
    }

    main_window_->SetIconicThumbnail(cover);
}

PlaylistPage* Xamp::CurrentPlaylistPage() {
    current_playlist_page_ = dynamic_cast<PlaylistPage*>(ui_.currentView->currentWidget());
    if (!current_playlist_page_) {
        current_playlist_page_ = playlist_page_.get();
    }
    return current_playlist_page_;
}

void Xamp::PlayPlayListEntity(const PlayListEntity& entity) {
    current_playlist_page_ = qobject_cast<PlaylistPage*>(sender());
    main_window_->SetTaskbarPlayerPlaying();
    current_entity_ = entity;
    PlayEntity(entity);
    update();
}

void Xamp::PlayNextItem(int32_t forward) {
    if (!current_playlist_page_) {
        CurrentPlaylistPage();
    }

    auto* playlist_view = current_playlist_page_->playlist();
    const auto count = playlist_view->model()->rowCount();
    if (count == 0) {
        StopPlay();
        XMessageBox::ShowInformation(tr("Not found any playing item."));
        return;
    }    

    try {
        playlist_view->Play(order_);
        play_index_ = playlist_view->currentIndex();
    }
    catch (Exception const& e) {
        XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
        return;
    }
    playlist_view->FastReload();
}

void Xamp::OnArtistIdChanged(const QString& artist, const QString& /*cover_id*/, int32_t artist_id) {    
    ui_.currentView->setCurrentWidget(album_page_.get());
}

void Xamp::AddPlaylistItem(const QList<int32_t>& music_ids, const QList<PlayListEntity> & entities) {
	auto *playlist_view = playlist_page_->playlist();
    qMainDb.AddMusicToPlaylist(music_ids, playlist_view->GetPlaylistId());
    emit ChangePlayerOrder(order_);
}

void Xamp::SetPlaylistPageCover(const QPixmap* cover, PlaylistPage* page) {
    if (!cover) {
        cover = &qTheme.GetUnknownCover();
    }

	if (!page) {
        page = current_playlist_page_;
	}

    const QSize cover_size(ui_.coverLabel->size().width() - image_utils::kPlaylistImageRadius,
        ui_.coverLabel->size().height() - image_utils::kPlaylistImageRadius);
    const auto ui_cover = image_utils::RoundImage(
        image_utils::ResizeImage(*cover, cover_size, false),
        image_utils::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);
    page->SetCover(cover);
}

void Xamp::OnPlayerStateChanged(xamp::player::PlayerState play_state) {
    if (!player_) {
        return;
    }
    if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
        main_window_->ResetTaskbarProgress();
        lrc_page_->spectrum()->Reset();
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(FormatDuration(0));
        PlayNextItem(1);
        PayNextMusic();
    }
}

void Xamp::InitialPlaylist() {
    lrc_page_.reset(new LrcPage(this));
    album_page_.reset(new AlbumArtistPage(this));

    ui_.naviBar->AddSeparator();
    ui_.naviBar->AddTab(tr("Playlist"), TAB_PLAYLIST, qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST));
    ui_.naviBar->AddTab(tr("File explorer"), TAB_FILE_EXPLORER, qTheme.GetFontIcon(Glyphs::ICON_DESKTOP));
    ui_.naviBar->AddTab(tr("Lyrics"), TAB_LYRICS, qTheme.GetFontIcon(Glyphs::ICON_SUBTITLE));
    ui_.naviBar->AddTab(tr("Library"), TAB_MUSIC_LIBRARY, qTheme.GetFontIcon(Glyphs::ICON_MUSIC_LIBRARY));
    ui_.naviBar->AddTab(tr("CD"), TAB_CD, qTheme.GetFontIcon(Glyphs::ICON_CD));
    ui_.naviBar->setCurrentIndex(ui_.naviBar->model()->index(0, 0));

    qMainDb.ForEachTable([this](auto table_id,
        auto /*table_index*/,
        auto playlist_id,
        const auto& name) {
            if (name.isEmpty()) {
                return;
            }

            ui_.naviBar->AddTab(name, table_id, qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST));

            if (!playlist_page_) {
                playlist_page_.reset(NewPlaylistPage(playlist_id, kEmptyString));
                playlist_page_->playlist()->SetPlaylistId(playlist_id, name);
            }

            if (playlist_page_->playlist()->GetPlaylistId() != playlist_id) {
                playlist_page_.reset(NewPlaylistPage(playlist_id, kEmptyString));
                playlist_page_->playlist()->SetPlaylistId(playlist_id, name);                
            }
        });

    if (!playlist_page_) {
        auto playlist_id = kDefaultPlaylistId;
        if (!qMainDb.IsPlaylistExist(playlist_id)) {
            playlist_id = qMainDb.AddPlaylist(kEmptyString, 0);
        }
        playlist_page_.reset(NewPlaylistPage(kDefaultPlaylistId, kAppSettingPlaylistColumnName));
        ConnectPlaylistPageSignal(playlist_page_.get());
        playlist_page_->playlist()->SetHeaderViewHidden(false);

        (void)QObject::connect(this,
            &Xamp::ChangePlayerOrder,
            playlist_page_->playlist(),
            &PlayListTableView::AddPendingPlayListFromModel);
    }

    if (!file_system_view_page_) {
        file_system_view_page_.reset(new FileSystemViewPage(this));
        auto playlist_id = kDefaultFileExplorerPlaylistId;
        if (!qMainDb.IsPlaylistExist(playlist_id)) {
            playlist_id = qMainDb.AddPlaylist(kEmptyString, 2);
        }
        file_system_view_page_->playlistPage()->playlist()->SetPlaylistId(playlist_id, kAppSettingFileSystemPlaylistColumnName);
        file_system_view_page_->playlistPage()->playlist()->SetHeaderViewHidden(false);
        file_system_view_page_->playlistPage()->playlist()->SetOtherPlaylist(kDefaultPlaylistId);
        SetCover(kEmptyString, file_system_view_page_->playlistPage());
        ConnectPlaylistPageSignal(file_system_view_page_->playlistPage());
    }

    if (!cd_page_) {
        auto playlist_id = kDefaultCdPlaylistId;
        if (!qMainDb.IsPlaylistExist(playlist_id)) {
            playlist_id = qMainDb.AddPlaylist(kEmptyString, 4);
        }
        cd_page_.reset(new CdPage(this));
        cd_page_->playlistPage()->playlist()->SetPlaylistId(playlist_id, kAppSettingCdPlaylistColumnName);
        cd_page_->playlistPage()->playlist()->SetHeaderViewHidden(false);
        cd_page_->playlistPage()->playlist()->SetOtherPlaylist(kDefaultPlaylistId);
        SetCover(kEmptyString, cd_page_->playlistPage());
        ConnectPlaylistPageSignal(cd_page_->playlistPage());
    }

    current_playlist_page_ = playlist_page_.get();

    (void)QObject::connect(this,
        &Xamp::BlurImage,
        background_worker_.get(),
        &BackgroundWorker::OnBlurImage);
#if defined(Q_OS_WIN)
    (void)QObject::connect(this,
        &Xamp::FetchCdInfo,
        background_worker_.get(),
        &BackgroundWorker::OnFetchCdInfo);
#endif
    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::OnReadCdTrackInfo,
        this,
        &Xamp::OnUpdateCdTrackInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::OnMbDiscInfo,
        this,
        &Xamp::OnUpdateMbDiscInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
		&BackgroundWorker::OnDiscCover,
        this,
        &Xamp::OnUpdateDiscCover,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_view_page_.get(),
        &FileSystemViewPage::addDirToPlaylist,
        this,
        &Xamp::AppendToPlaylist);    

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        album_page_.get(),
        &AlbumArtistPage::OnThemeColorChanged);

    if (!qMainDb.IsPlaylistExist(kDefaultAlbumPlaylistId)) {
        qMainDb.AddPlaylist(kEmptyString, 1);
    }

    ConnectPlaylistPageSignal(album_page_->album()->albumViewPage()->playlistPage());
    ConnectPlaylistPageSignal(album_page_->year()->albumViewPage()->playlistPage());

    (void)QObject::connect(this,
        &Xamp::SearchLyrics,
        background_worker_.get(),
        &BackgroundWorker::OnSearchLyrics);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::SearchLyricsCompleted,
        this,
        &Xamp::OnSearchLyricsCompleted);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::SearchArtistCompleted,
        this,
        &Xamp::OnSearchArtistCompleted);

    PushWidget(playlist_page_.get());
    PushWidget(lrc_page_.get());
    PushWidget(album_page_.get());
    PushWidget(file_system_view_page_.get());
    PushWidget(cd_page_.get());

    ui_.currentView->setCurrentIndex(0);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::ClickedArtist,
        this,
        &Xamp::OnArtistIdChanged);

    (void)QObject::connect(this,
        &Xamp::ThemeChanged,
        lrc_page_.get(),
        &LrcPage::OnThemeChanged);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::BlurImage,
        lrc_page_.get(),
        &LrcPage::SetBackground);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::DominantColor,
        lrc_page_->lyrics(),
        &LyricsShowWidget::SetLrcColor);

    (void)QObject::connect(album_page_->album(),
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
        emit ExtractFile(file_name, playlist_page_->playlist()->GetPlaylistId(), false);
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
	const auto last_dir = qAppSettings.ValueAsString(kAppSettingLastOpenFolderPath);
    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    GetSaveFileName(this,
        [this, item, profile](const auto &file_name) {
            const auto dialog = MakeProgressDialog(
                tr("Export progress dialog"),
                tr("Export '") + item.title + tr("' to aac file"),
                tr("Cancel"));

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
                read_until::EncodeFile(config,
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
        },
        tr("Save AAC file"),
        save_file_name,
        tr("AAC Files (*.m4a)"));   

    
}

void Xamp::EncodeWavFile(const PlayListEntity& item) {
    const auto last_dir = qAppSettings.ValueAsString(kAppSettingLastOpenFolderPath);
    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    GetSaveFileName(this,
        [this, item](const auto& file_name) {
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
                read_until::EncodeFile(item.file_path.toStdWString(),
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
        },
        tr("Save Wav file"),
        save_file_name,
        tr("Wav Files (*.wav)"));    
}

void Xamp::EncodeFlacFile(const PlayListEntity& item) {
    const auto last_dir = qAppSettings.ValueAsString(kAppSettingLastOpenFolderPath);
    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;

    GetSaveFileName(this,
        [item](const auto& file_name) {
            const auto dialog = MakeProgressDialog(
                tr("Export progress dialog"),
                tr("Export '") + item.title + tr("' to flac file"),
                tr("Cancel"));

            TrackInfo track_info;
            track_info.album = item.album.toStdWString();
            track_info.artist = item.artist.toStdWString();
            track_info.title = item.title.toStdWString();
            track_info.track = item.track;

            const auto command
                = qSTR("-%1 -V").arg(qAppSettings.GetValue(kFlacEncodingLevel).toInt()).toStdWString();

            try {
                auto encoder = StreamFactory::MakeFlacEncoder();
                read_until::EncodeFile(item.file_path.toStdWString(),
                    file_name.toStdWString(),
                    encoder,
                    command,
                    [&](auto progress) -> bool {
                        dialog->SetValue(progress);
                        qApp->processEvents();
                        return dialog->WasCanceled() != true;
                    }, track_info);
            }
            catch (Exception const& e) {
                XMessageBox::ShowError(qTEXT(e.what()));
            }
        },    
        tr("Save Flac file"),
        save_file_name,
        tr("FLAC Files (*.flac)"));    
}

void Xamp::ConnectPlaylistPageSignal(PlaylistPage* playlist_page) {
    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::AddPlaylistItemFinished,
        album_page_.get(),
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
        background_worker_.get(),
        &BackgroundWorker::OnReadReplayGain);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::ExtractFile,
        extract_file_worker_.get(),
        &ExtractFileWorker::OnExtractFile);
    
    (void)QObject::connect(background_worker_.get(),
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

void Xamp::OnInsertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id) {
    DatabaseFacade facede;
    (void)QObject::connect(&facede,
        &DatabaseFacade::FindAlbumCover,
        find_album_cover_worker_.get(),
        &FindAlbumCoverWorker::OnFindAlbumCover,
        Qt::QueuedConnection);
    facede.InsertTrackInfo(result, playlist_id);    
    emit Translation(GetStringOrEmptyString(result.front().artist), qTEXT("ja"), qTEXT("en"));
    playlist_page_->playlist()->Reload();
}

void Xamp::OnCalculateEta(uint64_t ms) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetTitle(qSTR("ETA %1").arg(FormatDuration(ms / 1000.0, true)));
}

void Xamp::OnReadFilePath(const QString& file_path) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetLabelText(file_path);
}

void Xamp::OnSetAlbumCover(int32_t album_id,
    const QString& album,
    const QString& cover_id) {
    XAMP_LOG_DEBUG("{} => {}", album.toStdString(), cover_id.toStdString());
    qMainDb.SetAlbumCover(album_id, album, cover_id);
    album_page_->Refresh();
}

void Xamp::OnTranslationCompleted(const QString& keyword, const QString& result) {
    qMainDb.UpdateArtistEnglishName(keyword, result);
}

void Xamp::OnReadFileProgress(int32_t progress) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->SetValue(progress);    
}

void Xamp::OnReadCompleted() {    
    album_page_->Refresh();
    playlist_page_->playlist()->Reload();
    current_playlist_page_->playlist()->Reload();
    file_system_view_page_->playlistPage()->playlist()->Reload();
    Delay(1);
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->close();
    read_progress_dialog_.reset();
}

void Xamp::OnFoundFileCount(size_t file_count) {
    constexpr auto kShowProgressDialogMsSecs = 50;

    if (!read_progress_dialog_
        && progress_timer_.elapsed() > kShowProgressDialogMsSecs) {
        read_progress_dialog_ = MakeProgressDialog(kApplicationTitle,
            tr("Read track information"),
            tr("Cancel"));

        (void)QObject::connect(read_progress_dialog_.get(),
            &XProgressDialog::CancelRequested, [this]() {
                extract_file_worker_->OnCancelRequested();
                find_album_cover_worker_->OnCancelRequested();
            });

        read_progress_dialog_->exec();
    }

    if (!read_progress_dialog_) {
        return;
    }

    read_progress_dialog_->SetTitle(qSTR("Total number of files %1").arg(file_count));
}

void Xamp::OnReadFileStart() {
    progress_timer_.restart();
}
