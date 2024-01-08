#include <FramelessHelper/Widgets/framelesswidgetshelper.h>
#include <xamp.h>

#include <QShortcut>
#include <QToolTip>
#include <QWidgetAction>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImageReader>
#include <QCoroFuture>

#include <QSimpleUpdater.h>
#include <set>

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
#include <widget/supereqview.h>
#include <widget/playlistpage.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlisttableview.h>
#include <widget/mbdiscid_uiltis.h>
#include <widget/spectrumwidget.h>
#include <widget/xdialog.h>
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
#include <widget/processindicator.h>
#include <widget/tageditpage.h>
#include <widget/xmessagebox.h>
#include <widget/playlisttabwidget.h>
#include <widget/youtubedl/ytmusic.h>

#include <thememanager.h>
#include <version.h>

namespace {
    void showMeMessage(const QString& message) {
        if (qAppSettings.dontShowMeAgain(message)) {
            auto [button, checked] = XMessageBox::showCheckBoxInformation(
                message,
                qTR("Ok, and don't show again."),
                kApplicationTitle,
                false);
            if (checked) {
                qAppSettings.addDontShowMeAgain(message);
            }
        }
    }

    void setShufflePlayOrder(Ui::XampWindow& ui) {
        ui.repeatButton->setIcon(qTheme.fontIcon(Glyphs::ICON_SHUFFLE_PLAY_ORDER));
    }

    void setRepeatOnePlayOrder(Ui::XampWindow& ui) {
        ui.repeatButton->setIcon(qTheme.fontIcon(Glyphs::ICON_REPEAT_ONE_PLAY_ORDER));
    }

    void setRepeatOncePlayOrder(Ui::XampWindow& ui) {
        ui.repeatButton->setIcon(qTheme.fontIcon(Glyphs::ICON_REPEAT_ONCE_PLAY_ORDER));
    }

    void setThemeIcon(Ui::XampWindow& ui) {
        qTheme.setTitleBarButtonStyle(ui.closeButton, ui.minWinButton, ui.maxWinButton);

        const QColor hover_color = qTheme.hoverColor();

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
        ui.stopButton->setIcon(qTheme.fontIcon(ICON_STOP_PLAY));

        ui.nextButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
        ui.nextButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_FORWARD));

        ui.prevButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
        ui.prevButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_BACKWARD));

        ui.selectDeviceButton->setStyleSheet(qTEXT(R"(
                                                QToolButton#selectDeviceButton {                                                
                                                border: none;
                                                background-color: transparent;                                                
                                                }
                                                QToolButton#selectDeviceButton::menu-indicator { image: none; }
                                                )"));
        ui.selectDeviceButton->setIcon(qTheme.fontIcon(Glyphs::ICON_SPEAKER));

        ui.mutedButton->setStyleSheet(qSTR(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/%1/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )").arg(qTheme.themeColorPath()));

        ui.eqButton->setStyleSheet(qTEXT(R"(
                                         QToolButton#eqButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
        ui.eqButton->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));

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
                                         )").arg(colorToString(qTheme.hoverColor())));

        ui.menuButton->setIcon(qTheme.fontIcon(Glyphs::ICON_MORE));
    }

    void setRepeatButtonIcon(Ui::XampWindow& ui, PlayerOrder order) {
        switch (order) {
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
            setRepeatOncePlayOrder(ui);
            break;
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
            setRepeatOnePlayOrder(ui);
            break;
        case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
            setShufflePlayOrder(ui);
            break;
        default:
            break;
        }
    }

    void setNaviBarTheme(TabListView* navi_bar) {
        QString tab_left_color;

        switch (qTheme.themeColor()) {
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

    void setWidgetStyle(Ui::XampWindow& ui) {
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

        if (qTheme.themeColor() == ThemeColor::DARK_THEME) {
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

        setNaviBarTheme(ui.naviBar);
        qTheme.setSliderTheme(ui.seekSlider);

        ui.deviceDescLabel->setStyleSheet(qTEXT("background: transparent;"));

        setThemeIcon(ui);
        ui.sliderFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));
        ui.sliderFrame2->setStyleSheet(qTEXT("background: transparent; border: none;"));
        ui.currentViewFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));
    }

    DsdModes getDsdModes(const DeviceInfo& device_info,
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
    , trigger_upgrade_restart_(false)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , main_window_(nullptr)
	, lrc_page_(nullptr)
	, music_page_(nullptr)
	, cd_page_(nullptr)
	, album_page_(nullptr)
	, file_system_view_page_(nullptr)
	, background_worker_(nullptr)
    , state_adapter_(std::make_shared<UIPlayerStateAdapter>())
	, player_(player) {
    ui_.setupUi(this);
}

Xamp::~Xamp() = default;

void Xamp::destroy() {
    if (player_ != nullptr) {
        player_->Destroy();
        player_.reset();
        XAMP_LOG_DEBUG("Player destroy!");
    }

    auto worker_cancel_requested = [](auto* worker) {
        if (worker != nullptr) {
            worker->cancelRequested();
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
    worker_cancel_requested(ytmusic_worker_.get());

    QCoro::connect(ytmusic_worker_->cleanupAsync(), this, [this](auto) {
        XAMP_LOG_DEBUG("Cleanup done!");
        });
    delay(2);    

    quit_and_wait_thread(background_thread_);
    quit_and_wait_thread(find_album_cover_thread_);
    quit_and_wait_thread(extract_file_thread_);
    quit_and_wait_thread(ytmusic_thread_);

    if (main_window_ != nullptr) {
        (void)main_window_->saveGeometry();
    }

    local_tab_widget_->saveTabOrder();

    XAMP_LOG_DEBUG("Xamp destory!");
}

void Xamp::setXWindow(IXMainWindow* main_window) {
    FramelessWidgetsHelper::get(this)->setTitleBarWidget(ui_.titleFrame);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.minWinButton, SystemButtonType::Minimize);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.maxWinButton, SystemButtonType::Maximize);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.closeButton, SystemButtonType::Close);
    FramelessWidgetsHelper::get(this)->setHitTestVisible(ui_.menuButton);

    main_window_ = main_window;
    order_ = qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder);

    (void)QObject::connect(ui_.minWinButton, &QToolButton::clicked, [this]() {
        main_window_->showMinimized();
        });

    (void)QObject::connect(ui_.maxWinButton, &QToolButton::clicked, [this]() {
        main_window_->updateMaximumState();
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

    ytmusic_worker_.reset(new YtMusic());    
    ytmusic_worker_->moveToThread(&ytmusic_thread_);
    ytmusic_thread_.start();

    QCoro::connect(ytmusic_worker_->initialAsync(), this, [this](auto) {
        XAMP_LOG_DEBUG("Initial done!");
        });

    player_->Startup(state_adapter_);
    player_->SetDelayCallback([this](auto seconds) {
        delay(seconds);
        });

    initialUi();
    initialController();
    initialPlaylist();
    initialShortcut();
    initialSpectrum();

    setPlaylistPageCover(nullptr, cd_page_->playlistPage());

    cd_page_->playlistPage()->hidePlaybackInformation(true);

    const auto tab_name = qAppSettings.valueAsString(kAppSettingLastTabName);
    const auto tab_id = ui_.naviBar->tabId(tab_name);
    if (tab_id != -1) {
        ui_.naviBar->setCurrentIndex(ui_.naviBar->model()->index(tab_id + 1, 0));
        setCurrentTab(tab_id);
    }
    else {
        ui_.naviBar->setCurrentIndex(ui_.naviBar->model()->index(0, 0));
        setCurrentTab(0);
    }

    (void)QObject::connect(this,
        &Xamp::translation,
        background_worker_.get(),
        &BackgroundWorker::onTranslation);

    (void)QObject::connect(background_worker_.get(), 
        &BackgroundWorker::translationCompleted,
        this,
        &Xamp::onTranslationCompleted);

    (void)QObject::connect(find_album_cover_worker_.get(),
        &FindAlbumCoverWorker::setAlbumCover,
        this, 
        &Xamp::onSetAlbumCover);

    (void)QObject::connect(&qTheme, 
        &ThemeManager::currentThemeChanged, 
        this, 
        &Xamp::onCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::currentThemeChanged,
        ui_.naviBar,
        &TabListView::onCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::currentThemeChanged,
        album_page_->album(),
        &AlbumView::onCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::currentThemeChanged,
        cd_page_.get(),
        &CdPage::onCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::currentThemeChanged,
        ui_.mutedButton,
        &VolumeButton::onCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::currentThemeChanged,
        lrc_page_.get(),
        &LrcPage::onCurrentThemeChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::currentThemeChanged,
        album_page_.get(),
        &AlbumArtistPage::onCurrentThemeChanged);

    setPlayerOrder();
    initialDeviceList();

    (void)QObject::connect(this,
        &Xamp::extractFile,
        extract_file_worker_.get(),
        &ExtractFileWorker::onExtractFile,
        Qt::QueuedConnection);

   (void)QObject::connect(album_page_->album(),
        &AlbumView::extractFile,
        extract_file_worker_.get(),
        &ExtractFileWorker::onExtractFile,
        Qt::QueuedConnection);

    // ExtractFileWorker

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::foundFileCount,
        this,
        &Xamp::onFoundFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::insertDatabase,
        this,
        &Xamp::onInsertDatabase,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::readFilePath,
        this,
        &Xamp::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::readFileStart,
        this,
        &Xamp::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::readCompleted,
        this,
        &Xamp::onReadCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(extract_file_worker_.get(),
        &ExtractFileWorker::readFileProgress,
        this,
        &Xamp::onReadFileProgress,
        Qt::QueuedConnection);

    // BackgroundWorker

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::foundFileCount,
        this,
        &Xamp::onFoundFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::readFilePath,
        this,
        &Xamp::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::readFileStart,
        this,
        &Xamp::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::readCompleted,
        this,
        &Xamp::onReadCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::readFileProgress,
        this,
        &Xamp::onReadFileProgress,
        Qt::QueuedConnection);

    auto* updater = QSimpleUpdater::getInstance();    

    (void)QObject::connect(updater,
        &QSimpleUpdater::appcastDownloaded,
        [updater, this](const QString& url, const QByteArray& reply) {
            const auto document = QJsonDocument::fromJson(reply);
            const auto updates = document.object().value(qTEXT("updates")).toObject();
            const auto platform = updates.value(kPlatformKey).toObject();
            const auto changelog = platform.value(qTEXT("changelog")).toString();
            const auto latest_version = platform.value(qTEXT("latest-version")).toString();

            Version latest_version_value;
            if (!parseVersion(latest_version, latest_version_value)) {
                return;
            }

            if (trigger_upgrade_action_) {
                //const auto download_url = platform.value(qTEXT("download-url")).toString();
                //XMessageBox::ShowInformation(changelog, qTEXT("New version!"), false);
            }

            if (latest_version_value > kApplicationVersionValue) {
                trigger_upgrade_action_ = true;
            }

            emit updateNewVersion(latest_version_value);
        });

    auto* menu = new XMenu();
    qTheme.setMenuStyle(menu);
    const auto * preference_action = menu->addAction(qTheme.fontIcon(Glyphs::ICON_SETTINGS), qTR("Preference"));
    (void)QObject::connect(preference_action, &QAction::triggered, [this]() {
        const QScopedPointer<XDialog> dialog(new XDialog(this));
        const QScopedPointer<PreferencePage> preference_page(new PreferencePage(dialog.get()));
        preference_page->loadSettings();
        dialog->setContentWidget(preference_page.get());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
        dialog->setTitle(qTR("Preference"));
        dialog->exec();
        preference_page->saveAll();
        });
   
    const auto* about_action = menu->addAction(qTheme.fontIcon(Glyphs::ICON_ABOUT),qTR("About"));
    (void)QObject::connect(about_action, &QAction::triggered, [this]() {
        const QScopedPointer<XDialog> dialog(new XDialog(this));
        const QScopedPointer<AboutPage> about_page(new AboutPage(dialog.get()));
        (void)QObject::connect(about_page.get(), &AboutPage::CheckForUpdate, this, &Xamp::onCheckForUpdate);
        (void)QObject::connect(about_page.get(), &AboutPage::RestartApp, this, &Xamp::onRestartApp);
        (void)QObject::connect(this, &Xamp::updateNewVersion, about_page.get(), &AboutPage::OnUpdateNewVersion);
        dialog->setContentWidget(about_page.get());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_ABOUT));
        dialog->setTitle(qTR("About"));
        emit about_page->CheckForUpdate();
        dialog->exec();
        });
    ui_.menuButton->setPopupMode(QToolButton::InstantPopup);
    ui_.menuButton->setMenu(menu);

    onCheckForUpdate();

    (void)QObject::connect(&ui_update_timer_timer_, &QTimer::timeout, this, &Xamp::performDelayedUpdate);
    ui_update_timer_timer_.setInterval(3000);
}

