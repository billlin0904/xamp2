﻿#include <Audioclient.h>
#include <QImageReader>
#include <QInputDialog>
#include <QShortcut>
#include <QSimpleUpdater.h>
#include <QToolTip>
#include <QWidgetAction>
#include <QMoveEvent>
#include <QCoreApplication>

#include <set>
#include <thememanager.h>
#include <version.h>
#include <xamp.h>
#include <style_util.h>

#include <base/dsd_utils.h>
#include <base/logger_impl.h>
#include <base/scopeguard.h>
#include <base/crashhandler.h>

#include <output_device/api.h>
#include <output_device/iaudiodevicemanager.h>
#include <output_device/win32/sharedwasapidevicetype.h>

#include <player/api.h>
#include <stream/api.h>

#include <stream/compressorconfig.h>
#include <stream/idspmanager.h>

#include <widget/aboutpage.h>
#include <widget/actionmap.h>
#include <widget/albumartistpage.h>
#include <widget/albumview.h>
#include <widget/artistview.h>
#include <widget/encodejobwidget.h>

#include <widget/appsettings.h>
#include <widget/cdpage.h>
#include <widget/xtooltip.h>
#include <widget/processindicator.h>
#include <widget/equalizerview.h>
#include <widget/filesystemviewpage.h>
#include <widget/genre_view_page.h>
#include <widget/imagecache.h>
#include <widget/jsonsettings.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/playlistpage.h>
#include <widget/playlisttabbar.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlisttableview.h>
#include <widget/playlisttabwidget.h>
#include <widget/preferencepage.h>
#include <widget/spectrumwidget.h>
#include <widget/tageditpage.h>
#include <widget/widget_shared.h>
#include <widget/xdialog.h>
#include <widget/xmessagebox.h>
#include <widget/xprogressdialog.h>

#include <widget/playliststyleditemdelegate.h>

#include <widget/util/image_util.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>

#include <widget/worker/backgroundservice.h>
#include <widget/worker/filesystemservice.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/m3uparser.h>
#include <widget/chatgpt/waveformwidget.h>
#include <widget/artistinfopage.h>
#include <widget/logview.h>
#include <widget/ytmusicserverprocessor.h>

namespace {
    constexpr auto kSoftwareUpdateUrl =
        "https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json"_str;

    constexpr auto kEpisodesForLaterName("Episodes for Later"_str);

    QString make544xh544Image(const QString &url) {
        QString modified_url = url;
        modified_url.replace("w120-h120-l90-rj"_str, "w544-h544-l90-rj"_str);
        return modified_url;
    }

    DsdModes getDsdModes(const DeviceInfo& device_info,
        const PlayListEntity& entity,
        uint32_t target_sample_rate) {
        auto dsd_modes = DsdModes::DSD_MODE_AUTO;
        const auto is_enable_sample_rate_converter = target_sample_rate > 0;
        //const auto is_dsd_file = IsDsdFile(file_path);
		const auto is_dsd_file = 
            entity.file_extension == ".dff"_str || entity.file_extension == ".dsf"_str;

        if (!is_dsd_file
            && !is_enable_sample_rate_converter
            && entity.sample_rate % kPcmSampleRate441 == 0) {
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

    PlaylistPage* createPlaylistPage(PlaylistTabWidget* tab_widget, int32_t playlist_id, const QString& column_setting_name, const QString& cloud_playlist_id) {
        auto* playlist_page = new PlaylistPage(tab_widget);
        playlist_page->playlist()->setPlaylistId(playlist_id, column_setting_name);
        if (!cloud_playlist_id.isEmpty()) {
            playlist_page->playlist()->setCloudPlaylistId(cloud_playlist_id);
        }
        return playlist_page;
    }
}

Xamp::Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer>& player)
    : IXFrame(parent)
    , is_seeking_(false)
    , trigger_upgrade_action_(false)
    , trigger_upgrade_restart_(false)
    , cloud_playlist_process_count_(0)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , lrc_page_(nullptr)
	, music_page_(nullptr)
	, cd_page_(nullptr)
	, library_page_(nullptr)
	, file_explorer_page_(nullptr)
	, background_service_(nullptr)
	, state_adapter_(std::make_shared<UIPlayerStateAdapter>())
	, player_(player) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_.setupUi(this);
}

Xamp::~Xamp() {
	destroy();
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

QString Xamp::translateText(const std::string_view& text) {
    return tr(text.data());
}

QString Xamp::translateError(Errors error) {
    using xamp::base::Errors;
    static const QMap<Errors, ConstexprQString> lut{
        { Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR,    QT_TRANSLATE_NOOP("Xamp", "Platform spec error") },
        { Errors::XAMP_ERROR_DEVICE_CREATE_FAILURE,  QT_TRANSLATE_NOOP("Xamp", "Device create failure") },
    };
    const auto str = lut.value(error, kEmptyString);
    return tr(str.data());
}

void Xamp::destroy() {
    if (player_ != nullptr) {
        player_->Destroy();
        player_.reset();
        XAMP_LOG_DEBUG("Player destroy!");
    } else {
        return;
    }

    yt_music_server_processor_.reset();

    auto quit_and_wait_thread = [](auto& thread) {
        if (!thread.isFinished()) {
            thread.requestInterruption();
            thread.quit();
            thread.wait();
        }
    };

    emit cancelRequested();

    quit_and_wait_thread(background_service_thread_);
    quit_and_wait_thread(album_cover_service_thread_);
    quit_and_wait_thread(file_system_service_thread_);

    if (main_window_ != nullptr) {
        (void)main_window_->saveGeometry();
    }

    qJsonSettings.save();
    qAppSettings.save();
    qAppSettings.saveLogConfig();

    playlist_tab_page_->tabWidget()->saveTabOrder();
    album_cover_service_->cleaup();
    qGuiDb.Close();

    XAMP_LOG_DEBUG("Xamp destroy!");

    XampCrashHandler.Cleanup();
}

void Xamp::onActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    {
        main_window_->showNormal();
        main_window_->raise();
        main_window_->activateWindow();
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

void Xamp::onEncodeAlacFiles(int32_t encode_type, const QList<PlayListEntity>& files) {
    getExistingDirectory(this, kEmptyString, [this, encode_type, &files](const auto& dir_name) {
        const QScopedPointer<XDialog> dialog(new XDialog(this));
        const QScopedPointer<EncodeJobWidget> encode_job_widget(new EncodeJobWidget(dialog.get()));
        encode_job_widget->setFixedSize(QSize(1200, 600));
        (void)QObject::connect(background_service_.get(), &BackgroundService::updateJobProgress,
            encode_job_widget.get(), &EncodeJobWidget::onUpdateProgress);
        (void)QObject::connect(background_service_.get(), &BackgroundService::jobError,
            encode_job_widget.get(), &EncodeJobWidget::onJobError);
        emit addJobs(dir_name, encode_job_widget->addJobs(encode_type, files));
        dialog->setContentWidget(encode_job_widget.get());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_ABOUT));
        dialog->setTitle(tr("Encode batch progress"));
        dialog->exec();
        background_service_->cancelAllJob();
        });
}

void Xamp::showAbout() {
	const QScopedPointer<XDialog> dialog(new XDialog(this));
	const QScopedPointer<AboutPage> about_page(new AboutPage(dialog.get()));
	(void)QObject::connect(about_page.get(), &AboutPage::CheckForUpdate, this, &Xamp::onCheckForUpdate);
	(void)QObject::connect(about_page.get(), &AboutPage::RestartApp, this, &Xamp::onRestartApp);
	(void)QObject::connect(this, &Xamp::updateNewVersion, about_page.get(), &AboutPage::OnUpdateNewVersion);
	dialog->setContentWidget(about_page.get());
	dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_ABOUT));
    dialog->setTitle(tr("About"));
    dialog->exec();
}

void Xamp::connectThemeChangedSignal() {
	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       lrc_page_.get(),
	                       &LrcPage::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       this,
	                       &Xamp::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       playlist_tab_page_->tabWidget(),
	                       &PlaylistTabWidget::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       ui_.naviBar,
	                       &NavBarListView::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       library_page_->album(),
	                       &AlbumView::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       cd_page_.get(),
	                       &CdPage::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       ui_.mutedButton,
	                       &VolumeButton::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       lrc_page_.get(),
	                       &LrcPage::onThemeChangedFinished);

	(void)QObject::connect(&qTheme,
	                       &ThemeManager::themeChangedFinished,
	                       library_page_.get(),
	                       &AlbumArtistPage::onThemeChangedFinished);
}