void Xamp::onRestartApp() {
    trigger_upgrade_restart_ = true;
    qApp->exit(kRestartExistCode);
}

void Xamp::onFetchPlaylistTrackCompleted(PlaylistPage* playlist_page, const std::vector<playlist::Track>& tracks) {
    TrackInfo track_info;

    std::wstring prev_album;
    auto track_no = 1;

    for (const auto& track : tracks) {
        track_info.title = String::ToString(track.title);

        track_info.album = kUnknownAlbum;
        if (track.album) {
            track_info.album = String::ToString(track.album.value().name);
        }

        if (prev_album != track_info.album.value()) {
            prev_album = track_info.album.value();
            track_no = 1;
        }
        track_info.track = track_no++;

        std::string thumbnail_url;
        if (!track.thumbnails.empty()) {
            thumbnail_url = track.thumbnails.back().url;
        }

        track_info.artist = kUnknownArtist;
        if (!track.artists.empty()) {
            track_info.artist = String::ToString(track.artists.front().name);
        }        

        if (track.duration) {
            track_info.duration = parseDuration(track.duration.value());
        }
        else {
            track_info.duration = 0;
        }

        if (!track.video_id) {
            continue;
        }

        QCoro::connect(ytmusic_worker_->extractVideoInfoAsync(QString::fromStdString(track.video_id.value())), this,
            [this, playlist_page, thumbnail_url, track_info](const auto& video_info) {
                onExtractVideoInfoCompleted(playlist_page, thumbnail_url, track_info, video_info);
            });
    }
}

void Xamp::onFetchAlbumCompleted(const album::Album& album) {
    TrackInfo track_info;
    track_info.album = String::ToString(album.title);
    int track_no = 1;
    std::string thumbnail_url;
    if (!album.thumbnails.empty()) {
        thumbnail_url = album.thumbnails.back().url;
    }

    for (const auto& track : album.tracks) {
        track_info.track = track_no++;
        track_info.title = String::ToString(track.title);

        XAMP_LOG_DEBUG("{} - {}.{}", album.title, track_info.track, track.title);

        if (!track.artists.empty()) {
            track_info.artist = String::ToString(track.artists.front().name);
        }
        else {
            track_info.artist = std::wstring(L"Unknown artist");
        }

        if (track.duration) {
            track_info.duration = parseDuration(track.duration.value());
        }
        else {
            track_info.duration = 0;
        }

        if (!track.video_id) {
            continue;
        }

        QCoro::connect(ytmusic_worker_->extractVideoInfoAsync(QString::fromStdString(track.video_id.value())), this,
            [this, thumbnail_url, track_info](const auto& video_info) {
                onExtractVideoInfoCompleted(cloud_search_page_.get(), thumbnail_url, track_info, video_info);
                cloud_search_page_->spinner()->stopAnimation();
            });
    }
}

void Xamp::onSearchCompleted(const std::vector<search::SearchResultItem>& result) {
    if (result.empty()) {
        cloud_search_page_->spinner()->stopAnimation();
        return;
    }

    for (auto& item : result) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, search::Album>) {
                if (!arg.browse_id) {
                    return;
                }
                QCoro::connect(ytmusic_worker_->fetchAlbumAsync(
                    QString::fromStdString(arg.browse_id.value())), this,
                    &Xamp::onFetchAlbumCompleted);
            }
            }, item);
    }
}

void Xamp::onSearchSuggestionsCompleted(const std::vector<std::string>& result) {
    for (const auto &text : result) {
        cloud_search_page_->addSuggestions(QString::fromStdString(text));
    }
    cloud_search_page_->showCompleter();
}

void Xamp::onExtractVideoInfoCompleted(PlaylistPage* playlist_page,
    const std::string& thumbnail_url,
    TrackInfo track_info,
    const video_info::VideoInfo& video_info) {
    if (video_info.formats.empty()) {
        return;
    }

    std::multiset<video_info::Format> formats;
    for (const auto& format : video_info.formats) {
        if (format.vcodec == "none" && format.acodec != "none") {
            formats.insert(format);
        }        
    }

    if (formats.empty()) {
        return;
    }

    const auto best_format = *formats.begin();

    track_info.file_path = String::ToString(best_format.url);
    if (track_info.album == kUnknownAlbum) {
        track_info.track = 1;
    }

    const ForwardList<TrackInfo> track_infos{ track_info };

    DatabaseFacade facede;
    facede.insertTrackInfo(track_infos, playlist_page->playlist()->playlistId(), [=, this](auto album_id) {
        if (thumbnail_url.empty()) {
            return;
        }
    	http::HttpClient(QString::fromStdString(thumbnail_url))
            .download([=, this](const auto& content) {
            QPixmap image;
            if (!image.loadFromData(content)) {
                return;
            }
            qMainDb.setAlbumCover(album_id, qImageCache.addImage(image));
            playlist_page->playlist()->reload();
        });
    });    
    playlist_page->playlist()->reload();
}

void Xamp::onCheckForUpdate() {
    auto* updater = QSimpleUpdater::getInstance();
    updater->setPlatformKey(kSoftwareUpdateUrl, kPlatformKey);
    updater->setModuleVersion(kSoftwareUpdateUrl, kApplicationVersion);
    updater->setUseCustomAppcast(kSoftwareUpdateUrl, true);
    updater->checkForUpdates(kSoftwareUpdateUrl);
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
    if (!qAppSettings.valueAsBool(kAppSettingEnableSpectrum)) {
        return;
    }

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::fftResultChanged,
        lrc_page_->spectrum(),
        &SpectrumWidget::onFftResultChanged,
        Qt::QueuedConnection);
}

void Xamp::updateMaximumState(bool is_maximum) {
    lrc_page_->setFullScreen(is_maximum);
    qTheme.updateMaximumIcon(ui_.maxWinButton, is_maximum);
}

void Xamp::closeEvent(QCloseEvent* event) {
    if (!trigger_upgrade_restart_ 
        && XMessageBox::showYesOrNo(qTR("Do you want to exit the XAMP ?")) == QDialogButtonBox::No) {
        event->ignore();
        return;
    }
    destroy();
    window()->close();
}

void Xamp::initialUi() {
    QFont f(qTEXT("DisplayFont"));
    f.setWeight(QFont::Bold);
    f.setPointSize(qTheme.fontSize(6));
    ui_.titleLabel->setFont(f);

    f.setWeight(QFont::Normal);
    f.setPointSize(qTheme.fontSize(6));
    ui_.artistLabel->setFont(f);    

    QFont format_font(qTEXT("FormatFont"));
    format_font.setWeight(QFont::Normal);
    format_font.setPointSize(qTheme.fontSize(6));
    ui_.formatLabel->setFont(format_font);

    QToolTip::hideText();

    f.setWeight(QFont::DemiBold);
    f.setPointSize(10);
    ui_.titleFrameLabel->setFont(f);
    ui_.titleFrameLabel->setText(kApplicationTitle);
    ui_.titleFrameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QFont mono_font(qTEXT("MonoFont"));
    mono_font.setPointSize(qTheme.defaultFontSize());
    ui_.startPosLabel->setFont(mono_font);
    ui_.endPosLabel->setFont(mono_font);
    ui_.seekSlider->setDisabled(true);

    ui_.mutedButton->setPlayer(player_);
    ui_.coverLabel->setAttribute(Qt::WA_StaticContents);
    qTheme.setPlayOrPauseButton(ui_.playButton, false);

    ui_.stopButton->hide();    
}

void Xamp::onVolumeChanged(float volume) {
    setVolume(static_cast<int32_t>(volume * 100.0f));
}

void Xamp::onDeviceStateChanged(DeviceState state) {
    XAMP_LOG_DEBUG("OnDeviceStateChanged: {}", state);

    if (state == DeviceState::DEVICE_STATE_REMOVED) {
        player_->Stop(true, true, true);
    }
    if (state == DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE) {
        return;
    }
    initialDeviceList();
}

QWidgetAction* Xamp::createDeviceMenuWidget(const QString& desc, const QIcon &icon) {
    auto* desc_label = new QLabel(desc);

    desc_label->setObjectName(qTEXT("textSeparator"));

    QFont f(qTEXT("DisplayFont"));
    f.setPointSize(qTheme.fontSize(8));
    f.setBold(true);
    desc_label->setFont(f);
    desc_label->setAlignment(Qt::AlignCenter);

    auto* device_type_frame = new QFrame();
    qTheme.setTextSeparator(device_type_frame);

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

void Xamp::waitForReady() {
    FramelessWidgetsHelper::get(this)->waitForReady();
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

    const auto device_type_id = qAppSettings.valueAsId(kAppSettingDeviceType);
    const auto device_id = qAppSettings.valueAsString(kAppSettingDeviceId).toStdString();
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
        menu->addAction(createDeviceMenuWidget(fromStdStringView(device_type->GetDescription())));
       
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
                qTheme.setDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info.connect_type);
                device_info_ = device_info;
                ui_.deviceDescLabel->setText(QString::fromStdWString(device_info_.value().name));
                qAppSettings.setValue(kAppSettingDeviceType, device_info_.value().device_type_id);
                qAppSettings.setValue(kAppSettingDeviceId, device_info_.value().device_id);
            };

            (void)QObject::connect(device_action, &QAction::triggered, trigger_callback);
            menu->addAction(device_action);

            if (device_type_id == device_info.device_type_id && device_id == device_info.device_id) {
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
        qTheme.setDeviceConnectTypeIcon(ui_.selectDeviceButton, device_info_.value().connect_type);
        device_id_action[device_info_.value().device_id]->setChecked(true);
        qAppSettings.setValue(kAppSettingDeviceType, device_info_.value().device_type_id);
        qAppSettings.setValue(kAppSettingDeviceId, device_info_.value().device_id);
        XAMP_LOG_DEBUG("Use default device Id : {}", device_info_.value().device_id);
    }
}