void Xamp::setMainWindow(IXMainWindow* main_window) {
    main_window_ = main_window;
    order_ = qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder);

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

    auto* quit_action = new QAction(tr("Quit"));
    (void)QObject::connect(quit_action, &QAction::triggered, [this]() {
        tray_icon_->hide();
        qApp->quit();
        });
    tray_icon_menu->addAction(quit_action);

    tray_icon_->setContextMenu(tray_icon_menu);
    (void)QObject::connect(tray_icon_.get(), &QSystemTrayIcon::activated, this, &Xamp::onActivated);
    tray_icon_->show();

    background_service_.reset(new BackgroundService());
    background_service_->moveToThread(&background_service_thread_);
    background_service_thread_.start(QThread::LowestPriority);

    album_cover_service_.reset(new AlbumCoverService());
    album_cover_service_->moveToThread(&album_cover_service_thread_);
    album_cover_service_thread_.start(QThread::NormalPriority);

    file_system_service_.reset(new FileSystemService());
    file_system_service_->moveToThread(&file_system_service_thread_);
    file_system_service_thread_.start(QThread::LowestPriority);

    /*yt_music_server_processor_.reset(new YtMusicServerProcessor());
    yt_music_server_processor_->start().then([this](int exit_code) {
        if (exit_code != 0) {
            XAMP_LOG_ERROR("ytmusic server process exit with code: " + std::to_string(exit_code));
        }
        });*/

    if (background_service_ != nullptr) {
        (void)QObject::connect(this, &Xamp::cancelRequested, 
            background_service_.get(),
            &BackgroundService::cancelRequested);
    }
    if (album_cover_service_ != nullptr) {
        (void)QObject::connect(this, &Xamp::cancelRequested, 
            album_cover_service_.get(),
            &AlbumCoverService::cancelRequested);
    }
    if (file_system_service_ != nullptr) {
        (void)QObject::connect(this, &Xamp::cancelRequested,
            file_system_service_.get(),
            &FileSystemService::cancelRequested);
    }

    player_->SetStateAdapter(state_adapter_);
    player_->SetDelayCallback([this](auto seconds) {
        delay(seconds);
    });

    initialUi();
    initialController();
    initialPlaylist();
    initialShortcut();
    initialSpectrum();

    last_playlist_tab_ = playlist_tab_page_->tabWidget();

    setPlaylistPageCover(nullptr, cd_page_->playlistPage());

    cd_page_->playlistPage()->hidePlaybackInformation(true);

    const auto tab_name = qAppSettings.valueAsString(kAppSettingLastTabName);
    const auto tab_id = ui_.naviBar->tabId(tab_name);
    ui_.currentView->setAnimationEnabled(false);
    if (tab_id != -1) {
        ui_.naviBar->setCurrentIndex(tab_id);
        setCurrentTab(tab_id);
    }
    else {
        ui_.naviBar->setCurrentIndex(0);
        setCurrentTab(0);
    }
    ui_.currentView->setAnimationEnabled(true);

    (void)QObject::connect(album_cover_service_.get(),
        &AlbumCoverService::setAlbumCover,
        this, 
        &Xamp::onSetAlbumCover);

    (void)QObject::connect(this,
        &Xamp::fetchThumbnailUrl,
        album_cover_service_.get(),
        &AlbumCoverService::onFetchThumbnailUrl);

    (void)QObject::connect(this,
        &Xamp::fetchYoutubeThumbnailUrl,
        album_cover_service_.get(),
        &AlbumCoverService::onFetchYoutubeThumbnailUrl);

    (void)QObject::connect(this,
        &Xamp::fetchArtistThumbnailUrl,
        album_cover_service_.get(),
        &AlbumCoverService::onFetchArtistThumbnailUrl);

    (void)QObject::connect(album_cover_service_.get(),
        &AlbumCoverService::setThumbnail,
        this,
        &Xamp::onSetThumbnail);

    (void)QObject::connect(album_cover_service_.get(),
        &AlbumCoverService::setAristThumbnail,
        this,
        &Xamp::onSetAristThumbnail);

    connectThemeChangedSignal();

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::enableFetchThumbnail,
        album_cover_service_.get(),
        &AlbumCoverService::enableFetchThumbnail,
        Qt::QueuedConnection);

    (void)QObject::connect(this,
        &Xamp::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile,
        Qt::QueuedConnection);

   (void)QObject::connect(library_page_->album(),
        &AlbumView::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile,
        Qt::QueuedConnection);

   (void)QObject::connect(library_page_->artist()->artistViewPage()->album()->styledDelegate(),
       &AlbumViewStyledDelegate::findAlbumCover,
       album_cover_service_.get(),
       &AlbumCoverService::onFindAlbumCover,
       Qt::QueuedConnection);

   (void)QObject::connect(library_page_->album()->progressPage(),
       &ScanFileProgressPage::cancelRequested,
       this,
       &Xamp::onCancelRequested);

   (void)QObject::connect(library_page_->album()->styledDelegate(),
       &AlbumViewStyledDelegate::findAlbumCover,
       album_cover_service_.get(),
       &AlbumCoverService::onFindAlbumCover,
       Qt::QueuedConnection);

   (void)QObject::connect(library_page_->year()->styledDelegate(),
       &AlbumViewStyledDelegate::findAlbumCover,
       album_cover_service_.get(),
       &AlbumCoverService::onFindAlbumCover,
       Qt::QueuedConnection);

   (void)QObject::connect(this,
       &Xamp::findAlbumCover,
       album_cover_service_.get(),
       &AlbumCoverService::onFindAlbumCover,
       Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::insertDatabase,
        this,
        &Xamp::onInsertDatabase,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::batchInsertDatabase,
        this,
        &Xamp::onBatchInsertDatabase,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::foundFileCount,
        library_page_->album()->progressPage(),
        &ScanFileProgressPage::onFoundFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFilePath,
        library_page_->album()->progressPage(),
        &ScanFileProgressPage::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileStart,
        library_page_->album()->progressPage(),
        &ScanFileProgressPage::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readCompleted,
        library_page_->album()->progressPage(),
        &ScanFileProgressPage::onReadCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileProgress,
        library_page_->album()->progressPage(),
        &ScanFileProgressPage::onReadFileProgress,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::remainingTimeEstimation,
        library_page_->album()->progressPage(),
        &ScanFileProgressPage::onRemainingTimeEstimation,
        Qt::QueuedConnection);

    auto* updater = QSimpleUpdater::getInstance();    

    //(void)QObject::connect(updater,
    //    &QSimpleUpdater::appcastDownloaded,
    //    [updater, this](const QString& url, const QByteArray& reply) {
    //        const auto document = QJsonDocument::fromJson(reply);
    //        const auto updates = document.object().value("updates"_str).toObject();
    //        const auto platform = updates.value(kPlatformKey).toObject();
    //        const auto changelog = platform.value("changelog"_str).toString();
    //        const auto latest_version = platform.value("latest-version"_str).toString();

    //        QVersionNumber latest_version_value;
    //        if (!parseVersion(latest_version, latest_version_value)) {
    //            return;
    //        }

    //        //if (trigger_upgrade_action_) {
    //            const auto download_url = platform.value("download-url"_str).toString();
    //            XMessageBox::showInformation(changelog, tr("New version!"), true);
    //        //}

    //        //if (latest_version_value > kApplicationVersionValue) {
    //        //    trigger_upgrade_action_ = true;
    //        //}

    //        emit updateNewVersion(latest_version_value);
    //    });
    //updater->checkForUpdates(kSoftwareUpdateUrl);
    
    setPlayerOrder();
    initialDeviceList();
}

void Xamp::onRestartApp() {
    trigger_upgrade_restart_ = true;
    qApp->exit(kRestartExistCode);
}

void Xamp::onCheckForUpdate() {
    auto* updater = QSimpleUpdater::getInstance();
    updater->setPlatformKey(kSoftwareUpdateUrl, kPlatformKey);
    updater->setModuleVersion(kSoftwareUpdateUrl, kApplicationVersion);
    updater->setUseCustomAppcast(kSoftwareUpdateUrl, true);
    updater->checkForUpdates(kSoftwareUpdateUrl);
}

void Xamp::initialSpectrum() {
    if (!qAppSettings.valueAsBool(kAppSettingEnableSpectrum)) {
        return;
    }

    lrc_page_->spectrum()->setBarColor(qTheme.highlightColor());

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::fftResultChanged,
        lrc_page_->spectrum(),
        &SpectrumWidget::onFftResultChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(&qTheme,
        &ThemeManager::themeChangedFinished,
        lrc_page_->spectrum(),
        &SpectrumWidget::onThemeChangedFinished);
}

void Xamp::closeEvent(QCloseEvent* event) {
    if (!trigger_upgrade_restart_ 
        && XMessageBox::showYesOrNo(tr("Do you want to exit the XAMP2 ?")) == QDialogButtonBox::No) {
        event->ignore();
        return;
    }
    destroy();
    window()->close();
}

void Xamp::initialUi() {
    QFont f("DisplayFont"_str);
    f.setWeight(QFont::Bold);
    f.setPointSize(qTheme.fontSize(8));
    ui_.titleLabel->setFont(f);

    f.setWeight(QFont::Normal);
    f.setPointSize(qTheme.fontSize(8));
    ui_.artistLabel->setFont(f);    

    QToolTip::hideText();

    f.setWeight(QFont::DemiBold);
    f.setPointSize(10);

    QFont mono_font("MonoFont"_str);
    mono_font.setPointSize(qTheme.fontSize(6));
    ui_.startPosLabel->setFont(mono_font);
    ui_.endPosLabel->setFont(mono_font);
    ui_.seekSlider->setDisabled(true);

    ui_.mutedButton->setAudioPlayer(player_);
    ui_.mutedButton->updateState();
    ui_.coverLabel->setAttribute(Qt::WA_StaticContents);
    qTheme.setPlayOrPauseButton(ui_.playButton, false);

    //ui_.stopButton->hide();    
    //ui_.formatLabel->hide();

    setNaviBarMenuButton(ui_);    
}

void Xamp::onDeviceStateChanged(DeviceState state, const QString &device_id) {
    XAMP_LOG_DEBUG("OnDeviceStateChanged: {}", state);

    if (state == DeviceState::DEVICE_STATE_REMOVED) {
        player_->Stop(true, true, true);
    }
    if (state == DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE) {
        return;
    }

    if (qAppSettings.valueAsBool(kAppSettingAutoSelectNewDevice)) {
        initialDeviceList(device_id.toStdString());
    } else {
        initialDeviceList();
    }
}

QWidgetAction* Xamp::createDeviceMenuWidget(const QString& desc, const QIcon &icon) {
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
        menu->addAction(createDeviceMenuWidget(translateDeviceDescription(device_type.get())));
       
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

        qDaoFacade.music_dao.updateMusicHeart(current_entity_.value().music_id, current_entity_.value().heart);
        qTheme.setHeartButton(ui_.heartButton, current_entity_.value().heart);
        if (last_playlist_page_ != nullptr) {
            last_playlist_page_->setHeart(current_entity_.value().heart);
            last_playlist_page_->playlist()->reload();
        }
        });

    ui_.seekSlider->enableAnimation(false);

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
        &UIPlayerStateAdapter::playbackError,
        this,
        &Xamp::onPlaybackError,
        Qt::QueuedConnection);

    (void)QObject::connect(state_adapter_.get(),
        &UIPlayerStateAdapter::volumeChanged,
        this,
        &Xamp::setVolume,
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
        QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
        QScopedPointer<XDialog> dialog(new XDialog(this));
        QScopedPointer< EqualizerView> eq(new EqualizerView(dialog.get()));
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
        dialog->setTitle(tr("Equalizer"));
        dialog->setContentWidget(eq.get(), false);
        dialog->exec();

        /*QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
        const QScopedPointer<XDialog> dialog(new XDialog(this));
        dialog->setTitle(tr("Log Viewer"));
        QScopedPointer<LogView> log_view(new LogView(dialog.get()));
        log_view->loadLogFile("logs/xamp.log"_str);
        log_view->setFixedSize(QSize(1200, 600));
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_REPORT_BUG));
        dialog->setContentWidget(log_view.get(), false);
        dialog->exec();*/
    });

    (void)QObject::connect(ui_.repeatButton, &QToolButton::clicked, [this]() {
        order_ = getNextOrder(order_);
        setPlayerOrder(true);
    });

    (void)QObject::connect(ui_.playButton, &QToolButton::clicked, [this]() {
        playOrPause();
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

    (void)QObject::connect(ui_.naviBarButton, &QToolButton::clicked, [this]() {
        showNaviBarButton();
        });
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

    // 當動畫完成後鎖定寬度
    (void)QObject::connect(animation, &QPropertyAnimation::finished, this, [this, end_width]() {
        ui_.sliderFrame2->setMinimumWidth(end_width);
        ui_.sliderFrame2->setMaximumWidth(end_width);
        if (end_width == kMinWidth) {
            ui_.naviBar->collapse();
        } else {
			ui_.naviBar->expand();
        }
        });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Xamp::setCurrentTab(int32_t table_id) {
    switch (table_id) {
    case TAB_MUSIC_LIBRARY:
        library_page_->reload();
        break;
    case TAB_FILE_EXPLORER:
        break;
    case TAB_PLAYLIST:        
        ensureLocalOnePlaylistPage();
        playlist_tab_page_->tabWidget()->reloadAll();
        break;
    case TAB_LYRICS:
        ui_.currentView->setCurrentWidget(lrc_page_.get());
        break;
    case TAB_CD:
        ui_.currentView->setCurrentWidget(cd_page_.get());
        break;
    case TAB_YT_MUSIC_PLAYLIST:
        if (!yt_music_tab_page_->tabWidget()->count()) {
            initialCloudPlaylist();
        }
        else {
            yt_music_tab_page_->tabWidget()->setCurrentIndex(0);
        }
        yt_music_tab_page_->tabWidget()->reloadAll();
        break;
    }
    if (widgets_.size() > table_id) {
        ui_.currentView->slideInWidget(widgets_[table_id]);
    }
    else {
		ui_.currentView->slideInWidget(playlist_tab_page_.get());
    }
}

void Xamp::onThemeChangedFinished(ThemeColor theme_color) {
    Q_FOREACH(QFrame *frame, device_type_frame_) {
        qTheme.setFrameBackgroundColor(frame);
	}

    if (qAppSettings.valueAsBool(kAppSettingIsMuted)) {
        qTheme.setMuted(ui_.mutedButton, true);
    }
    else {
        qTheme.setMuted(ui_.mutedButton, false);
    }

    setThemeIcon(ui_);
    setRepeatButtonIcon(ui_, order_);
    setWidgetStyle(ui_);
    updateButtonState(ui_.playButton, player_->GetState());

    for (auto i = 0; i < playlist_tab_page_->tabWidget()->tabBar()->count(); ++i) {
        auto* page = playlist_tab_page_->tabWidget()->playlistPage(i);
        if (!page) {
            continue;
        }
        setPlaylistPageCover(nullptr, page);
    }

    setPlaylistPageCover(nullptr, cd_page_->playlistPage());

    if (current_entity_) {
        if (current_entity_.value().cover_id == qImageCache.unknownCoverId()) {
            setCover(kEmptyString);
        }
    }
    else {
		setCover(kEmptyString);
    }
   
	setNaviBarMenuButton(ui_);

    preference_action_->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));

    library_page_->album()->reload();
    library_page_->artist()->reload();
}