void Xamp::initialController() {    
    (void)QObject::connect(ui_.mutedButton, &QToolButton::clicked, [this]() {
        if (!player_->IsMute()) {
            setVolume(0);
        } else {
            setVolume(player_->GetVolume());
        }
    });

    qTheme.setHeartButton(ui_.heartButton);

    (void)QObject::connect(ui_.heartButton, &QToolButton::clicked, [this]() {
        if (!current_entity_) {
            return;
        }

        current_entity_.value().heart = !(current_entity_.value().heart);

        qMainDb.updateMusicHeart(current_entity_.value().music_id, current_entity_.value().heart);
        qTheme.setHeartButton(ui_.heartButton, current_entity_.value().heart);
        getCurrentPlaylistPage()->setHeart(current_entity_.value().heart);
        getCurrentPlaylistPage()->playlist()->reload();
        });

    (void)QObject::connect(ui_.seekSlider, &SeekSlider::leftButtonValueChanged, [this](auto value) {
        try {
			is_seeking_ = true;
			player_->Seek(value / 1000.0);
            qTheme.setPlayOrPauseButton(ui_.playButton, true);
            main_window_->setTaskbarPlayingResume();
        }
        catch (const Exception & e) {
            player_->Stop(false);
            XMessageBox::showError(qTEXT(e.GetErrorMessage()));
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

    if (qAppSettings.valueAsBool(kAppSettingIsMuted)) {
        qTheme.setMuted(ui_.mutedButton, true);
    }
    else {
        qTheme.setMuted(ui_.mutedButton, false);
    }

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

    (void)QObject::connect(ui_.nextButton, &QToolButton::clicked, [this]() {
        playNext();
    });

    (void)QObject::connect(ui_.prevButton, &QToolButton::clicked, [this]() {
        playPrevious();
    });

    (void)QObject::connect(ui_.eqButton, &QToolButton::clicked, [this]() {
        if (player_->GetDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->GetDsdModes() == DsdModes::DSD_MODE_NATIVE) {
            return;
        }
        //MaskWidget mask_widget(this);
        QScopedPointer<XDialog> dialog(new XDialog(this));        
        //QScopedPointer<SuperEqView> eq(new SuperEqView(dialog.get()));
        QScopedPointer<EqualizerView> eq(new EqualizerView(dialog.get()));
        dialog->setContentWidget(eq.get(), false);
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
        //dialog->setTitle(qTR("SuperEQ"));
        dialog->setTitle(qTR("EQ"));
        dialog->exec();
    });

    (void)QObject::connect(ui_.repeatButton, &QToolButton::clicked, [this]() {
        order_ = getNextOrder(order_);
        setPlayerOrder(true);
    });

    (void)QObject::connect(ui_.playButton, &QToolButton::clicked, [this]() {
        playOrPause();
    });

    (void)QObject::connect(ui_.stopButton, &QToolButton::clicked, [this]() {
        stopPlay();
    });

    (void)QObject::connect(ui_.artistLabel, &ClickableLabel::clicked, [this]() {
        if (current_entity_) {
            onArtistIdChanged(current_entity_.value().artist,
                current_entity_.value().cover_id,
                current_entity_.value().artist_id);
        }        
    });

    (void)QObject::connect(ui_.coverLabel, &ClickableLabel::clicked, [this]() {
        ui_.currentView->setCurrentWidget(lrc_page_.get());
    });

    (void)QObject::connect(ui_.naviBar, &TabListView::clickedTable, [this](auto table_id) {
        setCurrentTab(table_id);
        qAppSettings.setValue(kAppSettingLastTabName, ui_.naviBar->tabName(table_id));
    });

    ui_.seekSlider->setEnabled(false);
    ui_.startPosLabel->setText(formatDuration(0));
    ui_.endPosLabel->setText(formatDuration(0));
}

void Xamp::setCurrentTab(int32_t table_id) {
    switch (table_id) {
    case TAB_MUSIC_LIBRARY:
        album_page_->reload();
        ui_.currentView->setCurrentWidget(album_page_.get());
        break;
    case TAB_FILE_EXPLORER:
        ui_.currentView->setCurrentWidget(file_system_view_page_.get());
        break;
    case TAB_PLAYLIST:        
        ui_.currentView->setCurrentWidget(local_tab_widget_.get());
        ensureOnePlaylistPage();
        getCurrentPlaylistPage()->playlist()->reload();
        break;
    case TAB_LYRICS:
        ui_.currentView->setCurrentWidget(lrc_page_.get());
        break;
    case TAB_CD:
        ui_.currentView->setCurrentWidget(cd_page_.get());
        break;
    case TAB_YT_MUSIC:
        ui_.currentView->setCurrentWidget(cloud_search_page_.get());
        break;
    case TAB_YT_MUSIC_PLAYLIST:
        ui_.currentView->setCurrentWidget(cloud_tab_widget_.get());
        break;
    }
}

void Xamp::updateButtonState() {
    qTheme.setPlayOrPauseButton(ui_.playButton, player_->GetState() != PlayerState::PLAYER_STATE_PAUSED);
}

void Xamp::onCurrentThemeChanged(ThemeColor theme_color) {
	switch (theme_color) {
	case ThemeColor::DARK_THEME:
        qTheme.setThemeColor(ThemeColor::DARK_THEME);		
        break;
    case ThemeColor::LIGHT_THEME:
        qTheme.setThemeColor(ThemeColor::LIGHT_THEME);
        break;
	}

    Q_FOREACH(QFrame *frame, device_type_frame_) {
        qTheme.setTextSeparator(frame);
	}

    if (qAppSettings.valueAsBool(kAppSettingIsMuted)) {
        qTheme.setMuted(ui_.mutedButton, true);
    }
    else {
        qTheme.setMuted(ui_.mutedButton, false);
    }

    qTheme.loadAndApplyTheme();
    setThemeIcon(ui_);
    setRepeatButtonIcon(ui_, order_);
    setThemeColor(qTheme.backgroundColor(), qTheme.themeTextColor());

    lrc_page_->setCover(qTheme.unknownCover());
    local_tab_widget_->onCurrentThemeChanged(theme_color);
    local_tab_widget_->setPlaylistCover(qTheme.unknownCover());

    setPlaylistPageCover(nullptr, getCurrentPlaylistPage());
    setPlaylistPageCover(nullptr, cd_page_->playlistPage());

    emit currentThemeChanged(theme_color);
}

void Xamp::setThemeColor(QColor background_color, QColor color) {
    qTheme.setBackgroundColor(background_color);
    setWidgetStyle(ui_);
    updateButtonState();
    emit themeChanged(background_color, color);
}

void Xamp::onSearchArtistCompleted(const QString& artist, const QByteArray& image) {
    QPixmap cover;
    if (cover.loadFromData(image)) {        
        qMainDb.updateArtistCoverId(qMainDb.addOrUpdateArtist(artist), qImageCache.addImage(cover));
    }
    emit translation(artist, qTEXT("ja"), qTEXT("en"));    
    album_page_->reload();
}

void Xamp::onSearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics) {
    lrc_page_->lyrics()->onSetLrc(lyrics, trlyrics);
    qMainDb.addOrUpdateLyrc(music_id, lyrics, trlyrics);
}

void Xamp::setFullScreen() {
    // TODO ...
    /*if (qAppSettings.valueAsBool(kAppSettingEnterFullScreen)) {
        setCurrentTab(TAB_LYRICS);
        ui_.bottomFrame->setHidden(true);
        ui_.sliderFrame->setHidden(true);
        ui_.titleFrame->setHidden(true);
        ui_.verticalSpacer_3->changeSize(0, 0);
        lrc_page_->setFullScreen(true);
        qAppSettings.setValue<bool>(kAppSettingEnterFullScreen, false);
    }
    else {
        ui_.bottomFrame->setHidden(false);
        ui_.sliderFrame->setHidden(false);
        ui_.titleFrame->setHidden(false);
        ui_.verticalSpacer_3->changeSize(20, 15, QSizePolicy::Minimum, QSizePolicy::Fixed);
        lrc_page_->setFullScreen(false);
        qAppSettings.setValue<bool>(kAppSettingEnterFullScreen, true);
    }*/
}

void Xamp::shortcutsPressed(const QKeySequence& shortcut) {
    XAMP_LOG_DEBUG("shortcutsPressed: {}", shortcut.toString().toStdString());

	const QMap<QKeySequence, std::function<void()>> shortcut_map {
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
        },
        {
            QKeySequence(Qt::Key_F11),
            [this]() {
                setFullScreen();
            },
        }
    };

    const auto key = shortcut_map.value(shortcut);
    if (key != nullptr) {
        key();
    }
}

void Xamp::setVolume(uint32_t volume) {
    qAppSettings.setValue(kAppSettingVolume, volume);
    qTheme.setMuted(ui_.mutedButton, volume == 0);
    ui_.mutedButton->onVolumeChanged(volume);
    ui_.mutedButton->showDialog();
}

void Xamp::initialShortcut() {
    const auto* space_key = new QShortcut(QKeySequence(Qt::Key_Space), this);
    (void)QObject::connect(space_key, &QShortcut::activated, [this]() {
        playOrPause();
    });
}

void Xamp::stopPlay() {
    player_->Stop(true, true);
    setSeekPosValue(0);
    lrc_page_->spectrum()->reset();
    ui_.seekSlider->setEnabled(false);
    getCurrentPlaylistPage()->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
    album_page_->album()->albumViewPage()->playlistPage()->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
    qTheme.setPlayOrPauseButton(ui_.playButton, true); // note: Stop play must set true.
    current_entity_ = std::nullopt;
    main_window_->setTaskbarPlayerStop();
}

void Xamp::playNext() {
    if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED 
        || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
        playNextItem(1);
    } else {
        stopPlay();
    }
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

void Xamp::deleteKeyPress() {
    if (!ui_.currentView->count()) {
        return;
    }
    auto* playlist_view = getCurrentPlaylistPage()->playlist();
    playlist_view->removeSelectItems();
}

void Xamp::setPlayerOrder(bool emit_order) {
    setRepeatButtonIcon(ui_, order_);
    if (emit_order) {
        emit changePlayerOrder(order_);
    }
    qAppSettings.setEnumValue(kAppSettingOrder, order_);
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
    ui_.endPosLabel->setText(formatDuration(player_->GetDuration() - stream_time, full_text));
    const auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
    ui_.seekSlider->setValue(stream_time_as_ms);
    ui_.startPosLabel->setText(formatDuration(stream_time, full_text));
    main_window_->setTaskbarProgress(static_cast<int32_t>(100.0 * ui_.seekSlider->value() / ui_.seekSlider->maximum()));
    lrc_page_->lyrics()->onSetLrcTime(stream_time_as_ms);
}

void Xamp::playLocalFile(const PlayListEntity& item) {
    main_window_->setTaskbarPlayerPlaying();
    onPlayPlayListEntity(item);
}

void Xamp::playOrPause() {
    XAMP_LOG_DEBUG("Player state:{}", player_->GetState());

    auto tab_index = ui_.currentView->currentIndex();
    PlaylistPage* page = nullptr;
    PlaylistTabWidget* tab = nullptr;

    switch (tab_index) {
    case TAB_PLAYLIST:
        page = dynamic_cast<PlaylistPage*>(local_tab_widget_->currentWidget());
        tab = local_tab_widget_.get();
        break;
    case TAB_YT_MUSIC:
        page = dynamic_cast<PlaylistPage*>(ui_.currentView->currentWidget());
        break;
    case TAB_YT_MUSIC_PLAYLIST:
        page = dynamic_cast<PlaylistPage*>(cloud_tab_widget_->currentWidget());
        tab = cloud_tab_widget_.get();
        break;
    }

    try {
        if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
            qTheme.setPlayOrPauseButton(ui_.playButton, false);
            player_->Pause();
            if (page != nullptr) {
                page->playlist()->setNowPlayState(PlayingState::PLAY_PAUSE);
            }
            if (tab != nullptr) {
                tab->setPlaylistTabIcon(qTheme.playlistPauseIcon(PlaylistTabWidget::kTabIconSize, 1.0));
            }
            main_window_->setTaskbarPlayerPaused();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
            qTheme.setPlayOrPauseButton(ui_.playButton, true);
            player_->Resume();
            if (page != nullptr) {
                page->playlist()->setNowPlayState(PlayingState::PLAY_PLAYING);
            }
            if (tab != nullptr) {
                tab->setPlaylistTabIcon(qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.0));
            }
            main_window_->setTaskbarPlayingResume();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
            if (!ui_.currentView->count()) {
                return;
            }
            if (!page) {
                return;
            }
            if (const auto select_item = page->playlist()->selectItem()) {
                play_index_ = select_item.value();
            }
            play_index_ = page->playlist()->model()->index(
                play_index_.row(), PLAYLIST_PLAYING);
            if (play_index_.row() == -1) {
                XMessageBox::showInformation(qTR("Not found any playing item."));
                return;
            }
            page->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
            page->playlist()->setNowPlaying(play_index_, true);
            page->playlist()->onPlayIndex(play_index_);
        }
    }
    catch (const Exception& e) {
        XAMP_LOG_DEBUG(e.GetStackTrace());
    }
    catch (const std::exception& e) {
        XAMP_LOG_DEBUG(e.what());
    }
    catch (...) {	    
    }
}