void Xamp::onSearchArtistCompleted(const QString& artist, const QByteArray& image) {
    QPixmap cover;
    if (cover.loadFromData(image)) {        
        qDaoFacade.artist_dao.updateArtistCoverId(
            qDaoFacade.artist_dao.addOrUpdateArtist(artist), qImageCache.addImage(cover));
    }
}

void Xamp::shortcutsPressed(const QKeySequence& shortcut) {
    XAMP_LOG_DEBUG("shortcutsPressed: {}", shortcut.toString().toStdString());

	static const QMap<QKeySequence, std::function<void()>> shortcut_map {
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
            QKeySequence(Qt::Key_F10), [this]() {
                last_playlist_page_->playlist()->moveUp();
            },
        },
        {
            QKeySequence(Qt::Key_F11), [this]() {
				last_playlist_page_->playlist()->moveDown();
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
    
    playlist_tab_page_->tabWidget()->forEachPlaylist([](auto* playlist) {
        playlist->setNowPlayState(PlayingState::PLAY_CLEAR);
        });
    playlist_tab_page_->tabWidget()->resetAllTabIcon();

    library_page_->album()->albumViewPage()->playlistPage()->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
    qTheme.setPlayOrPauseButton(ui_.playButton, true); // note: Stop play must set true.
    current_entity_ = std::nullopt;
    main_window_->setTaskbarPlayerStop();
    lrc_page_->spectrum()->reset();
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
	if (!current_entity_) {
		return;
	}
    const auto full_text = isMoreThan1Hours(current_entity_.value().duration);
    if (current_entity_.value().duration - stream_time >= 0.0) {
        ui_.endPosLabel->setText(formatDuration(current_entity_.value().duration - stream_time, full_text));
        const auto stream_time_as_ms = static_cast<int32_t>(stream_time * 1000.0);
        ui_.seekSlider->setValue(stream_time_as_ms);
        ui_.startPosLabel->setText(formatDuration(stream_time, full_text));
        main_window_->setTaskbarProgress(static_cast<int32_t>(100.0 * ui_.seekSlider->value() / ui_.seekSlider->maximum()));
        lrc_page_->lyrics()->setLrcTime(stream_time_as_ms);
        file_explorer_page_->waveformWidget()->setCurrentPosition(stream_time);
    }
}

void Xamp::playLocalFile(const PlayListEntity& entity) {
    main_window_->setTaskbarPlayerPlaying();
    onPlayMusic(kInvalidDatabaseId, entity);
}

void Xamp::playOrPause() {
    XAMP_LOG_DEBUG("Player state:{}", player_->GetState());

    const auto tab_index = ui_.currentView->currentIndex();
    PlaylistPage* page = nullptr;
    PlaylistTabWidget* tab = nullptr;

    if (!last_playlist_tab_) {
        tab = playlist_tab_page_->tabWidget();
        page = localPlaylistPage();
    }
    else {
        tab = last_playlist_tab_;
        page = last_playlist_page_;
    }

    if (!page) {
        auto *tab_page = dynamic_cast<PlaylistTabPage*>(ui_.currentView->currentWidget());
        tab = tab_page->tabWidget();
		page = tab->playlistPage(tab->currentIndex());
    }

    XAMP_TRY_LOG(
        if (player_->GetState() == PlayerState::PLAYER_STATE_RUNNING) {
            qTheme.setPlayOrPauseButton(ui_.playButton, false);
            player_->Pause();
            page->playlist()->setNowPlayState(PlayingState::PLAY_PAUSE);
            main_window_->setTaskbarPlayerPaused();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_PAUSED) {
            qTheme.setPlayOrPauseButton(ui_.playButton, true);
            player_->Resume();
            page->playlist()->setNowPlayState(PlayingState::PLAY_PLAYING);
            main_window_->setTaskbarPlayingResume();
        }
        else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED 
            || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
            if (!ui_.currentView->count()) {
                return;
            }
            if (const auto select_item = page->playlist()->selectFirstItem()) {
                play_index_ = select_item.value();
            }
            play_index_ = page->playlist()->model()->index(
                play_index_.row(), PLAYLIST_IS_PLAYING);
            if (play_index_.row() == -1) {
                // Don't show message box. when the application will receive other signal.
                // message box will be show again.
                //XMessageBox::showInformation(tr("Not found any playing item."));
                return;
            }
            page->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
            page->playlist()->setNowPlaying(play_index_);
            page->playlist()->onPlayIndex(play_index_);
        }
        tab->setPlayerStateIcon(page->playlist()->playlistId(), player_->GetState());
    );
}

void Xamp::resetSeekPosValue() {
    ui_.seekSlider->setValue(0);
    ui_.startPosLabel->setText(formatDuration(0));
}

void Xamp::setupDsp(const PlayListEntity& item) const {
	if (qAppSettings.valueAsBool(kAppSettingEnableEQ)) {
        if (qAppSettings.contains(kAppSettingEQName)) {
            auto [name, _] = qAppSettings.eqSettings();
            auto eq_settings = qAppSettings.eqPreset()[name];
            player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, eq_settings);
            player_->GetDspManager()->AddParametricEq();
        }
    }
    else {
        float replay_gain = 0;
        auto mode = qAppSettings.valueAsEnum<ReplayGainMode>(kAppSettingReplayGainMode);
        if (mode == ReplayGainMode::RG_TRACK_MODE) {
            if (item.replay_gain) {
                EqSettings eq_settings;
                eq_settings.preamp = item.replay_gain.value().track_gain;
                player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, eq_settings);
                player_->GetDspManager()->AddParametricEq();
            }
        }
        else if (mode == ReplayGainMode::RG_TRACK_MODE) {
            if (item.replay_gain) {
                EqSettings eq_settings;
                eq_settings.preamp = item.replay_gain.value().album_gain;
                player_->GetDspConfig().AddOrReplace(DspConfig::kEQSettings, eq_settings);
                player_->GetDspManager()->AddParametricEq();
            }
        }        
    	else {
            player_->GetDspManager()->RemoveEqualizer();
            player_->GetDspManager()->RemoveParametricEq();
        }
    }
}

void Xamp::setupSampleWriter(ByteFormat byte_format,
    PlaybackFormat& playback_format) const {
    player_->GetDspManager()->SetSampleWriter();
    player_->PrepareToPlay(byte_format);
    playback_format = getPlaybackFormat(player_.get());
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
        soxr_settings = qJsonSettings.valueAs(kSoxr).toMap()[setting_name].toMap();
        target_sample_rate = soxr_settings[kResampleSampleRate].toUInt();

        initial_sample_rate_converter = [this, soxr_settings]() {
            player_->GetDspManager()->AddPreDSP(makeSoxrSampleRateConverter(soxr_settings));
        };
    }
    else if (sample_rate_converter_type == kR8Brain) {
        auto config = qJsonSettings.valueAsMap(kR8Brain);
        target_sample_rate = config[kResampleSampleRate].toUInt();

        initial_sample_rate_converter = [this]() {
            player_->GetDspManager()->AddPreDSP(makeR8BrainSampleRateConverter());
        };
    }
    else if (sample_rate_converter_type == kSrc) {
        auto config = qJsonSettings.valueAsMap(kSrc);
        target_sample_rate = config[kResampleSampleRate].toUInt();

        initial_sample_rate_converter = [this]() {
            player_->GetDspManager()->AddPreDSP(makeSrcSampleRateConverter());
        };
    }
}

void Xamp::onSetAristThumbnail(int32_t artist_id, const QString& cover_id) {
    qDaoFacade.artist_dao.updateArtistCoverId(artist_id, cover_id);
}

void Xamp::onSetThumbnail(const DatabaseCoverId& id, const QString& cover_id) {
    try {
        if (id.isAlbumIdValid()) {
            qDaoFacade.album_dao.setAlbumCover(id.get(), cover_id);
        }
        else {
            qDaoFacade.music_dao.setMusicCover(id.get(), cover_id);
        }
    }
    catch (const Exception& e) {
        XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
    }
    catch (const std::exception& e) {
        XAMP_LOG_DEBUG("Failed to insert database: {}", e.what());
    }
    catch (...) {
    }
    
}

void Xamp::onPlayEntity(PlaylistPage* playlist_page, 
    const PlayListEntity& entity) {
    if (!device_info_) {
        showMeMessage(tr("Not found any audio device, please check your audio device."));
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
    uint32_t target_sample_rate = 0;
    auto byte_format = ByteFormat::SINT32;
    std::function<void()> sample_rate_converter_factory;

    player_->Stop();
    player_->EnableFadeOut(qAppSettings.valueAsBool(kAppSettingEnableFadeOut));

	// Setup blue tooth device or shared output device (ex: Windows WASAPI) sample rate converter.
    if (player_->GetAudioDeviceManager()->IsSharedDevice(device_info_.value().device_type_id)) {
        AudioFormat default_format;
        if (device_info_.value().default_format) {
            default_format = device_info_.value().default_format.value();
        }
        else {
#ifdef Q_OS_WIN            
            default_format = AudioFormat::k16BitPCM441Khz;
#else
            default_format = AudioFormat::k16BitPCM48Khz;
#endif
        }

        auto sample_rate = entity.isFilePath()
    	? entity.sample_rate : AudioFormat::k16BitPCM48Khz.GetSampleRate();

        if (sample_rate != default_format.GetSampleRate()) {
            target_sample_rate = default_format.GetSampleRate();
        } else {
			target_sample_rate = sample_rate;
        }
        byte_format = ByteFormat::SINT16;
        player_->GetDspManager()->AddPreDSP(
            makeSoxrSampleRateConverter(target_sample_rate));
    }

    setupSampleRateConverter(sample_rate_converter_factory,
                             target_sample_rate,
                             sample_rate_converter_type);
    
    const auto open_dsd_mode = getDsdModes(device_info_.value(),
        entity,
        target_sample_rate);

    try {
	    PlaybackFormat playback_format;

        if (entity.is_zip_file) {
            ArchiveFile archive_file;
            auto result = archive_file.Open(entity.file_path.toStdWString());
            if (!result) {
                return;
            }
            auto entry_result = archive_file.OpenEntry(entity.archive_entry_name.value().toStdWString());
            if (!entry_result) {
                return;
            }
            player_->OpenArchiveEntry(std::move(entry_result.value()),
                device_info_.value(),
                target_sample_rate,
                open_dsd_mode);
        }
        else {
            player_->Open(entity.file_path.toStdWString(),
                device_info_.value(),
                target_sample_rate,
                open_dsd_mode);
        }	    

        player_->SeFileCacheMode(entity.isHttpUrl());

        if (sample_rate_converter_factory != nullptr) {
            if (player_->GetInputFormat().GetSampleRate() == target_sample_rate) {
                player_->GetDspManager()->RemoveSampleRateConverter();
            }
            else {
                sample_rate_converter_factory();
            }
        }

        if (open_dsd_mode == DsdModes::DSD_MODE_PCM) {
            setupDsp(entity);
        }
        setupSampleWriter(byte_format, playback_format);

        playback_format.bit_rate = entity.bit_rate;
        // note: 某些DAC(ex: Dx3Pro)再撥放DSD的時候需要將音量設置最大.
        if (player_->GetDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->GetDsdModes() == DsdModes::DSD_MODE_NATIVE) {
            showMeMessage(tr("Play DSD file need set 100% volume."));
            player_->SetVolume(100);
        }

        player_->BufferStream(0, entity.offset, entity.duration);

        ui_.mutedButton->updateState();
        open_done = true;
        updateUi(playlist_page,
            entity,
            playback_format,
            open_done);
        if (playlist_page != nullptr) {
            playlist_page->spinner()->stopAnimation();
        }        
        return;
    }
    catch (...) {
        open_done = false;
        player_->Stop(false, true);
        logAndShowMessage(std::current_exception());
        if (playlist_page != nullptr) {
            playlist_page->spinner()->stopAnimation();
        }
    }
    
    if (last_playlist_page_ != nullptr) {
        last_playlist_page_->playlist()->reload();
    }   
}

void Xamp::ensureLocalOnePlaylistPage() {
	if (playlist_tab_page_->tabWidget()->count() == 0) {
		const auto playlist_id = 
            qDaoFacade.playlist_dao.addPlaylist(tr("Playlist"), 
                0,
                StoreType::PLAYLIST_LOCAL_STORE);
		newPlaylistPage(playlist_tab_page_->tabWidget(), 
            playlist_id,
            kEmptyString, 
            tr("Playlist"));
	}
}

void Xamp::updateUi(PlaylistPage* playlist_page,
    const PlayListEntity& entity,
    const PlaybackFormat& playback_format, 
    bool open_done) {
    qTheme.setPlayOrPauseButton(ui_.playButton, open_done);

    const int64_t max_duration_ms = Round(entity.duration) * 1000;
    ui_.seekSlider->setRange(0, max_duration_ms);
    ui_.seekSlider->setValue(0);

    ensureLocalOnePlaylistPage();

    library_page_->album()->setPlayingAlbumId(entity.album_id);
    updateButtonState(ui_.playButton, player_->GetState());

    const QFontMetrics title_metrics(ui_.titleLabel->font());
    const QFontMetrics artist_metrics(ui_.artistLabel->font());
    
    ui_.titleLabel->setText(title_metrics.elidedText(entity.title,
        Qt::ElideRight, ui_.titleLabel->width()));
    ui_.artistLabel->setText(artist_metrics.elidedText(entity.artist,
        Qt::ElideRight, ui_.artistLabel->width()));

    lrc_page_->lyrics()->loadFile(kEmptyString);
    if (!entity.yt_music_album_id.isEmpty()) {
        auto cache_path = qAppSettings.getOrCreateLrcCachePath()
    	+ entity.file_name.trimmed();
        lrc_page_->lyrics()->loadFile(cache_path);
    } else {
        lrc_page_->lyrics()->loadFile(entity.file_path);
    }
    
    lrc_page_->setPlayListEntity(entity);

    emit searchLyrics(entity);
    lrc_page_->disableLoadLrcButton();
    //emit transcribeFile(entity.file_path);

    if (playlist_page != nullptr) {
        playlist_page->format()->setText(
            format2String(playback_format, entity.file_extension));
    }
    
    lrc_page_->title()->setText(entity.title);
    lrc_page_->album()->setText(entity.album);
    lrc_page_->artist()->setText(entity.artist);
    lrc_page_->format()->setText(format2String(playback_format, entity.file_extension));

    if (current_entity_.has_value()) {
        qTheme.setHeartButton(ui_.heartButton, current_entity_.value().heart);
    }    

    if (playlist_page != nullptr) {
        if (playlist_page == file_explorer_page_->playlistPage()) {
            file_explorer_page_->waveformWidget()->loadFile(entity.file_path.toStdWString());
            file_explorer_page_->waveformWidget()->setSampleRate(entity.getDopSampleRate());
            file_explorer_page_->waveformWidget()->setTotalDuration(entity.duration);
            setPlaylistPageCover(nullptr, playlist_page);
            onSetCover(entity.validCoverId(), nullptr);
        }
        else {
            file_explorer_page_->playlistPage()->playlist()->setNowPlayState(PLAY_CLEAR);
            file_explorer_page_->waveformWidget()->clear();
            onSetCover(entity.validCoverId(), playlist_page);
            if (!entity.isHttpUrl()) {
                emit findAlbumCover(DatabaseCoverId(entity.music_id, entity.album_id));
            }
        }
    }	

    qDaoFacade.music_dao.updateMusicPlays(entity.music_id);
    qDaoFacade.album_dao.updateAlbumPlays(entity.album_id);

    lrc_page_->spectrum()->reset();
    lrc_page_->spectrum()->start();
    
    if (open_done) {
        setCover(entity.validCoverId());
    }

    player_->Play();

    update();
}

void Xamp::setCover(const QString &cover_id) {
	// Lazy load qImageCache cache image.
    QPixmap cover;
	if (cover_id.isEmpty()) {
        cover = qTheme.defaultSizeUnknownCover();
	}
	else {
        cover = qImageCache.getOrAddDefault(cover_id, true);
	}

    const QSize cover_size(ui_.coverLabel->size().width() - image_util::kPlaylistImageRadius,
        ui_.coverLabel->size().height() - image_util::kPlaylistImageRadius);
    const auto ui_cover = image_util::roundImage(
        image_util::resizeImage(cover, cover_size, false),
        image_util::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);

    lrc_page_->setCover(cover);
}

void Xamp::onUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info) {
    const auto disc_id = QString::fromStdString(mb_disc_id_info.disc_id);
    const auto album = QString::fromStdWString(mb_disc_id_info.album);
    const auto artist = QString::fromStdWString(mb_disc_id_info.artist);
    
    if (!album.isEmpty()) {
        qDaoFacade.album_dao.updateAlbumByDiscId(disc_id, album);
    }
    if (!artist.isEmpty()) {
        qDaoFacade.artist_dao.updateArtistByDiscId(disc_id, artist);
    }

	const auto album_id = qDaoFacade.album_dao.getAlbumIdByDiscId(disc_id);

    if (!mb_disc_id_info.tracks.empty()) {
        QList<PlayListEntity> entities;
        qDaoFacade.album_dao.forEachAlbumMusic(album_id, [&entities](const auto& entity) {
            entities.append(entity);
        });
        std::sort(entities.begin(), entities.end(), [](const auto& a, const auto& b) {
            return b.track > a.track;
        });
        auto i = 0;
        Q_FOREACH(const auto track, mb_disc_id_info.tracks) {
            qDaoFacade.music_dao.updateMusicTitle(entities[i++].music_id, 
                QString::fromStdWString(track.title));
        }
    }    

    if (!album.isEmpty()) {
        cd_page_->playlistPage()->title()->setText(album);
        cd_page_->playlistPage()->playlist()->reload();
    }

    if (const auto album_stats = qDaoFacade.album_dao.getAlbumStats(album_id)) {
        cd_page_->playlistPage()->format()->setText(tr("%1 Songs, %2, %3")
            .arg(QString::number(album_stats.value().songs))
            .arg(formatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year)));
    }
}

void Xamp::onFetchMbDiscInfoCompleted(const MbDiscIdInfo &mb_disc_id_info) {
    const auto album_id = qDaoFacade.album_dao.getAlbumIdByDiscId(
        QString::fromStdString(mb_disc_id_info.disc_id));
    if (album_id != kInvalidDatabaseId) {
        qDaoFacade.album_dao.forEachAlbumMusic(album_id, [&mb_disc_id_info](const auto &entity) {
            qDaoFacade.artist_dao.updateArtist(entity.artist_id, QString::fromStdWString(mb_disc_id_info.artist));
			auto itr = std::find_if(mb_disc_id_info.tracks.begin(), mb_disc_id_info.tracks.end(),
                [&entity](const auto& track) {
                    return entity.track == track.track;
				});
            if (itr != mb_disc_id_info.tracks.end()) {
                qDaoFacade.music_dao.updateMusicTitle(entity.music_id, 
                    QString::fromStdWString(itr->title));
            }
            });
    }
    cd_page_->playlistPage()->playlist()->reload();
    cd_page_->showPlaylistPage(true);
    library_page_->reload();
}