void Xamp::resetSeekPosValue() {
    ui_.seekSlider->setValue(0);
    ui_.startPosLabel->setText(formatDuration(0));
}

void Xamp::setupDsp(const PlayListEntity& item) const {
    if (qAppSettings.valueAsBool(kAppSettingEnableEQ)) {
        if (qAppSettings.contains(kAppSettingEQName)) {
            const auto [name, settings] =
                qAppSettings.eqSettings();
            player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, settings);
            player_->GetDspManager()->AddEqualizer();
        }
    }
    else {
        player_->GetDspManager()->RemoveEqualizer();
    }

    if (player_->GetDsdModes() == DsdModes::DSD_MODE_PCM) {
        CompressorParameters parameters;
        if (qAppSettings.valueAsBool(kAppSettingEnableReplayGain)) {
            if (item.track_loudness != 0.0) {
                parameters.gain = Ebur128Reader::GetEbur128Gain(item.track_loudness, -1.0) * -1;
            }
        }
        player_->GetDspConfig().AddOrReplace(DspConfig::kCompressorParameters, parameters);
        player_->GetDspManager()->AddCompressor();
    }
    
    if (qAppSettings.valueAsBool(kAppSettingEnableReplayGain)) {
        const auto mode = qAppSettings.valueAsEnum<ReplayGainMode>(kAppSettingReplayGainMode);
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

QString Xamp::translateErrorCode(const Errors error) const {
    return fromStdStringView(EnumToString(error));
}

void Xamp::setupSampleWriter(ByteFormat byte_format,
    PlaybackFormat& playback_format) const {
    player_->GetDspManager()->SetSampleWriter();
    player_->PrepareToPlay(byte_format);
    playback_format = getPlaybackFormat(player_.get());
}

void Xamp::showEvent(QShowEvent* event) {
    IXFrame::showEvent(event);
}

void Xamp::setupSampleRateConverter(std::function<void()>& initial_sample_rate_converter,
    uint32_t &target_sample_rate,
    QString& sample_rate_converter_type) const {
    sample_rate_converter_type = qAppSettings.valueAsString(kAppSettingResamplerType);

    if (!qAppSettings.valueAsBool(kAppSettingResamplerEnable)) {
        return;
    }

    if (sample_rate_converter_type == kSoxr || sample_rate_converter_type.isEmpty()) {
        QMap<QString, QVariant> soxr_settings;
        const auto setting_name = qAppSettings.valueAsString(kAppSettingSoxrSettingName);
        soxr_settings = JsonSettings::valueAs(kSoxr).toMap()[setting_name].toMap();
        target_sample_rate = soxr_settings[kResampleSampleRate].toUInt();

        initial_sample_rate_converter = [this, soxr_settings]() {
            player_->GetDspManager()->AddPreDSP(makeSoxrSampleRateConverter(soxr_settings));
        };
    }
    else if (sample_rate_converter_type == kR8Brain) {
        auto config = JsonSettings::valueAsMap(kR8Brain);
        target_sample_rate = config[kResampleSampleRate].toUInt();

        initial_sample_rate_converter = [this]() {
            player_->GetDspManager()->AddPreDSP(makeR8BrainSampleRateConverter());
        };
    }
    else if (sample_rate_converter_type == kSrc) {
        auto config = JsonSettings::valueAsMap(kSrc);
        target_sample_rate = config[kResampleSampleRate].toUInt();

        initial_sample_rate_converter = [this]() {
            player_->GetDspManager()->AddPreDSP(makeSrcSampleRateConverter());
            };
    }
}

void Xamp::performDelayedUpdate() {
    cd_page_->playlistPage()->playlist()->reload();
    album_page_->reload();
    cloud_search_page_->playlist()->reload();
    if (local_tab_widget_->count() > 0) {
        getCurrentPlaylistPage()->playlist()->reload();
    }
}

void Xamp::onPlayEntity(const PlayListEntity& entity) {
    if (!device_info_) {
        showMeMessage(qTR("Not found any audio device, please check your audio device."));
        return;
    }

    lrc_page_->spectrum()->reset();

    auto open_done = false;

    ui_.seekSlider->setEnabled(true);

    XAMP_ON_SCOPE_EXIT(
        if (open_done) {
            return;
        }
        current_entity_ = std::nullopt;
        resetSeekPosValue();
        ui_.seekSlider->setEnabled(false);
        player_->Stop(false, true);
        );

    QString sample_rate_converter_type;
    PlaybackFormat playback_format;
    uint32_t target_sample_rate = 0;
    auto byte_format = ByteFormat::SINT32;
    std::function<void()> sample_rate_converter_factory;

    player_->Stop();
    player_->EnableFadeOut(qAppSettings.valueAsBool(kAppSettingEnableFadeOut));

    setupSampleRateConverter(sample_rate_converter_factory,
        target_sample_rate,
        sample_rate_converter_type);

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
                .arg(formatSampleRate(entity.sample_rate))
                .arg(formatSampleRate(default_format.GetSampleRate()));
            showMeMessage(message);
            player_->GetDspManager()->RemoveSampleRateConverter();
            target_sample_rate = default_format.GetSampleRate();
            sample_rate_converter_type = kSoxr;
            player_->GetDspManager()->AddPreDSP(makeSampleRateConverter(target_sample_rate));
        }
        byte_format = ByteFormat::SINT16;
    }
    
    const auto open_dsd_mode = getDsdModes(device_info_.value(),
        entity.file_path.toStdWString(),
        entity.sample_rate,
        target_sample_rate);

    try {
        player_->Open(entity.file_path.toStdWString(),
            device_info_.value(),
            target_sample_rate,
            open_dsd_mode);

        if (sample_rate_converter_factory != nullptr) {
            if (player_->GetInputFormat().GetSampleRate() == target_sample_rate) {
                player_->GetDspManager()->RemoveSampleRateConverter();
            }
            else {
                sample_rate_converter_factory();
            }
        }

        setupDsp(entity);
        setupSampleWriter(byte_format, playback_format);

        playback_format.bit_rate = entity.bit_rate;
        if (sample_rate_converter_type == kR8Brain) {
            player_->SetReadSampleSize(kR8brainBufferSize);
        }

        if (player_->GetDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->GetDsdModes() == DsdModes::DSD_MODE_NATIVE) {
            showMeMessage(qTR("Play DSD file need set 100% volume."));
            player_->SetVolume(100);
        }

        player_->BufferStream();
        open_done = true;
    }
    catch (const Exception & e) {        
        XMessageBox::showError(QString::fromUtf8(e.GetErrorMessage()));
        XAMP_LOG_DEBUG(e.GetStackTrace());
    }
    catch (const std::exception & e) {
        XMessageBox::showError(qTEXT(e.what()));
    }
    catch (...) {        
        XMessageBox::showError(qTR("Unknown error"));
    }

    if (open_done) {
        updateUi(entity, playback_format, open_done);
    }
    else {
        dynamic_cast<PlaylistPage*>(sender())->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
    }
}

void Xamp::ensureOnePlaylistPage() {
	if (local_tab_widget_->count() == 0) {
		const auto playlist_id = qMainDb.addPlaylist(qTR("Playlist"), 1, StoreType::LOCAL_STORE);
		newPlaylistPage(local_tab_widget_.get(), playlist_id, qTR("Playlist"));
	}
}