void Xamp::onFetchPlaylistTrackCompleted(PlaylistPage* playlist_page, const std::vector<playlist::Track>& tracks) {
	std::wstring prev_album;

    for (const auto& track : tracks) {
        TrackInfo track_info;
        auto is_unknown_album = false;

        track_info.title = track.title.toStdWString();

        track_info.album = qDatabaseFacade.unknownAlbum().toStdWString();
        if (track.album) {
            track_info.album = kYoutubeMusicLibraryAlbumPrefix + track.album.value().name.toStdWString();
        }

        if (prev_album != track_info.album) {
            prev_album = track_info.album;
        }
        track_info.track = 0;

        QString thumbnail_url;
        if (!track.thumbnails.empty()) {
            thumbnail_url = track.thumbnails.back().url;
        }

        track_info.artist = qDatabaseFacade.unknownArtist().toStdWString();
        if (!track.artists.empty()) {
            track_info.artist = String::ToString(track.artists.front().name.toStdString());
        }

        if (track.duration) {
            track_info.duration = parseDuration(track.duration.value().toStdString());
        }
        else {
            track_info.duration = 0;
        }

        if (track_info.album == qDatabaseFacade.unknownAlbum().toStdWString()) {
            track_info.track = 0;
            is_unknown_album = true;
        }

        if (!track.video_id) {
            continue;
        }

        track_info.file_path = makeYtMusicPath(track);
        track_info.rating = track.like_status.value() == QString("LIKE"_str);
        track_info.sample_rate = kYtMusicSampleRate;
        if (track.album && track.album->id) {
            track_info.yt_album_id = track.album->id.value().toStdString();
        }
        if (!track.artists.empty() && track.artists.front().id) {
            track_info.yt_artist_id = track.artists.front().id->toStdString();
        }

        const std::forward_list<TrackInfo> track_infos{ track_info };

        auto found_cover = [thumbnail_url, is_unknown_album, this](auto music_id, auto album_id, const QString&) {
            if (thumbnail_url.isEmpty()) {
                return;
            }
            DatabaseCoverId id;
            id.first = music_id;
            if (!is_unknown_album) {
                id.second = album_id;
            }
            auto big_thumbnail_url = make544xh544Image(thumbnail_url);
            emit fetchThumbnailUrl(id, big_thumbnail_url);
            };

        qDatabaseFacade.insertTrackInfo(track_infos, 
            playlist_page->playlist()->playlistId(), 
            StoreType::CLOUD_STORE,
            kEmptyString, 
            found_cover);

        auto artist_id = qDaoFacade.artist_dao.getArtistId(track.artists.front().name);
        if (artist_id != kInvalidDatabaseId) {
            auto cover_id = qDaoFacade.artist_dao.getArtistCoverId(artist_id);
            if (!track_info.yt_artist_id.empty() && !qImageCache.contains(cover_id)) {
                cacheAristPageImage(artist_id, QString::fromStdString(track_info.yt_artist_id));
            }
        }
    }
    playlist_page->playlist()->reload();
}

void Xamp::onNavigateToArtistPage(int32_t artist_id, const QString& yt_artist_id) {
    if (yt_artist_id.isEmpty()) {
        XMessageBox::showInformation(tr("Not found artist."));
        return;
    }

    ytmusic_service_->fetchArtist(
        yt_artist_id)
        .then([this, artist_id](const std::optional<artist::Artist>& artist_opt) {
        if (!artist_opt.has_value()) {
            XMessageBox::showInformation(tr("Not found artist."));
            return;
		}

		auto artist = artist_opt.value();
        if (!artist.songs.has_value() || artist.songs.value().results.empty()) {
            XMessageBox::showInformation(tr("Not found artist."));
            return;
        }

        if (!artist.albums.has_value() || artist.albums.value().results.empty()) {
            XMessageBox::showInformation(tr("Not found artist."));
            return;
        }

        auto tab_title = artist.name;
        auto* artist_info_page = new ArtistInfoPage(yt_music_tab_page_->tabWidget());
        artist_info_page->setTitle(tab_title);
        artist_info_page->setArtistId(artist_id);

        (void)QObject::connect(artist_info_page, 
            &ArtistInfoPage::browseAlbum,
            this, 
            &Xamp::onNavigateToArtistAlbumPage);

        auto cover_id = qDaoFacade.artist_dao.getArtistCoverId(artist_id);        

        QPixmap image;
        if (qImageCache.contains(cover_id)) {
            image = qImageCache.getOrDefault(kEmptyString, cover_id);
        } else {
            XAMP_LOG_DEBUG("Not found artist cover : {}", String::ToString(tab_title.toStdWString()));
        }

        artist_info_page->setArtistImage(image);
        artist_info_page->setDescription(artist.description);

        yt_music_tab_page_->tabWidget()->createNewTab(tab_title, artist_info_page);

        for (const auto& song : artist.songs.value().results) {
            QPixmap cover;
            if (qImageCache.contains(song.video_id)) {
                cover = qImageCache.getOrAddDefault(song.video_id);
            } else {
                emit fetchYoutubeThumbnailUrl(song.video_id,
                    song.thumbnails.front().url);
            }
            artist_info_page->insertSong(
                song.album.name,
                song.title,
                artist.name,
                song.video_id,
                cover);
        }

        for (const auto& album : artist.albums.value().results) {
            QPixmap cover;
            if (qImageCache.contains(album.browse_id)) {
                cover = qImageCache.getOrAddDefault(album.browse_id);
            }
            else {
                emit fetchYoutubeThumbnailUrl(album.browse_id,
                    album.thumbnails.front().url);
            }
            artist_info_page->insertAlbum(
                album.title,
                album.type,
                album.browse_id,
                cover);
        }

        auto thumbnail_url = artist.thumbnails.back().url;
        emit fetchArtistThumbnailUrl(artist_id, 
            thumbnail_url);
        });
}

void Xamp::cacheAristPageImage(int32_t artist_id, const QString& yt_artist_id) {
    ytmusic_service_->fetchArtist(
        yt_artist_id)
        .then([this, artist_id](const std::optional<artist::Artist>& artist_opt) {
        if (!artist_opt.has_value()) {
            return;
        }
        auto artist = artist_opt.value();

        if (artist.songs.has_value() && !artist.songs.value().results.empty()) {
            for (const auto& song : artist.songs.value().results) {
                if (!qImageCache.contains(song.video_id)) {
                    emit fetchYoutubeThumbnailUrl(song.video_id,
                        song.thumbnails.front().url);
                }
            }
        }

        if (artist.albums.has_value() && !artist.albums.value().results.empty()) {
            for (const auto& album : artist.albums.value().results) {
                if (!qImageCache.contains(album.browse_id)) {
                    emit fetchYoutubeThumbnailUrl(album.browse_id,
                        album.thumbnails.front().url);
                }
            }
        }

        auto thumbnail_url = artist.thumbnails.back().url;
        emit fetchArtistThumbnailUrl(artist_id,
            thumbnail_url);
		});
}

void Xamp::onNavigateToArtistAlbumPage(const QPixmap& album_cover, const QString& yt_music_album_id) {
    ytmusic_service_->fetchAlbum(yt_music_album_id).then([this, album_cover, yt_music_album_id](const std::optional<album::Album>& album_opt) {
        if (!album_opt.has_value()) {
            XMessageBox::showInformation(tr("Not found album."));
            return;
        }

        auto album = album_opt.value();
        std::wstring prev_album;

        auto title = album.title;

        auto tab_index = yt_music_tab_page_->tabWidget()->count();

        const auto playlist_id =
            qDaoFacade.playlist_dao.addPlaylist(title,
                tab_index,
                StoreType::CLOUD_STORE);

        auto* playlist_page = newPlaylistPage(yt_music_tab_page_->tabWidget(),
            playlist_id,
            kEmptyString,
            title);

        playlist_page->pageTitle()->hide();
        playlist_page->hidePlaybackInformation(true);
        playlist_page->playlist()->setPlayListGroup(PLAYLIST_GROUP_NONE);
        playlist_page->playlist()->enableCloudMode(true);
        playlist_page->playlist()->reload();

        std::string yt_artist_id;
        if (!album.artists.empty() && album.artists.front().id) {
            yt_artist_id = album.artists.front().id.value().toStdString();            
        }

        for (const auto& track : album.tracks) {
            TrackInfo track_info;
            auto is_unknown_album = false;

            track_info.title = track.title.toStdWString();

            track_info.album = qDatabaseFacade.unknownAlbum().toStdWString();
            if (track.album) {
                track_info.album = track.album.value().toStdWString();
            }

            if (prev_album != track_info.album) {
                prev_album = track_info.album;
            }
            track_info.track = track.track;

            auto thumbnail_url = album.thumbnails.back().url;

            track_info.artist = qDatabaseFacade.unknownArtist().toStdWString();
            if (!track.artists.empty()) {
                track_info.artist = track.artists.front().name.toStdWString();
            }

            if (track.duration) {
                track_info.duration = parseDuration(track.duration.value().toStdString());
            }
            else {
                track_info.duration = 0;
            }

            if (track_info.album == qDatabaseFacade.unknownAlbum().toStdWString()) {
                track_info.track = 0;
                is_unknown_album = true;
            }

            if (!track.video_id) {
                continue;
            }

            track_info.file_path = makeYtMusicPath(track);
            track_info.rating = track.like_status.value() == QString("LIKE"_str);
            track_info.sample_rate = kYtMusicSampleRate;
            track_info.yt_album_id = yt_music_album_id.toStdString();
            track_info.yt_artist_id = yt_artist_id;

            auto found_cover = [thumbnail_url, is_unknown_album, album_cover, this](auto music_id, auto album_id, const QString&) {
                if (thumbnail_url.isEmpty()) {
                    return;
                }
                DatabaseCoverId id;
                id.first = music_id;
                if (!is_unknown_album) {
                    id.second = album_id;
                }
                if (!album_cover.isNull()) {
                    auto cover_id = qImageCache.addImage(album_cover);
                    qDaoFacade.album_dao.setAlbumCover(album_id, cover_id);
                }
                else {
                    auto big_thumbnail_url = make544xh544Image(thumbnail_url);
                    emit fetchThumbnailUrl(id, big_thumbnail_url);
                }
            };

            const std::forward_list<TrackInfo> track_infos{ track_info };
            qDatabaseFacade.insertTrackInfo(track_infos,
                playlist_page->playlist()->playlistId(),
                StoreType::CLOUD_STORE,
                kEmptyString, 
                found_cover);
        }
        playlist_page->playlist()->reload();
        });
}