void Xamp::updateUi(const PlayListEntity& item, const PlaybackFormat& playback_format, bool open_done) {
    QString ext = item.file_extension;
    if (item.file_extension.isEmpty()) {
        ext = qTEXT(".m4a");
    }
	
    qTheme.setPlayOrPauseButton(ui_.playButton, open_done);

    const int64_t max_duration_ms = Round(player_->GetDuration()) * 1000;
    ui_.seekSlider->setRange(0, max_duration_ms - 1000);
    ui_.seekSlider->setValue(0);

    ensureOnePlaylistPage();

    auto* playlist_page = qobject_cast<PlaylistPage*>(sender());
    if (playlist_page != nullptr) {
        onSetCover(item.cover_id, playlist_page);
    }
    else {
        playlist_page = getCurrentPlaylistPage();
    }
    
    if (last_play_list_ != nullptr) {
        if (last_play_list_ != getCurrentPlaylistPage()->playlist()) {
            last_play_list_->removePlaying();
        }
    }

    last_play_list_ = playlist_page->playlist();
    playlist_page->format()->setText(format2String(playback_format, ext));
    playlist_page->title()->setText(item.title);
    playlist_page->setHeart(item.heart);

    album_page_->album()->setPlayingAlbumId(item.album_id);
    updateButtonState();

    const QFontMetrics title_metrics(ui_.titleLabel->font());
    const QFontMetrics artist_metrics(ui_.artistLabel->font());
    
    ui_.titleLabel->setText(title_metrics.elidedText(item.title, Qt::ElideRight, ui_.titleLabel->width()));
    ui_.artistLabel->setText(artist_metrics.elidedText(item.artist, Qt::ElideRight, ui_.artistLabel->width()));
    ui_.formatLabel->setText(format2String(playback_format, ext));
    
    lrc_page_->lyrics()->loadLrcFile(item.file_path);	
    lrc_page_->title()->setText(item.title);
    lrc_page_->album()->setText(item.album);
    lrc_page_->artist()->setText(item.artist);
    lrc_page_->format()->setText(format2String(playback_format, ext));
    lrc_page_->spectrum()->setFftSize(state_adapter_->fftSize());
    lrc_page_->spectrum()->setSampleRate(playback_format.output_format.GetSampleRate());

    auto lyrc_opt = qMainDb.getLyrc(item.music_id);
    if (!lyrc_opt) {
        emit searchLyrics(item.music_id, item.title, item.artist);
    } else {
        onSearchLyricsCompleted(item.music_id, std::get<0>(lyrc_opt.value()), std::get<1>(lyrc_opt.value()));
    }

    qTheme.setHeartButton(ui_.heartButton, current_entity_.value().heart);

    local_tab_widget_->setPlaylistTabIcon(qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.0));

    player_->Play();
}

void Xamp::onUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info) {
    const auto disc_id = QString::fromStdString(mb_disc_id_info.disc_id);
    const auto album = QString::fromStdWString(mb_disc_id_info.album);
    const auto artist = QString::fromStdWString(mb_disc_id_info.artist);
    
    if (!album.isEmpty()) {
        qMainDb.updateAlbumByDiscId(disc_id, album);
    }
    if (!artist.isEmpty()) {
        qMainDb.updateArtistByDiscId(disc_id, artist);
    }

	const auto album_id = qMainDb.getAlbumIdByDiscId(disc_id);

    if (!mb_disc_id_info.tracks.empty()) {
        QList<PlayListEntity> entities;
        qMainDb.forEachAlbumMusic(album_id, [&entities](const auto& entity) {
            entities.append(entity);
            });
        std::ranges::sort(entities, [](const auto& a, const auto& b) {
            return b.track > a.track;
            });
        auto i = 0;
        Q_FOREACH(const auto track, mb_disc_id_info.tracks) {
            qMainDb.updateMusicTitle(entities[i++].music_id, QString::fromStdWString(track.title));
        }
    }    

    if (!album.isEmpty()) {
        cd_page_->playlistPage()->title()->setText(album);
        cd_page_->playlistPage()->playlist()->reload();
    }

    if (const auto album_stats = qMainDb.getAlbumStats(album_id)) {
        cd_page_->playlistPage()->format()->setText(qTR("%1 Songs, %2, %3")
            .arg(QString::number(album_stats.value().songs))
            .arg(formatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year)));
    }
}

void Xamp::onUpdateDiscCover(const QString& disc_id, const QString& cover_id) {
	const auto album_id = qMainDb.getAlbumIdByDiscId(disc_id);
    qMainDb.setAlbumCover(album_id, cover_id);
}

void Xamp::onUpdateCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos) {
    const auto album_id = qMainDb.getAlbumIdByDiscId(disc_id);
    qMainDb.removeAlbum(album_id);
    cd_page_->playlistPage()->playlist()->removeAll();
    DatabaseFacade facade;
    facade.insertTrackInfo(track_infos, kDefaultCdPlaylistId);    
    emit translation(getStringOrEmptyString(track_infos.front().artist), qTEXT("ja"), qTEXT("en"));
    cd_page_->playlistPage()->playlist()->reload();
    cd_page_->showPlaylistPage(true);
}

void Xamp::drivesChanges(const QList<DriveInfo>& drive_infos) {
    cd_page_->playlistPage()->playlist()->removeAll();
    cd_page_->playlistPage()->playlist()->reload();
    emit fetchCdInfo(drive_infos.first());
}

void Xamp::drivesRemoved(const DriveInfo& /*drive_info*/) {
    cd_page_->playlistPage()->playlist()->removeAll();
    cd_page_->playlistPage()->playlist()->reload();
    cd_page_->showPlaylistPage(false);
}

void Xamp::onSetCover(const QString& cover_id, PlaylistPage* page) {
    auto found_cover = false;
    const auto cover = qImageCache.getOrDefault(cover_id, true);

    if (!cover_id.isEmpty() && cover_id != qImageCache.unknownCoverId()) {
        found_cover = true;
    }

    if (!found_cover) {
        setPlaylistPageCover(nullptr, page);
    } else {
        setPlaylistPageCover(&cover, page);
        emit blurImage(cover_id, cover, QSize(500, 500));
    }

    if (lrc_page_ != nullptr) {
        lrc_page_->clearBackground();
        lrc_page_->setCover(image_utils::resizeImage(cover, lrc_page_->cover()->size(), true));
        lrc_page_->addCoverShadow(found_cover);        
    }

    main_window_->setIconicThumbnail(cover);
}

void Xamp::onPlayPlayListEntity(const PlayListEntity& entity) {    
    main_window_->setTaskbarPlayerPlaying();
    current_entity_ = entity;
    onPlayEntity(entity);
    update();
}

void Xamp::playNextItem(int32_t forward) {
    auto* playlist_view = getCurrentPlaylistPage()->playlist();
    const auto count = playlist_view->model()->rowCount();
    if (count == 0) {
        stopPlay();
        XMessageBox::showInformation(qTR("Not found any playing item."));
        return;
    }    

    try {
        playlist_view->play(order_);
        play_index_ = playlist_view->currentIndex();
    }
    catch (Exception const& e) {
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
    }
}

void Xamp::onArtistIdChanged(const QString& artist, const QString& /*cover_id*/, int32_t artist_id) {    
    ui_.currentView->setCurrentWidget(album_page_.get());
}

void Xamp::onAddPlaylistItem(const QList<int32_t>& music_ids, const QList<PlayListEntity> & entities) {
    ensureOnePlaylistPage();
    const auto *playlist_view = getCurrentPlaylistPage()->playlist();
    qMainDb.addMusicToPlaylist(music_ids, playlist_view->playlistId());
    emit changePlayerOrder(order_);
}

void Xamp::setPlaylistPageCover(const QPixmap* cover, PlaylistPage* page) {
    if (!cover) {
        cover = &qTheme.unknownCover();
    }

	if (!page) {
        page = getCurrentPlaylistPage();
	}

    const QSize cover_size(ui_.coverLabel->size().width() - image_utils::kPlaylistImageRadius,
        ui_.coverLabel->size().height() - image_utils::kPlaylistImageRadius);
    const auto ui_cover = image_utils::roundImage(
        image_utils::resizeImage(*cover, cover_size, false),
        image_utils::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);
    page->setCover(cover);
}

void Xamp::onPlayerStateChanged(xamp::player::PlayerState play_state) {
    if (!player_) {
        return;
    }

    if (play_state == PlayerState::PLAYER_STATE_STOPPED) {
        main_window_->resetTaskbarProgress();
        lrc_page_->spectrum()->reset();
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(formatDuration(0));        
        playNextItem(1);
        payNextMusic();
    }
}

PlaylistPage* Xamp::newPlaylistPage(PlaylistTabWidget *tab_widget, int32_t playlist_id, const QString& name) {
    auto* playlist_page = createPlaylistPage(playlist_id, kAppSettingPlaylistColumnName);
    playlist_page->playlist()->setHeaderViewHidden(false);    
    connectPlaylistPageSignal(playlist_page);
    onSetCover(kEmptyString, playlist_page);
    tab_widget->createNewTab(name, playlist_page);
    return playlist_page;
}