void Xamp::onNavigateToAlbumPage(const PlayListEntity& entity) {
    ytmusic_service_->fetchAlbum(entity.yt_music_album_id).then([this, entity](const std::optional<album::Album>& album_opt) {
        if (!album_opt.has_value()) {
            XMessageBox::showInformation(tr("Not found album."));
            return;
        }

        auto album = album_opt.value();
        std::wstring prev_album;

        auto title = album.title;

        auto tab_index = yt_music_tab_page_->tabWidget()->count();

        const auto playlist_id =
            qDaoFacade.playlist_dao.addPlaylist(title,
                tab_index,
                StoreType::CLOUD_STORE);

        auto* playlist_page = newPlaylistPage(yt_music_tab_page_->tabWidget(),
            playlist_id,
            kEmptyString,
            title);

        playlist_page->pageTitle()->hide();
        playlist_page->hidePlaybackInformation(true);
        playlist_page->playlist()->setPlayListGroup(PLAYLIST_GROUP_NONE);
        playlist_page->playlist()->setNavigationViewMode(NAVIGATION_VIEW_ALBUM_AND_ARTIST);
        playlist_page->playlist()->enableCloudMode(true);
        playlist_page->playlist()->reload();

        std::string yt_artist_id;
        if (!album.artists.empty() && album.artists.front().id) {
            yt_artist_id = album.artists.front().id.value().toStdString();
            cacheAristPageImage(entity.artist_id, QString::fromStdString(yt_artist_id));
        }

        for (const auto& track : album.tracks) {
            TrackInfo track_info;
            auto is_unknown_album = false;

            track_info.title = track.title.toStdWString();

            track_info.album = qDatabaseFacade.unknownAlbum().toStdWString();
            if (track.album) {
                track_info.album = track.album.value().toStdWString();
            }

            if (prev_album != track_info.album) {
                prev_album = track_info.album;
            }
            track_info.track = track.track;

            auto thumbnail_url = album.thumbnails.back().url;

            track_info.artist = qDatabaseFacade.unknownArtist().toStdWString();
            if (!track.artists.empty()) {
                track_info.artist = track.artists.front().name.toStdWString();
            }

            if (track.duration) {
                track_info.duration = parseDuration(track.duration.value().toStdString());
            }
            else {
                track_info.duration = 0;
            }

            if (track_info.album == qDatabaseFacade.unknownAlbum().toStdWString()) {
                track_info.track = 0;
                is_unknown_album = true;
            }

            if (!track.video_id) {
                continue;
            }

            track_info.file_path = makeYtMusicPath(track);
            track_info.rating = track.like_status.value() == QString("LIKE"_str);
            track_info.sample_rate = kYtMusicSampleRate;
            track_info.yt_album_id = entity.yt_music_album_id.toStdString();
            track_info.yt_artist_id = yt_artist_id;

            auto found_cover = [thumbnail_url, is_unknown_album, this](auto music_id, auto album_id, const QString&) {
                if (thumbnail_url.isEmpty()) {
                    return;
                }
                DatabaseCoverId id;
                id.first = music_id;
                if (!is_unknown_album) {
                    id.second = album_id;
                }
                auto big_thumbnail_url = make544xh544Image(thumbnail_url);
                emit fetchThumbnailUrl(id, big_thumbnail_url);
                };

            const std::forward_list<TrackInfo> track_infos{ track_info };
            qDatabaseFacade.insertTrackInfo(track_infos,
                playlist_page->playlist()->playlistId(),
                StoreType::CLOUD_STORE,
                kEmptyString, 
                found_cover);
        }
        playlist_page->playlist()->reload();
        });
}

void Xamp::onUpdateDiscCover(const QString& disc_id,
    const QString& cover_id) {
	const auto album_id = qDaoFacade.album_dao.getAlbumIdByDiscId(disc_id);
    if (album_id != kInvalidDatabaseId) {
        qDaoFacade.album_dao.setAlbumCover(album_id, cover_id);
    }
}

void Xamp::onUpdateCdTrackInfo(const QString& disc_id,
    const std::forward_list<TrackInfo>& track_infos) {
    qDatabaseFacade.insertTrackInfo(track_infos,
        kCdPlaylistId, 
        StoreType::LOCAL_STORE,
        disc_id,
        nullptr);

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

	// Lazy load qImageCache cache image.
    QPixmap cover;
    if (cover_id.isEmpty()) {
        cover = qTheme.defaultSizeUnknownCover();
    } else {
        cover = qImageCache.getOrAddDefault(cover_id, true);
    }

    if (!cover_id.isEmpty() && cover_id != qImageCache.unknownCoverId()) {
        found_cover = true;
    }

    if (page != nullptr) {
        if (!found_cover) {
            setPlaylistPageCover(nullptr, page);
        }
        else {
            setPlaylistPageCover(&cover, page);
            emit blurImage(cover_id, cover, QSize(500, 500));
        }
    }
    main_window_->setIconicThumbnail(cover);
}

void Xamp::onPlayMusic(int32_t playlist_id, const PlayListEntity& entity) {
    if (entity.file_path.isEmpty()) {
        return;
    }

    main_window_->setTaskbarPlayerPlaying();
    current_entity_ = entity;

    auto* playlist_page = qobject_cast<PlaylistPage*>(sender());
    if (playlist_page != nullptr) {
        playlist_page->spinner()->startAnimation();
        centerParent(playlist_page->spinner());

        auto playlist_tab_widget = qobject_cast<PlaylistTabWidget*>(
            playlist_page->parent()->parent());
        // It's a signal from album page.
        if (playlist_tab_widget != nullptr) {
            playlist_tab_widget->setCurrentNowPlaying();
        }

        last_playlist_ = playlist_page->playlist();
        playlist_page->title()->setText(entity.title);
        playlist_page->setHeart(entity.heart);

        last_playlist_->removePlaying();
        playlist_page->playlist()->setNowPlayState(PlayingState::PLAY_PLAYING);
        playlist_page->playlist()->reload(false);
    }
    else {
        XAMP_LOG_DEBUG("Not found any playlist page");
        return;
    }

    auto *playlist_tab_widget = qobject_cast<PlaylistTabWidget*>(
        playlist_page->parent()->parent());
    if (!playlist_tab_widget) {
        last_playlist_tab_ = yt_music_tab_page_->tabWidget();
    }
    last_playlist_page_ = playlist_page;

    if (entity.isFilePath()) {
        onPlayEntity(playlist_page, entity);
    } else {
        onPlayCloudVideoId(playlist_page, entity);
    }
}

void Xamp::onPlayCloudVideoId(PlaylistPage* playlist_page, const PlayListEntity& entity) {
    auto [video_id, setVideoId] = parseYtMusicPath(entity.file_path);

    ytmusic_service_->fetchSongInfo(video_id).then([this, playlist_page, temp = entity](const
        SongInfo& song_info) {
        auto temp1 = temp;
        temp1.file_path = song_info.download_url;
        if (playlist_page != nullptr) {
            playlist_page->spinner()->stopAnimation();
        }

        XAMP_LOG_DEBUG("Download url: {}", temp1.file_path.toStdString());

    	qDaoFacade.playlist_dao.setNowPlaying(playlist_page->playlist()->playlistId(), temp.playlist_music_id);
        yt_music_tab_page_->tabWidget()->setNowPlaying(playlist_page->playlist()->playlistId());
        playlist_page->playlist()->reload();

    	const QByteArray byte_array = QByteArray::fromBase64(song_info.thumbnail_base64.toUtf8());
        QPixmap image;
        if (image.loadFromData(byte_array)) {
            qDaoFacade.album_dao.setAlbumCover(temp1.album_id, qImageCache.addImage(image));
            lrc_page_->setCover(image_util::resizeImage(image, lrc_page_->coverSizeHint(), true));
        }
        onPlayEntity(playlist_page, temp1);
        if (!lrc_page_->lyrics()->isValid()) {
            lrc_page_->lyrics()->setFullLrc(song_info.lyrics, temp1.duration);
        }
        });
}


void Xamp::playNextItem(int32_t forward, bool is_play) {
    if (!last_playlist_) {
        return;
    }
    const auto count = last_playlist_->model()->rowCount();
    if (count == 0) {
        stopPlay();
		// Don't show message box. when the application will receive other signal.
		// message box will be show again.
        //XMessageBox::showInformation(tr("Not found any playing item."));
        return;
    }

    XAMP_TRY_LOG(
        last_playlist_->play(order_, is_play);
        play_index_ = last_playlist_->currentIndex();
        );
}

void Xamp::onArtistIdChanged(const QString& artist, const QString& /*cover_id*/, int32_t artist_id) {    
    ui_.currentView->setCurrentWidget(library_page_.get());
}

void Xamp::onAddPlaylist(int32_t playlist_id, const QList<int32_t>& music_ids) {
    ensureLocalOnePlaylistPage();

    if (playlist_id == kInvalidDatabaseId) {
        const auto tab_index = playlist_tab_page_->tabWidget()->count();
        const auto playlist_id = 
            qDaoFacade.playlist_dao.addPlaylist(tr("Playlist"),
                tab_index,
                StoreType::PLAYLIST_LOCAL_STORE);
        newPlaylistPage(playlist_tab_page_->tabWidget(),
            playlist_id, 
            kEmptyString,
            tr("Playlist"));
        qDaoFacade.playlist_dao.addMusicToPlaylist(music_ids, playlist_id);
	}
    else {
        qDaoFacade.playlist_dao.addMusicToPlaylist(music_ids, playlist_id);
	}
    emit changePlayerOrder(order_);
}