void Xamp::initialPlaylist() {
    lrc_page_.reset(new LrcPage(this));
    album_page_.reset(new AlbumArtistPage(this));
    local_tab_widget_.reset(new PlaylistTabWidget(this));
    cloud_search_page_.reset(new PlaylistPage(this));

    cloud_search_page_->playlist()->setPlayListGroup(PLAYLIST_GROUP_ALBUM);
    cloud_search_page_->playlist()->enableCloudMode(true);
    cloud_search_page_->hidePlaybackInformation(true);
    cloud_tab_widget_.reset(new PlaylistTabWidget(this));
    cloud_tab_widget_->setStoreType(StoreType::CLOUD_STORE);

    ui_.naviBar->addSeparator();
    ui_.naviBar->addTab(qTR("Playlist"), TAB_PLAYLIST, qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
    ui_.naviBar->addTab(qTR("File explorer"), TAB_FILE_EXPLORER, qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_.naviBar->addTab(qTR("Lyrics"), TAB_LYRICS, qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
    ui_.naviBar->addTab(qTR("Library"), TAB_MUSIC_LIBRARY, qTheme.fontIcon(Glyphs::ICON_MUSIC_LIBRARY));
    ui_.naviBar->addTab(qTR("CD"), TAB_CD, qTheme.fontIcon(Glyphs::ICON_CD));
    ui_.naviBar->addTab(qTR("YouTube Search"), TAB_YT_MUSIC, qTheme.fontIcon(Glyphs::ICON_YOUTUBE));
    ui_.naviBar->addTab(qTR("YouTube Playlist"), TAB_YT_MUSIC_PLAYLIST, qTheme.fontIcon(Glyphs::ICON_YOUTUBE_LIBRARY));

    qMainDb.forEachPlaylist([this](auto playlist_id,
        auto index,
        auto store_type,
        const auto& name) {
            if (playlist_id == kDefaultAlbumPlaylistId
                || playlist_id == kDefaultCdPlaylistId
                || playlist_id == kDefaultYtMusicPlaylistId) {
                return;
            }
            if (store_type == StoreType::LOCAL_STORE) {
                newPlaylistPage(local_tab_widget_.get(), playlist_id, name);
            }
            else if (store_type == StoreType::CLOUD_STORE) {
                auto *playlist_page = newPlaylistPage(cloud_tab_widget_.get(), playlist_id, name);
                playlist_page->hidePlaybackInformation(true);
                playlist_page->playlist()->setPlayListGroup(PLAYLIST_GROUP_ALBUM);
                playlist_page->playlist()->enableCloudMode(true);
                playlist_page->playlist()->reload();
            }
        });

    if (cloud_tab_widget_->count() == 0) {
        QCoro::connect(ytmusic_worker_->fetchLibraryPlaylistsAsync(), this, [this](const auto& playlists) {
        	XAMP_LOG_DEBUG("Get library playlist done!");
            int32_t index = 1;
            for (const auto& playlist : playlists) {
                const auto playlist_id = qMainDb.addPlaylist(QString::fromStdString(playlist.title), index++, StoreType::CLOUD_STORE);
                auto* playlist_page = newPlaylistPage(cloud_tab_widget_.get(), playlist_id, QString::fromStdString(playlist.title));
                playlist_page->hidePlaybackInformation(true);
                QCoro::connect(ytmusic_worker_->fetchPlaylistAsync(QString::fromStdString(playlist.playlistId)),
                    this, [this, playlist_page](const auto& playlist) {
                        XAMP_LOG_DEBUG("Get playlist done!");
                        onFetchPlaylistTrackCompleted(playlist_page, playlist.tracks);
                    });
            }
            cloud_tab_widget_->setCurrentIndex(0);
            });
    } else {
        cloud_tab_widget_->setCurrentIndex(0);
    }     

    if (local_tab_widget_->tabBar()->count() == 0) {
	    const auto playlist_id = qMainDb.addPlaylist(qTR("Playlist"), 1, StoreType::LOCAL_STORE);
        newPlaylistPage(local_tab_widget_.get(), playlist_id, qTR("Playlist"));
    }
    if (!qMainDb.isPlaylistExist(kDefaultAlbumPlaylistId)) {
        qMainDb.addPlaylist(qTR("Playlist"), 1, StoreType::LOCAL_STORE);
    }
    if (!qMainDb.isPlaylistExist(kDefaultCdPlaylistId)) {
        qMainDb.addPlaylist(qTR("Playlist"), 1, StoreType::LOCAL_STORE);
    }
    if (!qMainDb.isPlaylistExist(kDefaultYtMusicPlaylistId)) {
        qMainDb.addPlaylist(qTR("Playlist"), 1, StoreType::LOCAL_STORE);
    }

    local_tab_widget_->restoreTabOrder();

    (void)QObject::connect(local_tab_widget_.get(), &PlaylistTabWidget::createNewPlaylist,
        [this]() {
            const auto tab_index = local_tab_widget_->count();
            const auto playlist_id = qMainDb.addPlaylist(qTR("Playlist"), tab_index, StoreType::LOCAL_STORE);
            newPlaylistPage(local_tab_widget_.get(), playlist_id, qTR("Playlist"));
        });

    (void)QObject::connect(local_tab_widget_.get(), &PlaylistTabWidget::removeAllPlaylist,
        [this]() {
            last_play_list_ = nullptr;
        });    

    file_system_view_page_.reset(new FileSystemViewPage(this));

    cd_page_.reset(new CdPage(this));
    cd_page_->playlistPage()->playlist()->setPlaylistId(kDefaultCdPlaylistId, kAppSettingCdPlaylistColumnName);
    cd_page_->playlistPage()->playlist()->setHeaderViewHidden(false);
    cd_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    onSetCover(kEmptyString, cd_page_->playlistPage());
    connectPlaylistPageSignal(cd_page_->playlistPage());

    cloud_search_page_->playlist()->setPlaylistId(kDefaultYtMusicPlaylistId, kAppSettingPlaylistColumnName);
    onSetCover(kEmptyString, cloud_search_page_.get());
    connectPlaylistPageSignal(cloud_search_page_.get());

    (void)QObject::connect(this,
        &Xamp::blurImage,
        background_worker_.get(),
        &BackgroundWorker::onBlurImage);
#if defined(Q_OS_WIN)
    (void)QObject::connect(this,
        &Xamp::fetchCdInfo,
        background_worker_.get(),
        &BackgroundWorker::onFetchCdInfo);
#endif
    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::readCdTrackInfo,
        this,
        &Xamp::onUpdateCdTrackInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::fetchMbDiscInfoCompleted,
        this,
        &Xamp::onUpdateMbDiscInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_worker_.get(),
		&BackgroundWorker::fetchDiscCoverCompleted,
        this,
        &Xamp::onUpdateDiscCover,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_view_page_.get(),
        &FileSystemViewPage::addPathToPlaylist,
        this,
        &Xamp::appendToPlaylist);    

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        album_page_.get(),
        &AlbumArtistPage::onThemeColorChanged);

    connectPlaylistPageSignal(album_page_->album()->albumViewPage()->playlistPage());
    connectPlaylistPageSignal(album_page_->year()->albumViewPage()->playlistPage());

    (void)QObject::connect(this,
        &Xamp::searchLyrics,
        background_worker_.get(),
        &BackgroundWorker::onSearchLyrics);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::fetchLyricsCompleted,
        this,
        &Xamp::onSearchLyricsCompleted);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::fetchArtistCompleted,
        this,
        &Xamp::onSearchArtistCompleted);

    pushWidget(local_tab_widget_.get());
    pushWidget(lrc_page_.get());
    pushWidget(album_page_.get());
    pushWidget(file_system_view_page_.get());
    pushWidget(cd_page_.get());
    pushWidget(cloud_search_page_.get());
    pushWidget(cloud_tab_widget_.get());

    ui_.currentView->setCurrentIndex(0);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::clickedArtist,
        this,
        &Xamp::onArtistIdChanged);

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        lrc_page_.get(),
        &LrcPage::onThemeColorChanged);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::blurImage,
        lrc_page_.get(),
        &LrcPage::setBackground);

    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::dominantColor,
        lrc_page_->lyrics(),
        &LyricsShowWidget::onSetLrcColor);

    (void)QObject::connect(album_page_->album(),
        &AlbumView::addPlaylist,
        this,
        &Xamp::onAddPlaylistItem);
}

void Xamp::appendToPlaylist(const QString& file_name, bool append_to_playlist) {
    if (!append_to_playlist) {
        emit extractFile(file_name, getCurrentPlaylistPage()->playlist()->playlistId(), false);
        album_page_->reload();
        return;
    }

    try {
        getCurrentPlaylistPage()->playlist()->append(file_name);
    }
    catch (const Exception& e) {
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
    }
}

void Xamp::addItem(const QString& file_name) {
    if (local_tab_widget_->count() == 0) {
        const auto playlist_id = qMainDb.addPlaylist(qTR("Playlist"), 1, StoreType::LOCAL_STORE);
        newPlaylistPage(local_tab_widget_.get(), playlist_id, qTR("Playlist"));
    }

    appendToPlaylist(file_name, true);
}

void Xamp::pushWidget(QWidget* widget) {
	const auto id = ui_.currentView->addWidget(widget);
    ui_.currentView->setCurrentIndex(id);
}

void Xamp::encodeAacFile(const PlayListEntity& item, const EncodingProfile& profile) {
	const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    getSaveFileName(this,
        [this, item, profile](const auto &file_name) {
            const auto dialog = makeProgressDialog(
                qTR("Export progress dialog"),
                qTR("Export '") + item.title + qTR("' to aac file"),
                qTR("Cancel"));

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
                read_until::encodeFile(config,
                    encoder,
                    [&](auto progress) -> bool {
                        dialog->setValue(progress);
                        qApp->processEvents();
                        return dialog->wasCanceled() != true;
                    }, track_info);
            }
            catch (Exception const& e) {
                XMessageBox::showError(qTEXT(e.what()));
            }
        },
        qTR("Save AAC file"),
        save_file_name,
        qTR("AAC Files (*.m4a)"));
}

void Xamp::encodeWavFile(const PlayListEntity& item) {
    const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;
    getSaveFileName(this,
        [this, item](const auto& file_name) {
            const auto dialog = makeProgressDialog(
                qTR("Export progress dialog"),
                qTR("Export '") + item.title + qTR("' to wav file"),
                qTR("Cancel"));

            TrackInfo metadata;
            metadata.album = item.album.toStdWString();
            metadata.artist = item.artist.toStdWString();
            metadata.title = item.title.toStdWString();
            metadata.track = item.track;

            std::wstring command;

            try {
                auto encoder = StreamFactory::MakeWaveEncoder();
                read_until::encodeFile(item.file_path.toStdWString(),
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
        },
        qTR("Save Wav file"),
        save_file_name,
        qTR("Wav Files (*.wav)"));    
}

void Xamp::encodeFlacFile(const PlayListEntity& item) {
    const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    const auto save_file_name = last_dir + qTEXT("/") + item.album + qTEXT("-") + item.title;

    getSaveFileName(this,
        [item](const auto& file_name) {
            const auto dialog = makeProgressDialog(
                qTR("Export progress dialog"),
                qTR("Export '") + item.title + qTR("' to flac file"),
                qTR("Cancel"));

            TrackInfo track_info;
            track_info.album = item.album.toStdWString();
            track_info.artist = item.artist.toStdWString();
            track_info.title = item.title.toStdWString();
            track_info.track = item.track;

            const auto command
                = qSTR("-%1 -V").arg(qAppSettings.valueAs(kFlacEncodingLevel).toInt()).toStdWString();

            try {
                auto encoder = StreamFactory::MakeFlacEncoder();
                read_until::encodeFile(item.file_path.toStdWString(),
                    file_name.toStdWString(),
                    encoder,
                    command,
                    [&](auto progress) -> bool {
                        dialog->setValue(progress);
                        qApp->processEvents();
                        return dialog->wasCanceled() != true;
                    }, track_info);
            }
            catch (Exception const& e) {
                XMessageBox::showError(qTEXT(e.what()));
            }
        },    
        qTR("Save Flac file"),
        save_file_name,
        qTR("FLAC Files (*.flac)"));    
}

void Xamp::connectPlaylistPageSignal(PlaylistPage* playlist_page) {
    if (playlist_page != cloud_search_page_.get()) {
        (void)QObject::connect(playlist_page,
            &PlaylistPage::search,
            [this, playlist_page](const auto& text, Match match) {
                playlist_page->playlist()->search(text);
            });
    } else {
        (void)QObject::connect(playlist_page,
            &PlaylistPage::search,
            [this, playlist_page](const auto& text, Match match) {
                if (playlist_page->spinner()->isAnimated()) {
                    return;
                }
                if (playlist_page->spinner()->isHidden()) {
                    playlist_page->spinner()->show();
                }
                playlist_page->spinner()->startAnimation();
                if (match == Match::MATCH_ITEM) {
                    QCoro::connect(ytmusic_worker_->searchAsync(text, "albums"), this, &Xamp::onSearchCompleted);
                } else {
                    QCoro::connect(ytmusic_worker_->searchSuggestionsAsync(text), this, &Xamp::onSearchSuggestionsCompleted);
                    playlist_page->spinner()->stopAnimation();
                }
            });
    }

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::addPlaylistItemFinished,
        album_page_.get(),
        &AlbumArtistPage::reload);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::playMusic,
        playlist_page,
        &PlaylistPage::playMusic);

    (void)QObject::connect(playlist_page,
        &PlaylistPage::playMusic,
        this,
        &Xamp::onPlayPlayListEntity);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::encodeFlacFile,
        this,
        &Xamp::encodeFlacFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::encodeAacFile,
        this,
        &Xamp::encodeAacFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::encodeWavFile,
        this,
        &Xamp::encodeWavFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::readReplayGain,
        background_worker_.get(),
        &BackgroundWorker::onReadReplayGain);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::editTags,
        this,
        &Xamp::onEditTags);

    (void)QObject::connect(playlist_page->playlist(),
        &PlayListTableView::extractFile,
        extract_file_worker_.get(),
        &ExtractFileWorker::onExtractFile);
    
    (void)QObject::connect(background_worker_.get(),
        &BackgroundWorker::readReplayGain,
        playlist_page->playlist(),
        &PlayListTableView::onUpdateReplayGain,
        Qt::QueuedConnection);    

    (void)QObject::connect(this,
        &Xamp::themeChanged,
        playlist_page->playlist(),
        &PlayListTableView::onThemeColorChanged);

    (void)QObject::connect(this,
        &Xamp::currentThemeChanged,
        playlist_page,
        &PlaylistPage::onCurrentThemeChanged);
}

PlaylistPage* Xamp::createPlaylistPage(int32_t playlist_id, const QString& column_setting_name) {
    auto* playlist_page = new PlaylistPage(local_tab_widget_.get());    
    playlist_page->playlist()->setPlaylistId(playlist_id, column_setting_name);
    return playlist_page;
}

void Xamp::addDropFileItem(const QUrl& url) {
    addItem(url.toLocalFile());
}

PlaylistPage* Xamp::getCurrentPlaylistPage() {
    if (local_tab_widget_->count() == 0) {
        return nullptr;
    }
    return dynamic_cast<PlaylistPage*>(local_tab_widget_->currentWidget());
}

void Xamp::onInsertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id) {
    DatabaseFacade facede;
    (void)QObject::connect(&facede,
        &DatabaseFacade::findAlbumCover,
        find_album_cover_worker_.get(),
        &FindAlbumCoverWorker::onFindAlbumCover,
        Qt::QueuedConnection);
    facede.insertTrackInfo(result, playlist_id);    
    emit translation(getStringOrEmptyString(result.front().artist), qTEXT("ja"), qTEXT("en"));
    if (local_tab_widget_->count() == 0) {
        const auto new_playlist_id = qMainDb.addPlaylist(qTR("Playlist"), 1, StoreType::LOCAL_STORE);
        newPlaylistPage(local_tab_widget_.get(), new_playlist_id, qTR("Playlist"));
    }
    getCurrentPlaylistPage()->playlist()->reload();
}

void Xamp::onReadFilePath(const QString& file_path) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->setLabelText(file_path);
}

void Xamp::onSetAlbumCover(int32_t album_id, const QString& cover_id) {
    qMainDb.setAlbumCover(album_id, cover_id);
    album_page_->reload();
}

void Xamp::onTranslationCompleted(const QString& keyword, const QString& result) {
    qMainDb.updateArtistEnglishName(keyword, result);
}

void Xamp::onEditTags(int32_t playlist_id, const QList<PlayListEntity>& entities) {
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<TagEditPage> tag_edit_page(new TagEditPage(dialog.get(), entities));
    dialog->setContentWidget(tag_edit_page.get());
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EDIT));
    dialog->setTitle(qTR("Edit track information"));    
    dialog->exec();
    if (local_tab_widget_->count() > 0) {
        getCurrentPlaylistPage()->playlist()->reload();
    }
}

void Xamp::onReadFileProgress(int32_t progress) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->setValue(progress);    
}

void Xamp::onReadCompleted() {
    ui_update_timer_timer_.stop();
    delay(1);
    if (local_tab_widget_->count() > 0) {
        getCurrentPlaylistPage()->playlist()->reload();
    }
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->close();
    read_progress_dialog_.reset();    
}

void Xamp::onFoundFileCount(size_t file_count) {    
    if (!read_progress_dialog_
        && progress_timer_.elapsed() > kShowProgressDialogMsSecs) {
        read_progress_dialog_ = makeProgressDialog(kApplicationTitle,
            qTR("Read track information"),
            qTR("Cancel"));

        (void)QObject::connect(read_progress_dialog_.get(),
            &XProgressDialog::cancelRequested, [this]() {
                extract_file_worker_->cancelRequested();
                find_album_cover_worker_->cancelRequested();
            });

        read_progress_dialog_->exec();
    }

    if (!read_progress_dialog_) {
        return;
    }

    read_progress_dialog_->setTitle(qSTR("Total number of files %1").arg(file_count));
}

void Xamp::onReadFileStart() {
    progress_timer_.restart();
    ui_update_timer_timer_.start();
}