void Xamp::setPlaylistPageCover(const QPixmap* cover, PlaylistPage* page) {
    if (!cover) {
        cover = &qTheme.unknownCover();
    }

	if (!page) {
        page = localPlaylistPage();
	}

    if (page != nullptr) {
        page->setCover(cover);
    }
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

PlaylistPage* Xamp::newPlaylistPage(PlaylistTabWidget *tab_widget,
    int32_t playlist_id,
    const QString& cloud_playlist_id, 
    const QString& name, bool resize) {
    auto* playlist_page = createPlaylistPage(tab_widget,
        playlist_id, 
        kAppSettingPlaylistColumnName, 
        cloud_playlist_id);
    playlist_page->playlist()->setHeaderViewHidden(false);
    playlist_page->pageTitle()->hide();
    connectPlaylistPageSignal(playlist_page);
    onSetCover(kEmptyString, playlist_page);
    tab_widget->createNewTab(name, playlist_page, resize);
    return playlist_page;
}

void Xamp::initialPlaylist() {
    lrc_page_.reset(new LrcPage(this));
    library_page_.reset(new AlbumArtistPage(this));
    playlist_tab_page_.reset(new PlaylistTabPage(this));

    ytmusic_service_.reset(new YtMusicHttpService());
    yt_music_tab_page_.reset(new PlaylistTabPage(this));
    yt_music_tab_page_->tabWidget()->hidePlusButton();

    ui_.naviBar->addTab(translateText("Playlists"),
        TAB_PLAYLIST,
        qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
    ui_.naviBar->addTab(translateText("File explorer"),
        TAB_FILE_EXPLORER, 
        qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_.naviBar->addTab(translateText("Lyrics"),
        TAB_LYRICS,
        qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
    ui_.naviBar->addTab(translateText("Library"),
        TAB_MUSIC_LIBRARY,
        qTheme.fontIcon(Glyphs::ICON_MUSIC_LIBRARY));    
    ui_.naviBar->addTab(translateText("YouTube playlist"),
        TAB_YT_MUSIC_PLAYLIST,
        qTheme.fontIcon(Glyphs::ICON_YOUTUBE_PLAYLIST));
    ui_.naviBar->addTab(translateText("CD"),
        TAB_CD,
        qTheme.fontIcon(Glyphs::ICON_CD));

    (void)QObject::connect(ui_.naviBar,
        &NavBarListView::clickedTable, [this](auto table_id) {
        setCurrentTab(table_id);
        qAppSettings.setValue(kAppSettingLastTabName,
            ui_.naviBar->tabName(table_id));
        });

    ui_.seekSlider->setEnabled(false);
    ui_.startPosLabel->setText(formatDuration(0));
    ui_.endPosLabel->setText(formatDuration(0));

    if (qAppSettings.valueAsBool(kAppSettingHideNaviBar)) {
        ui_.sliderFrame2->setMaximumWidth(50);
        ui_.naviBar->collapse();
    }
    else {
        ui_.sliderFrame2->setMaximumWidth(180);
        ui_.naviBar->expand();
    }
      
    qDaoFacade.playlist_dao.forEachPlaylist([this](auto playlist_id,
        auto index,
        auto store_type,
        const auto& cloud_playlist_id,
        const auto& name) {
        if (notAddablePlaylist(playlist_id)) {
            return;
        }
        if (store_type == StoreType::LOCAL_STORE
            || store_type == StoreType::PLAYLIST_LOCAL_STORE) {
            auto* playlist_page = newPlaylistPage(playlist_tab_page_->tabWidget(),
                playlist_id, kEmptyString, name, false);
            playlist_page->playlist()->enableCloudMode(false);
            playlist_page->pageTitle()->hide();
        }
        else if (store_type == StoreType::CLOUD_STORE) {
            auto* playlist_page = newPlaylistPage(yt_music_tab_page_->tabWidget(),
                playlist_id, cloud_playlist_id, name);
            playlist_page->pageTitle()->hide();
            playlist_page->hidePlaybackInformation(true);
            playlist_page->playlist()->setPlayListGroup(PLAYLIST_GROUP_NONE);
            if (yt_music_tab_page_->tabWidget()->count() == 1) {
                playlist_page->playlist()->hideColumn(PLAYLIST_TRACK);
            }
            playlist_page->playlist()->setNavigationViewMode(NAVIGATION_VIEW_ALBUM_AND_ARTIST);
            playlist_page->playlist()->enableCloudMode(true);
            playlist_page->playlist()->reload();
        }
    });    

    ensureLocalOnePlaylistPage();

    if (!qDaoFacade.playlist_dao.isPlaylistExist(kAlbumPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(tr("Album Playlist"),
            0, StoreType::LOCAL_STORE);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kCdPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(tr("CD Playlist"),
            0, StoreType::LOCAL_STORE);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kFileSystemPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(tr("FileSystem Playlist"),
            0, StoreType::LOCAL_STORE);
    }
    
    playlist_tab_page_->tabWidget()->restoreTabOrder();
    playlist_tab_page_->tabWidget()->setCurrentTabIndex(
        qAppSettings.valueAsInt(kAppSettingLastPlaylistTabIndex));

    (void)QObject::connect(playlist_tab_page_->tabWidget(),
        &PlaylistTabWidget::createNewPlaylist,
        [this]() {
            const auto tab_index = 
                playlist_tab_page_->tabWidget()->count();
            const auto playlist_id = 
                qDaoFacade.playlist_dao.addPlaylist(tr("Playlist"),
                    tab_index, StoreType::PLAYLIST_LOCAL_STORE);
            newPlaylistPage(playlist_tab_page_->tabWidget(), 
                playlist_id, kEmptyString, tr("Playlist"));
        });

    (void)QObject::connect(playlist_tab_page_->tabWidget(),
        &PlaylistTabWidget::removeAllPlaylist,
        [this]() {
            last_playlist_ = nullptr;
        });

    (void)QObject::connect(playlist_tab_page_->tabWidget(), &PlaylistTabWidget::saveToM3UFile,
        [this](auto playlist_id, const auto& playlist_name) {
            const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
            const auto save_file_name = last_dir + "/"_str + playlist_name + ".m3u"_str;
			getSaveFileName(this, [this, playlist_id, playlist_name](const auto& file_name) {
				if (file_name.isEmpty()) {
					return;
				}

                QStringList tracks;
                Q_FOREACH(const auto& item, 
                    playlist_tab_page_->tabWidget()->findPlaylistPage(playlist_id)->playlist()->items()) {
                    tracks << item.file_path;
                }                
                M3uParser::writeM3UFile(file_name, tracks, playlist_name);
				},
                tr("Save playlist file"),
                save_file_name,
                tr("M3U Files (*.m3u)"));
        });

    (void)QObject::connect(playlist_tab_page_->tabWidget(), &PlaylistTabWidget::loadPlaylistFile,
                            [this](auto playlist_id) {
        const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
        getOpenFileName(this, [this,playlist_id](auto file_name) {
            if (file_name.isEmpty()) {
                return;
            }
            auto playlist_page = playlist_tab_page_->tabWidget()->findPlaylistPage(playlist_id);
            auto entities = M3uParser::parseM3UFile(file_name);
            Q_FOREACH(const auto & file_path, entities) {
                playlist_page->playlist()->append(file_path);
            }
            }, tr("Load playlist file"), last_dir, tr("*.m3u"));
    });

    (void)QObject::connect(yt_music_tab_page_->tabWidget(), &PlaylistTabWidget::reloadAllPlaylist, [this]() {
        initialCloudPlaylist();
        });

    (void)QObject::connect(yt_music_tab_page_->tabWidget(), &PlaylistTabWidget::reloadPlaylist, [this](auto tab_index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(yt_music_tab_page_->tabWidget()->widget(tab_index));
        playlist_page->playlist()->removeAll();
        qDaoFacade.playlist_dao.removePlaylistAllMusic(playlist_page->playlist()->playlistId());
        const auto playlist_id = playlist_page->playlist()->cloudPlaylistId().value();
        QCoro::connect(ytmusic_service_->fetchPlaylist(playlist_id), this, [this, playlist_page](const auto& playlist) {
            XAMP_LOG_DEBUG("Get playlist done!");
            onFetchPlaylistTrackCompleted(playlist_page, playlist.tracks);
            });
        });

    (void)QObject::connect(yt_music_tab_page_->tabWidget(), &PlaylistTabWidget::deletePlaylist, [this](auto text) {
        last_playlist_page_ = yt_music_tab_page_->tabWidget()->playlistPage(0);
        last_playlist_tab_ = yt_music_tab_page_->tabWidget();
        last_playlist_ = last_playlist_page_->playlist();
        });

    (void)QObject::connect(yt_music_tab_page_->tabWidget(), &PlaylistTabWidget::removeAllPlaylist, [this]() {
        
        });

    file_explorer_page_.reset(new FileSystemViewPage(this));
    connectPlaylistPageSignal(file_explorer_page_->playlistPage());

    cd_page_.reset(new CdPage(this));
    cd_page_->playlistPage()->playlist()->setPlaylistId(kCdPlaylistId, kAppSettingCdPlaylistColumnName);
    cd_page_->playlistPage()->playlist()->setHeaderViewHidden(false);
    cd_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    onSetCover(kEmptyString, cd_page_->playlistPage());
    connectPlaylistPageSignal(cd_page_->playlistPage());

    setCover(kEmptyString);

    (void)QObject::connect(file_explorer_page_->waveformWidget(),
        &WaveformWidget::readAudioSpectrogram,
        background_service_.get(),
        &BackgroundService::onReadSpectrogram);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readAudioSpectrogram,
        file_explorer_page_->waveformWidget(),
		&WaveformWidget::setSpectrogramData);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readAudioDataCompleted,
        file_explorer_page_->waveformWidget(),
        &WaveformWidget::doneRead);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::fetchLyricsCompleted,
        lrc_page_.get(),
        &LrcPage::onFetchLyricsCompleted);

    (void)QObject::connect(file_explorer_page_->waveformWidget(),
        &WaveformWidget::playAt, [this](auto value) {
        try {
            player_->Seek(value);
            qTheme.setPlayOrPauseButton(ui_.playButton, true);
            main_window_->setTaskbarPlayingResume();
        }
        catch (...) {
            player_->Stop(false);
            logAndShowMessage(std::current_exception());
        }
        });

    (void)QObject::connect(this,
        &Xamp::addJobs,
        background_service_.get(),
        &BackgroundService::onAddJobs);

    (void)QObject::connect(this,
        &Xamp::blurImage,
        background_service_.get(),
        &BackgroundService::onBlurImage);
#if defined(Q_OS_WIN)
    (void)QObject::connect(this,
        &Xamp::fetchCdInfo,
        background_service_.get(),
        &BackgroundService::onFetchCdInfo);
#endif
    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readCdTrackInfo,
        this,
        &Xamp::onUpdateCdTrackInfo,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::fetchMbDiscInfoCompleted,
        this,
        &Xamp::onFetchMbDiscInfoCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::fetchDiscCoverCompleted,
        this,
        &Xamp::onUpdateDiscCover,
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_.get(),
        &FileSystemViewPage::addPathToPlaylist,
        this,
        &Xamp::appendToPlaylist);    

    (void)QObject::connect(library_page_->album(),
        &AlbumView::removeAll,        
        [this]() {
            
        });

    (void)QObject::connect(library_page_->album(),
        &AlbumView::removeSelectedAlbum,
        [this](auto album_id) {
            QList<QString> music_ids;
            qDaoFacade.album_dao.forEachAlbumMusic(album_id, [&music_ids](auto entity) {
                music_ids.append(QString::number(entity.music_id));
                });
			if (music_ids.isEmpty()) {
				return;
			}
        });

    (void)QObject::connect(this,
        &Xamp::searchLyrics,
        background_service_.get(),
        &BackgroundService::onSearchLyrics);

    (void)QObject::connect(this,
        &Xamp::transcribeFile,
        background_service_.get(),
        &BackgroundService::onTranscribeFile);

    (void)QObject::connect(library_page_->album(),
        &AlbumView::clickedArtist,
        this,
        &Xamp::onArtistIdChanged);    

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::blurImage,
        lrc_page_.get(),
        &LrcPage::setBackground);

    (void)QObject::connect(library_page_->album(),
        &AlbumView::addPlaylist,
        this,
        &Xamp::onAddPlaylist);

    (void)QObject::connect(library_page_->artist()->artistViewPage()->album(),
        &AlbumView::addPlaylist,
        this,
        &Xamp::onAddPlaylist);

    (void)QObject::connect(library_page_->year(),
        &AlbumView::addPlaylist,
        this,
        &Xamp::onAddPlaylist);

    pushWidget(playlist_tab_page_.get());
    pushWidget(file_explorer_page_.get());
    pushWidget(lrc_page_.get());
    pushWidget(library_page_.get());
    pushWidget(cd_page_.get());
    pushWidget(yt_music_tab_page_.get());

    playlist_tab_page_->tabWidget()->onThemeChangedFinished(qTheme.themeColor());
    yt_music_tab_page_->tabWidget()->onThemeChangedFinished(qTheme.themeColor());

    connectPlaylistPageSignal(library_page_->album()->albumViewPage()->playlistPage());
    connectPlaylistPageSignal(library_page_->year()->albumViewPage()->playlistPage());

    ui_.currentView->setCurrentIndex(0);

	last_playlist_page_ = playlist_tab_page_->tabWidget()->playlistPage(0);
}

void Xamp::initialCloudPlaylist() {
    yt_music_tab_page_->tabWidget()->closeAllTab();

    const auto process_dialog = makeProgressDialog(
        tr("Initial cloud playlist"),
        tr(""),
        tr("Cancel"));
    process_dialog->show();
    cloud_playlist_process_count_ = 0;

    QCoro::connect(ytmusic_service_->fetchLibraryPlaylists(), 
        this, 
        [process_dialog, this](const std::optional<QList<LibraryPlaylist>>& playlists_opt) {
        if (!playlists_opt.has_value()) {
            process_dialog->close();
			XMessageBox::showInformation("Not found any playlist."_str);
            return;
		}

        auto playlists = playlists_opt.value();
        int32_t index = 1;
        for (const auto& playlist : playlists) {
            const auto playlist_id = qDaoFacade.playlist_dao.addPlaylist(playlist.title, index++, StoreType::CLOUD_STORE, playlist.playlist_id);
            if (playlist.title == kEpisodesForLaterName) {
                continue;
            }
            auto* playlist_page = newPlaylistPage(yt_music_tab_page_->tabWidget(), playlist_id, playlist.playlist_id, playlist.title);
            playlist_page->pageTitle()->hide();
            playlist_page->hidePlaybackInformation(true);
            playlist_page->playlist()->setPlayListGroup(PLAYLIST_GROUP_NONE);
            playlist_page->playlist()->setCloudPlaylistId(playlist.playlist_id);
            playlist_page->playlist()->enableCloudMode(true);
            playlist_page->playlist()->hideColumn(PLAYLIST_TRACK);
            playlist_page->playlist()->setNavigationViewMode(NAVIGATION_VIEW_ALBUM);
            playlist_page->playlist()->reload();
            QCoro::connect(ytmusic_service_->fetchPlaylist(playlist.playlist_id),
                this, [this, playlists, playlist_page, process_dialog](const auto& playlist) {
                    process_dialog->setValue(cloud_playlist_process_count_++ * 100 / playlists.size() + 1);
                    onFetchPlaylistTrackCompleted(playlist_page, playlist.tracks);
                });
        }
        yt_music_tab_page_->tabWidget()->setCurrentIndex(0);
        });
}

void Xamp::appendToPlaylistId(const QString& file_name, int32_t playlist_id) {
    emit extractFile(file_name, playlist_id, false);
}

void Xamp::appendToPlaylist(const QString& file_name, bool append_to_playlist) {
    auto* playlist_page = localPlaylistPage();
    if (!playlist_page) {
        return;
    }

    if (!append_to_playlist) {
        emit extractFile(file_name, playlist_page->playlist()->playlistId(), false);
        library_page_->reload();
        return;
    }

    XAMP_TRY_LOG(playlist_page->playlist()->append(file_name));
}

void Xamp::addItem(const QString& file_name) {
    ensureLocalOnePlaylistPage();
    appendToPlaylist(file_name, true);
}

void Xamp::pushWidget(QWidget* widget) {
    const auto id = ui_.currentView->addWidget(widget);
    widgets_.push_back(widget);
    ui_.currentView->setCurrentIndex(id);
}

void Xamp::connectPlaylistPageSignal(PlaylistPage* playlist_page) {
    (void)QObject::connect(playlist_page,
        &PlaylistPage::search,
        [playlist_page](const auto& text, Match match) {
            playlist_page->playlist()->search(text);
        });

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::addPlaylistItemFinished,
        library_page_.get(),
        &AlbumArtistPage::reload);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::playMusic,
        playlist_page,
        &PlaylistPage::playMusic);

    (void)QObject::connect(playlist_page,
        &PlaylistPage::playMusic,
        this,
        &Xamp::onPlayMusic);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::editTags,
        this,
        &Xamp::onEditTags);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::scanReplayGain,
        file_system_service_.get(),
        &FileSystemService::scanReplayGain);
    
    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::scanReplayGainCompleted,
        playlist_page->playlist(),
        &PlaylistTableView::updateReplayGain);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::scanReplayGainError, [this]() {
	        
        });

    (void)QObject::connect(playlist_page->playlist()->styledDelegate(),
        &PlaylistStyledItemDelegate::findAlbumCover,
        album_cover_service_.get(),
        &AlbumCoverService::onFindAlbumCover,
        Qt::QueuedConnection);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::navigateToAlbumPage,
        this,
        &Xamp::onNavigateToAlbumPage);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::navigateToArtistPage,
        this,
        &Xamp::onNavigateToArtistPage);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::addPlaylist,
        this,
        [this](const auto& playlist_id, const auto& entities) {
            if (auto* playlist_page = playlist_tab_page_->tabWidget()->findPlaylistPage(playlist_id)) {
                for (const auto& entity : entities) {
                    qDaoFacade.playlist_dao.addMusicToPlaylist(entity.music_id, playlist_id, entity.album_id);
                }
                playlist_page->playlist()->reload();
            }
        });

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::encodeAlacFiles,
        this,
        &Xamp::onEncodeAlacFiles);

    (void)QObject::connect(&qTheme,
        &ThemeManager::themeChangedFinished,
        playlist_page,
        &PlaylistPage::onThemeChangedFinished);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::foundFileCount,
        playlist_page->playlist()->progressPage(),
        &ScanFileProgressPage::onFoundFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFilePath,
        playlist_page->playlist()->progressPage(),
        &ScanFileProgressPage::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileStart,
        playlist_page->playlist()->progressPage(),
        &ScanFileProgressPage::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readCompleted,
        playlist_page->playlist()->progressPage(),
        &ScanFileProgressPage::onReadCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileProgress,
        playlist_page->playlist()->progressPage(),
        &ScanFileProgressPage::onReadFileProgress,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::remainingTimeEstimation,
        playlist_page->playlist()->progressPage(),
        &ScanFileProgressPage::onRemainingTimeEstimation,
        Qt::QueuedConnection);
}

void Xamp::addDropFileItem(const QUrl& url) {
    addItem(url.toLocalFile());
}

PlaylistPage* Xamp::localPlaylistPage() const {
    if (playlist_tab_page_->tabWidget()->tabBar()->count() == 0) {
        return nullptr;
    }
    return dynamic_cast<PlaylistPage*>(playlist_tab_page_->tabWidget()->currentWidget());
}

void Xamp::onCancelRequested() {
    file_system_service_->cancelRequested();
    library_page_->album()->progressPage()->hide();
    if (playlist_tab_page_->tabWidget()->count() > 0) {
        localPlaylistPage()->playlist()->reload();
    }
    library_page_->reload();
}

void Xamp::onInsertDatabase(const std::forward_list<TrackInfo>& result,
    int32_t playlist_id) {
    if (playlist_id == kFileSystemPlaylistId) {
        return;
    }
    qDatabaseFacade.insertTrackInfo(result,
        playlist_id,
        StoreType::PLAYLIST_LOCAL_STORE,
        kEmptyString,
        nullptr);
    ensureLocalOnePlaylistPage();
    localPlaylistPage()->playlist()->reload();
}

void Xamp::onBatchInsertDatabase(const std::vector<std::forward_list<TrackInfo>>& results,
    int32_t playlist_id) {
    if (playlist_id == kFileSystemPlaylistId) {
        return;
    }
    qDatabaseFacade.insertMultipleTrackInfo(results,
        playlist_id,
        StoreType::PLAYLIST_LOCAL_STORE);
    ensureLocalOnePlaylistPage();
    localPlaylistPage()->playlist()->reload();    
    if (playlist_tab_page_->tabWidget()->count() > 0) {
        localPlaylistPage()->playlist()->reload();
    }
    library_page_->reload();
}

void Xamp::onSetAlbumCover(int32_t album_id, const QString& cover_id) {
    try {
        qDaoFacade.album_dao.setAlbumCover(album_id, cover_id);
    }
    catch (const Exception &e) {
        return;
    } catch (...) {
        return;
    }

    library_page_->artist()->artistViewPage()->album()->refreshCover();
    library_page_->album()->refreshCover();
    library_page_->year()->refreshCover();
    file_explorer_page_->playlistPage()->playlist()->setAlbumCoverId(album_id, cover_id);
	
    if (current_entity_) {
        if (current_entity_.value().album_id == album_id) {
            current_entity_.value().cover_id = cover_id;
            onSetCover(cover_id, nullptr);
		}
    }
}

void Xamp::onEditTags(int32_t /*playlist_id*/, const QList<PlayListEntity>& entities) {
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<TagEditPage> tag_edit_page(new TagEditPage(dialog.get(), entities));
    dialog->setContentWidget(tag_edit_page.get());
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EDIT));
    dialog->setTitle(tr("Edit track information"));    
    dialog->exec();
    if (playlist_tab_page_->tabWidget()->count() > 0) {
        localPlaylistPage()->playlist()->reload();
    }
}

void Xamp::onPlaybackError(const QString& message) {
    player_->Stop();
    XMessageBox::showError(message, kApplicationTitle, true);
}

void Xamp::onRetranslateUi() {
    ui_.naviBar->setTabText(tr("Playlists"), TAB_PLAYLIST);
    ui_.naviBar->setTabText(tr("File explorer"), TAB_FILE_EXPLORER);
    ui_.naviBar->setTabText(tr("Lyrics"), TAB_LYRICS);
    ui_.naviBar->setTabText(tr("Library"), TAB_MUSIC_LIBRARY);
    ui_.naviBar->setTabText(tr("CD"), TAB_CD);

    library_page_->onRetranslateUi();
    cd_page_->onRetranslateUi();
    initialDeviceList();
}

