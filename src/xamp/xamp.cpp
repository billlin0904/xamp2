#include <QCoroFuture>
#include <QCoroProcess>
#include <QImageReader>
#include <QInputDialog>
#include <QJsonObject>
#include <QShortcut>
#include <QSimpleUpdater.h>
#include <QToolTip>
#include <QWidgetAction>
#include <QScrollBar>
#include <QScrollArea>

#include <set>
#include <thememanager.h>
#include <version.h>
#include <xamp.h>
#include <style_util.h>

#include <base/dsd_utils.h>
#include <base/logger_impl.h>
#include <base/scopeguard.h>
#include <FramelessHelper/Widgets/framelesswidgetshelper.h>

#include <output_device/api.h>
#include <output_device/iaudiodevicemanager.h>
#include <output_device/win32/sharedwasapidevicetype.h>
#include <player/api.h>
#include <stream/api.h>
#include <stream/compressorconfig.h>
#include <stream/idspmanager.h>

#include <widget/aboutpage.h>
#include <widget/accountauthorizationpage.h>
#include <widget/actionmap.h>
#include <widget/albumartistpage.h>
#include <widget/albumview.h>
#include <widget/appsettings.h>
#include <widget/cdpage.h>
#include <widget/createplaylistview.h>
#include <widget/xtooltip.h>

#include <widget/equalizerview.h>
#include <widget/filesystemviewpage.h>
#include <widget/genre_view_page.h>
#include <widget/http.h>
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
#include <widget/processindicator.h>
#include <widget/spectrumwidget.h>
#include <widget/supereqview.h>
#include <widget/tageditpage.h>
#include <widget/tagio.h>
#include <widget/widget_shared.h>
#include <widget/xdialog.h>
#include <widget/xmessagebox.h>
#include <widget/xprogressdialog.h>
#include <widget/util/image_util.h>
#include <widget/util/mbdiscid_util.h>
#include <widget/util/read_until.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>
#include <widget/worker/backgroundservice.h>
#include <widget/worker/filesystemservice.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/youtubedl/ytmusicservice.h>
#include <widget/youtubedl/ytmusicoauth.h>
#include <widget/youtubedl/ytmusic_disckcache.h>

namespace {
    constexpr auto kYtMusicSampleRate = 48000;

    void showMeMessage(const QString& message) {
        if (qAppSettings.dontShowMeAgain(message)) {
            auto [button, checked] = XMessageBox::showCheckBoxInformation(
                message,
                qApp->tr("Ok, and don't show again."),
                kApplicationTitle,
                false);
            if (checked) {
                qAppSettings.addDontShowMeAgain(message);
            }
        }
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

    std::optional<video_info::Format> findBestAudioFormat(const video_info::VideoInfo& video_info) {
        std::multiset<video_info::Format> formats;
        static const std::string kBestAudioCodec{ "mp4" };
        for (const auto& format : video_info.formats) {            
            if (format.vcodec == "none" && format.acodec.find(kBestAudioCodec) != std::string::npos) {
                formats.insert(format);
                XAMP_LOG_DEBUG("video: {:<15} audio: {:<10} {:<10}", format.vcodec, format.acodec, format.abr);
            }
        }

        if (formats.empty()) {
            return std::nullopt;
        }

        return std::optional<video_info::Format> { std::in_place_t{}, * formats.begin() };
    }

    PlaylistPage* createPlaylistPage(PlaylistTabWidget* tab_widget, int32_t playlist_id, const QString& column_setting_name, const QString& cloud_playlist_id) {
        auto* playlist_page = new PlaylistPage(tab_widget);
        playlist_page->playlist()->setPlaylistId(playlist_id, column_setting_name);
        if (!cloud_playlist_id.isEmpty()) {
            playlist_page->playlist()->setCloudPlaylistId(cloud_playlist_id);
        }
        return playlist_page;
    }

    QString makeYtMusicUrl(const QString& video_id) {
        const auto ytmusic_url = qSTR("https://music.youtube.com/watch?v=%1").arg(video_id);
        return ytmusic_url;
    }

    QCoro::Task<bool> startWebViewProcessAsync(const QString& url) {
        auto program = qApp->applicationDirPath();
        program.append(qTEXT("/YtMusicOAuthView/YtMusicOAuthView.exe"));

        QStringList args;
        args << qTEXT("-") << url;

        QProcess p;
        auto process = qCoro(p);

        if (!co_await process.start(program, args)) {
            co_return false;
        }

        if (!co_await process.waitForFinished(-1)) {
            co_return false;
        }
        co_return true;
    }
}

Xamp::Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer>& player)
    : IXFrame(parent)
    , is_seeking_(false)
    , trigger_upgrade_action_(false)
    , trigger_upgrade_restart_(false)
    , cloud_playlist_process_count_(0)
    , order_(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)
    , main_window_(nullptr)
	, lrc_page_(nullptr)
	, music_page_(nullptr)
	, cd_page_(nullptr)
	, music_library_page_(nullptr)
	, file_explorer_page_(nullptr)
	, background_service_(nullptr)
    , state_adapter_(std::make_shared<UIPlayerStateAdapter>())
	, player_(player) {
    ui_.setupUi(this);
}

Xamp::~Xamp() = default;

QString Xamp::translateDeviceDescription(const IDeviceType* device_type) {
    static const QMap<std::string_view, ConstexprQString> lut{
        { "WASAPI (Exclusive Mode)", QT_TRANSLATE_NOOP("Xamp", "WASAPI (Exclusive Mode)") },
        { "WASAPI (Shared Mode)",    QT_TRANSLATE_NOOP("Xamp", "WASAPI (Shared Mode)") },
        { "Null Output",             QT_TRANSLATE_NOOP("Xamp", "Null Output") },
        { "XAudio2",                 QT_TRANSLATE_NOOP("Xamp", "XAudio2") },
        { "ASIO",                    QT_TRANSLATE_NOOP("Xamp", "ASIO") },
    };
    const auto str = lut.value(device_type->GetDescription(), fromStdStringView(device_type->GetDescription()));
    return tr(str.data());
}

QString Xamp::translateText(const std::string_view& text) {
    /*static const QMap<std::string_view, ConstexprQString> lut{
        { "Playlists",        QT_TRANSLATE_NOOP("Xamp", "Playlists")},
        { "File explorer",    QT_TRANSLATE_NOOP("Xamp", "File explorer") },
        { "Lyrics",           QT_TRANSLATE_NOOP("Xamp", "Lyrics") },
        { "Library",          QT_TRANSLATE_NOOP("Xamp", "Library") },
        { "CD",               QT_TRANSLATE_NOOP("Xamp", "CD") },
        { "YouTube search",   QT_TRANSLATE_NOOP("Xamp", "YouTube search") },
        { "YouTube playlist", QT_TRANSLATE_NOOP("Xamp", "YouTube playlist") },

        { "Hide this column", QT_TRANSLATE_NOOP("Xamp", "Hide this column") },
        { "Select columns to show...", QT_TRANSLATE_NOOP("Xamp", "Select columns to show...") },
        { "Remove all", QT_TRANSLATE_NOOP("PlaylistTableView", "Remove all") },
    };
    const auto str = lut.value(text, kEmptyString);
    return tr(str.data());*/
    return tr(text.data());
}

QString Xamp::translateError(Errors error) {
    using xamp::base::Errors;
    static const QMap<xamp::base::Errors, ConstexprQString> lut{
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
    }

    auto quit_and_wait_thread = [](auto& thread) {
        if (!thread.isFinished()) {
            thread.requestInterruption();
            thread.quit();
            thread.wait();
        }
    };

    emit cancelRequested();

    if (ytmusic_service_ != nullptr) {
        ytmusic_service_->cleanupAsync().waitForFinished();
    }

    quit_and_wait_thread(background_service_thread_);
    quit_and_wait_thread(album_cover_service_thread_);
    quit_and_wait_thread(file_system_service_thread_);
    quit_and_wait_thread(ytmusic_service_thread_);

    if (main_window_ != nullptr) {
        (void)main_window_->saveGeometry();
    }

    playlist_tab_page_->saveTabOrder();
    XAMP_LOG_DEBUG("Xamp destroy!");
}

void Xamp::initialYtMusicService() {
    if (ytmusic_service_) {
        return;
    }
	
    ytmusic_service_.reset(new YtMusicService());
    ytmusic_service_->moveToThread(&ytmusic_service_thread_);
	ytmusic_service_thread_.start();

    XAMP_LOG_DEBUG("Initial ytmusic service...");

    QCoro::connect(ytmusic_service_->initialAsync(), this, []() {
#ifdef Q_OS_WIN
        if (qAppSettings.valueAsBool(kAppSettingEnableSandboxMode)) {
            XAMP_LOG_DEBUG("Set process mitigation.");
            SetProcessMitigation();
        }
#endif
        XAMP_LOG_DEBUG("Initial ytmusic worker done.");
        });
}

void Xamp::setMainWindow(IXMainWindow* main_window) {
    FramelessWidgetsHelper::get(this)->setTitleBarWidget(ui_.titleFrame);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.minWinButton, SystemButtonType::Minimize);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.maxWinButton, SystemButtonType::Maximize);
    FramelessWidgetsHelper::get(this)->setSystemButton(ui_.closeButton, SystemButtonType::Close);
    FramelessWidgetsHelper::get(this)->setHitTestVisible(ui_.menuButton);
    FramelessWidgetsHelper::get(this)->extendsContentIntoTitleBar();

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

    background_service_.reset(new BackgroundService());
    background_service_->moveToThread(&background_service_thread_);
    background_service_thread_.start(QThread::LowestPriority);

    album_cover_service_.reset(new AlbumCoverService());
    album_cover_service_->moveToThread(&album_cover_service_thread_);
    album_cover_service_thread_.start(QThread::NormalPriority);

    album_cover_service_->mergeUnknownAlbumCover();

    file_system_service_.reset(new FileSystemService());
    file_system_service_->moveToThread(&file_system_service_thread_);
    file_system_service_thread_.start(QThread::LowestPriority);

    if (background_service_ != nullptr) {
        (void)QObject::connect(this, &Xamp::cancelRequested, background_service_.get(), &BackgroundService::cancelRequested);
    }
    if (album_cover_service_ != nullptr) {
        (void)QObject::connect(this, &Xamp::cancelRequested, album_cover_service_.get(), &AlbumCoverService::cancelRequested);
    }
    if (file_system_service_ != nullptr) {
        (void)QObject::connect(this, &Xamp::cancelRequested, file_system_service_.get(), &FileSystemService::cancelRequested);
    }
    if (ytmusic_service_ != nullptr) {
        (void)QObject::connect(this, &Xamp::cancelRequested, ytmusic_service_.get(), &YtMusicService::cancelRequested);
    }

    ytmusic_oauth_.reset(new YtMusicOAuth());

    (void)QObject::connect(ui_.authButton, &QToolButton::clicked, [this](auto checked) { 
        auto token = YtMusicOAuth::parseOAuthJson();
        if (token) {            
            const QScopedPointer<XDialog> dialog(new XDialog(this));
            const QScopedPointer<AccountAuthorizationPage> authorization_page(new AccountAuthorizationPage(dialog.get()));
            dialog->setContentWidget(authorization_page.get());
            dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
            dialog->setTitle(tr("Account Authorization"));
            authorization_page->setOAuthToken(token.value());
            dialog->exec();
        } else {
            XAMP_LOG_ERROR("{}", token.error());
            if (XMessageBox::showYesOrNo(tr(" Would you like to proceed with authorization at the moment?"),
                QString::fromStdString(token.error())) != QDialogButtonBox::Yes) {
                return;
            }
            ytmusic_oauth_->setup();
        }
    });

    (void)QObject::connect(ytmusic_oauth_.get(), &YtMusicOAuth::acceptAuthorization, [this](const auto &url) {
        startWebViewProcessAsync(url)
            .then([this](auto ret) {
            if (ret) {
                ytmusic_oauth_->requestGrant();
            }            
        });
    });

    (void)QObject::connect(ytmusic_oauth_.get(), &YtMusicOAuth::requestGrantCompleted, [this]() {
        auto token = YtMusicOAuth::parseOAuthJson();
        if (token) {
            initialYtMusicService();
            initialCloudPlaylist();
            setAuthButton(ui_, true);
        }
        else {
            XAMP_LOG_ERROR("{}", token.error());
        }
    });

    player_->SetStateAdapter(state_adapter_);
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
        ui_.naviBar->setCurrentIndex(ui_.naviBar->model()->index(tab_id, 0));
        setCurrentTab(tab_id);
    }
    else {
        ui_.naviBar->setCurrentIndex(ui_.naviBar->model()->index(0, 0));
        setCurrentTab(0);
    }

    (void)QObject::connect(this,
        &Xamp::translation,
        background_service_.get(),
        &BackgroundService::onTranslation);

    (void)QObject::connect(background_service_.get(), 
        &BackgroundService::translationCompleted,
        this,
        &Xamp::onTranslationCompleted);

    (void)QObject::connect(album_cover_service_.get(),
        &AlbumCoverService::setAlbumCover,
        this, 
        &Xamp::onSetAlbumCover);

    (void)QObject::connect(this,
        &Xamp::fetchThumbnailUrl,
        album_cover_service_.get(),
        &AlbumCoverService::onFetchThumbnailUrl);

    (void)QObject::connect(album_cover_service_.get(),
        &AlbumCoverService::setThumbnail,
        this,
        &Xamp::onSetThumbnail);

    (void)QObject::connect(album_cover_service_.get(),
        &AlbumCoverService::fetchThumbnailUrlError,
        this,
        &Xamp::onFetchThumbnailUrlError);

    (void)QObject::connect(&qTheme, 
        &ThemeManager::themeChangedFinished, 
        this, 
        &Xamp::onThemeChangedFinished);

    (void)QObject::connect(&qTheme,
        &ThemeManager::themeChangedFinished,
        playlist_tab_page_.get(),
        &PlaylistTabWidget::onThemeChangedFinished);

    (void)QObject::connect(&qTheme,
        &ThemeManager::themeChangedFinished,
        yt_music_tab_page_.get(),
        &PlaylistTabWidget::onThemeChangedFinished);

    (void)QObject::connect(&qTheme,
        &ThemeManager::themeChangedFinished,
        ui_.naviBar,
        &TabListView::onThemeChangedFinished);

    (void)QObject::connect(&qTheme,
        &ThemeManager::themeChangedFinished,
        music_library_page_->album(),
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
        music_library_page_.get(),
        &AlbumArtistPage::onThemeChangedFinished);    

    (void)QObject::connect(this,
        &Xamp::setWatchDirectory,
        file_system_service_.get(),
        &FileSystemService::onSetWatchDirectory,
        Qt::QueuedConnection);

    (void)QObject::connect(this,
        &Xamp::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile,
        Qt::QueuedConnection);

   (void)QObject::connect(music_library_page_->album(),
        &AlbumView::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile,
        Qt::QueuedConnection);

   (void)QObject::connect(music_library_page_->album()->styledDelegate(),
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
        &FileSystemService::foundFileCount,
        this,
        &Xamp::onFoundFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::insertDatabase,
        this,
        &Xamp::onInsertDatabase,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFilePath,
        this,
        &Xamp::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileStart,
        this,
        &Xamp::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readCompleted,
        this,
        &Xamp::onReadCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::readFileProgress,
        this,
        &Xamp::onReadFileProgress,
        Qt::QueuedConnection);

    (void)QObject::connect(file_system_service_.get(),
        &FileSystemService::remainingTimeEstimation,
        this,
        &Xamp::onRemainingTimeEstimation,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::foundFileCount,
        this,
        &Xamp::onFoundFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readFilePath,
        this,
        &Xamp::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readFileStart,
        this,
        &Xamp::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readCompleted,
        this,
        &Xamp::onReadCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readFileProgress,
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

            QVersionNumber latest_version_value;
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

    auto* menu = new XMenu(ui_.menuButton);
    const auto * preference_action = menu->addAction(qTheme.fontIcon(Glyphs::ICON_SETTINGS), tr("Preference"));
    (void)QObject::connect(preference_action, &QAction::triggered, [this]() {
        const QScopedPointer<XDialog> dialog(new XDialog(this));
        const QScopedPointer<PreferencePage> preference_page(new PreferencePage(dialog.get()));
        (void)QObject::connect(preference_page.get(), &PreferencePage::retranslateUi, this, &Xamp::onRetranslateUi);
        preference_page->loadSettings();
        dialog->setContentWidget(preference_page.get());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
        dialog->setTitle(tr("Preference"));
        dialog->exec();
        preference_page->saveAll();
        });

    const auto* mini_to_tray_action = menu->addAction(qTheme.fontIcon(Glyphs::ICON_SETTINGS), tr("Minimize To Tray"));
    (void)QObject::connect(mini_to_tray_action, &QAction::triggered, [this]() {
        });
   
    const auto* about_action = menu->addAction(qTheme.fontIcon(Glyphs::ICON_ABOUT),tr("About"));
    (void)QObject::connect(about_action, &QAction::triggered, [this]() {
        const QScopedPointer<XDialog> dialog(new XDialog(this));
        const QScopedPointer<AboutPage> about_page(new AboutPage(dialog.get()));
        (void)QObject::connect(about_page.get(), &AboutPage::CheckForUpdate, this, &Xamp::onCheckForUpdate);
        (void)QObject::connect(about_page.get(), &AboutPage::RestartApp, this, &Xamp::onRestartApp);
        (void)QObject::connect(this, &Xamp::updateNewVersion, about_page.get(), &AboutPage::OnUpdateNewVersion);
        dialog->setContentWidget(about_page.get());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_ABOUT));
        dialog->setTitle(tr("About"));
        emit about_page->CheckForUpdate();
        dialog->exec();
        });
    ui_.menuButton->setPopupMode(QToolButton::InstantPopup);
    ui_.menuButton->setMenu(menu);

    setPlayerOrder();
    initialDeviceList();
    
    (void)QObject::connect(&ui_update_timer_timer_, &QTimer::timeout, this, &Xamp::onDelayedDownloadThumbnail);
    emit setWatchDirectory(qAppSettings.myMusicFolderPath());
}

void Xamp::onRestartApp() {
    trigger_upgrade_restart_ = true;
    qApp->exit(kRestartExistCode);
}

void Xamp::onFetchPlaylistTrackCompleted(PlaylistPage* playlist_page, const std::vector<playlist::Track>& tracks) {
    std::wstring prev_album;
    auto track_no = 1;

    playlist_page->spinner()->startAnimation();

    for (const auto& track : tracks) {
        TrackInfo track_info;
        auto is_unknown_album = false;

        track_info.title = String::ToString(track.title);

        track_info.album = qDatabaseFacade.unknownAlbum().toStdWString();
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

        track_info.artist = qDatabaseFacade.unknownArtist().toStdWString();
        if (!track.artists.empty()) {
            track_info.artist = String::ToString(track.artists.front().name);
        }        

        if (track.duration) {
            track_info.duration = parseDuration(track.duration.value());
        }
        else {
            track_info.duration = 0;
        }

        if (track_info.album == qDatabaseFacade.unknownAlbum().toStdWString()) {
            track_info.track = 1;
            is_unknown_album = true;
        }

        if (!track.video_id /*|| !track.set_video_id*/) {
            continue;
        }

        track_info.file_path = makeYtMusicPath(track);
        track_info.rating = track.like_status == "LIKE";        
        track_info.sample_rate = kYtMusicSampleRate;

        const ForwardList<TrackInfo> track_infos{ track_info };
        DatabaseFacade facade;
        facade.insertTrackInfo(track_infos, playlist_page->playlist()->playlistId(), StoreType::CLOUD_STORE, [=, this](auto music_id, auto album_id) {
            if (thumbnail_url.empty()) {
                return;
            }
            DatabaseCoverId id;
            id.first = music_id;
            if (!is_unknown_album) {
                id.second = album_id;
            }
            emit fetchThumbnailUrl(id, QString::fromStdString(thumbnail_url));
        });
    }
    playlist_page->playlist()->reload();
    playlist_page->spinner()->stopAnimation();
}

void Xamp::cacheYtMusicFile(const PlayListEntity& entity) {
    auto [video_id, setVideoId] = parseYtMusicPath(entity.file_path);
    const auto ytmusic_url = makeYtMusicUrl(video_id);    
    auto vid = video_id;    

    const auto dialog = makeProgressDialog(
        tr("Cache file progress"),
        tr("Cache file"),
        tr("Cancel"));
    dialog->show();
    dialog->setLabelText(tr("Fetching video information ..."));

    QCoro::connect(ytmusic_service_->extractVideoInfoAsync(ytmusic_url), this,
        [this, dialog, vid, entity](const auto& video_info) {            
            dialog->setLabelText(tr("Fetching song information ..."));

            QCoro::connect(ytmusic_service_->fetchSongAsync(vid), this, [this, dialog, video_info, entity](const auto& song) {
                auto file_name = YtMusicDiskCache::makeFileCachePath(QString::fromStdString(song.value().video_id));
                
                dialog->setLabelText(tr("Start download ") + file_name + qTEXT(" ..."));
                
                auto process_handler = [dialog](auto ready, auto total) {
                    dialog->setValue(ready * 100 / total);
                    qApp->processEvents();
                    };

                auto download_file_handler = [dialog](auto) {
                    dialog->close();
                    };

                auto error_handler = [dialog](auto url, auto message) {
                    dialog->close();
                    };

                if (auto best_format = findBestAudioFormat(video_info)) {
                    http::HttpClient(QString::fromStdString(best_format.value().url))
                        .progress(process_handler)
                        .downloadFile(file_name, download_file_handler, error_handler);
                    dialog->exec();
                } else {
                    return;
                }                
                
                if (!QFile(file_name).exists()) {
                    XMessageBox::showError(tr("Failed to write cache file."));
                    return;
                }          

                TagIO tag_io;
                tag_io.writeAlbum(file_name.toStdWString(),  entity.album);
                tag_io.writeArtist(file_name.toStdWString(), entity.artist);
                tag_io.writeTitle(file_name.toStdWString(),  QString::fromStdString(song.value().title));
                tag_io.writeTrack(file_name.toStdWString(),  entity.track);

                const auto thumbnail_url = song.value().thumbnail.thumbnails.back().url;

                http::HttpClient(QString::fromStdString(thumbnail_url))
                    .download([file_name, song, this](const auto& content) {
                    QPixmap image;
                    if (!image.loadFromData(content)) {
                        return;
                    }

                    TagIO tag_io;
                    tag_io.writeEmbeddedCover(file_name.toStdWString(), image);
                    qDiskCache.setFileName(QString::fromStdString(song.value().video_id), file_name);                    
                });                

                });
        });
}

void Xamp::playCloudVideoId(const PlayListEntity& entity, const QString &id, bool is_doubleclicked) {
    auto [video_id, setVideoId] = parseYtMusicPath(id);

    if (qDiskCache.isCached(video_id) && !entity.isFilePath()) {
        auto file_entity = qDiskCache.getFileName(video_id);
        auto temp = entity;
        temp.file_path = file_entity.file_name;
        auto track_info = TagIO::getTrackInfo(temp.file_path.toStdWString());
        music_dao_.updateMusic(temp.music_id, track_info);
        temp.sample_rate = track_info.sample_rate;
        onPlayMusic(kInvalidDatabaseId, temp, false, is_doubleclicked);
        return;
    }

    XAMP_LOG_DEBUG("Fetching lyrics ...");
    fetchLyrics(entity, video_id);

    auto* playlist_page = dynamic_cast<PlaylistPage*>(sender());
    if (playlist_page != nullptr) {
        playlist_page->spinner()->startAnimation();
        centerParent(playlist_page->spinner());
    }

    // 取得256kbs AAC條件
    // 1. 白金帳號
    // 2. Browser cookies
    // 3. 使用YouTube music URL
    const auto ytmusic_url = makeYtMusicUrl(video_id);
    const auto vid = video_id;

    XAMP_LOG_DEBUG("Extract video information ...");

    QCoro::connect(ytmusic_service_->extractVideoInfoAsync(ytmusic_url), this,
        [temp = entity, vid, playlist_page, this, is_doubleclicked](const auto& video_info) {
        XAMP_ON_SCOPE_EXIT(
            if (playlist_page != nullptr) {
                playlist_page->spinner()->stopAnimation();
            }
        );

        auto temp1 = temp;
        if (video_info.formats.empty()) {
            XAMP_LOG_DEBUG("Not found song format.");
            return;
        }

        const auto best_format = findBestAudioFormat(video_info);
        if (!best_format) {
            return;
        }
        temp1.file_path = QString::fromStdString(best_format.value().url);
        XAMP_LOG_DEBUG("Download url: {}", temp1.file_path.toStdString());
        onPlayMusic(kInvalidDatabaseId, temp1, false, is_doubleclicked);

        auto album_id = temp.album_id;
        if (album_id == qDatabaseFacade.unknownAlbumId()) {
            return;
        }

		if (isNullOfEmpty(temp1.cover_id)) {
            return;
		}

        XAMP_LOG_DEBUG("Extract song information ...");

        QCoro::connect(ytmusic_service_->fetchSongAsync(vid), this, [this, album_id](const auto& song) {
            if (!song) {
                XAMP_LOG_DEBUG("Not found song information.");
                return;
            }
        	if (song.value().thumbnail.thumbnails.empty()) {
                XAMP_LOG_DEBUG("Not found song thumbnail.");
                return;
            }

            const auto thumbnail_url = song.value().thumbnail.thumbnails.back().url;

            XAMP_LOG_DEBUG("Extract song thumbnail url ...");

            http::HttpClient(QString::fromStdString(thumbnail_url))
                .download([=, this](const auto& content) {
                QPixmap image;
                if (!image.loadFromData(content)) {
                    XAMP_LOG_DEBUG("Failure to load song thumbnail.");
                    return;
                }
                album_dao_.setAlbumCover(album_id, qImageCache.addImage(image));
                lrc_page_->setCover(image_util::resizeImage(image, lrc_page_->coverSizeHint(), true));
				});
            });
        });
}

void Xamp::fetchLyrics(const PlayListEntity& entity, const QString& video_id) {
    QCoro::connect(ytmusic_service_->fetchWatchPlaylistAsync(video_id), this, [entity, this](const auto& playlist) {
        if (playlist.lyrics) {
            QCoro::connect(ytmusic_service_->fetchLyricsAsync(QString::fromStdString(*playlist.lyrics)), this, [entity, this](const auto& lyrics) {
                if (auto lyric = lyrics.lyrics) {
                    lrc_page_->lyrics()->onAddFullLrc(QString::fromStdString(lyric.value()));
                }
            });
        }
    });
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
        auto is_unknown_album = false;
        track_info.track = track_no++;
        track_info.title = String::ToString(track.title);

        if (!track.artists.empty()) {
            track_info.artist = String::ToString(track.artists.front().name);
        }
        else {
            track_info.artist = qDatabaseFacade.unknownArtist().toStdWString();
        }

        if (track.duration) {
            track_info.duration = parseDuration(track.duration.value());
        }
        else {
            track_info.duration = 0;
        }

        if (track_info.album == qDatabaseFacade.unknownAlbum().toStdWString()) {
            track_info.track = 1;
            is_unknown_album = true;
        }

        if (!track.video_id) {
            continue;
        }

        track_info.sample_rate = kYtMusicSampleRate;
        track_info.file_path = String::ToString(track.video_id.value());

        const ForwardList<TrackInfo> track_infos{ track_info };
        qDatabaseFacade.insertTrackInfo(track_infos, yt_music_search_page_->playlist()->playlistId(), StoreType::CLOUD_SEARCH_STORE, [=, this](auto music_id, auto album_id) {
            if (thumbnail_url.empty()) {
                return;
            }
            DatabaseCoverId id;
            id.first = music_id;
            if (!is_unknown_album) {
                id.second = album_id;
            }
            emit fetchThumbnailUrl(id, QString::fromStdString(thumbnail_url));
            }
        );
    }
    yt_music_search_page_->playlist()->reload();
}

void Xamp::onSearchCompleted(const std::vector<search::SearchResultItem>& result) {
    if (result.empty()) {
        yt_music_search_page_->spinner()->stopAnimation();
        return;
    }

    XAMP_LOG_DEBUG("Search result: {}", result.size());

    yt_music_search_page_->spinner()->stopAnimation();

    for (auto& item : result) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, search::Album>) {
                if (!arg.browse_id) {
                    return;
                }
                QCoro::connect(ytmusic_service_->fetchAlbumAsync(
                    QString::fromStdString(arg.browse_id.value())), this,
                    &Xamp::onFetchAlbumCompleted);
            }
            }, item);
    }
}

void Xamp::onSearchSuggestionsCompleted(const std::vector<std::string>& result) {
    for (const auto &text : result) {
        yt_music_search_page_->addSuggestions(QString::fromStdString(text));
    }
    yt_music_search_page_->showCompleter();
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
}

void Xamp::updateMaximumState(bool is_maximum) {
    lrc_page_->setFullScreen(is_maximum);
    qTheme.updateMaximumIcon(ui_.maxWinButton, is_maximum);
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
    QFont f(qTEXT("DisplayFont"));
    f.setWeight(QFont::Bold);
    f.setPointSize(qTheme.fontSize(8));
    ui_.titleLabel->setFont(f);

    f.setWeight(QFont::Normal);
    f.setPointSize(qTheme.fontSize(8));
    ui_.artistLabel->setFont(f);    

    QToolTip::hideText();
    f.setPointSize(qTheme.fontSize(12));
    QToolTip::setFont(f);

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

    ui_.mutedButton->setAudioPlayer(player_);
    ui_.mutedButton->updateState();
    ui_.coverLabel->setAttribute(Qt::WA_StaticContents);
    qTheme.setPlayOrPauseButton(ui_.playButton, false);

    //ui_.stopButton->hide();    
    //ui_.formatLabel->hide();

    if (YtMusicOAuth::parseOAuthJson()) {
        setAuthButton(ui_, true);
        XAMP_LOG_DEBUG("YouTube worker initial done!");
    }
    else {
        setAuthButton(ui_, false);
    }

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

    desc_label->setObjectName(qTEXT("textSeparator"));

    QFont f = font();
    f.setPointSize(qTheme.fontSize(10));
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

void Xamp::initialDeviceList(const std::string& device_id) {
    XAMP_LOG_DEBUG("Initial device list");

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
        device_id_action[device_info_.value().device_id]->setChecked(true);
        ui_.deviceDescLabel->setMinimumWidth(max_width + 60);
        ui_.deviceDescLabel->setText(QString::fromStdWString(device_info_.value().name));
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

        music_dao_.updateMusicHeart(current_entity_.value().music_id, current_entity_.value().heart);
        qTheme.setHeartButton(ui_.heartButton, current_entity_.value().heart);
        if (last_playlist_page_ != nullptr) {
            last_playlist_page_->setHeart(current_entity_.value().heart);
            last_playlist_page_->playlist()->reload();
        }
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
        constexpr bool use_supereq = true;
        QScopedPointer<XDialog> dialog(new XDialog(this));
        if (use_supereq) {
            QScopedPointer<SuperEqView> eq(new SuperEqView(dialog.get()));
            dialog->setContentWidget(eq.get(), false);
            dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
            dialog->setTitle(tr("SuperEQ"));
            dialog->exec();
        }
        else {
            QScopedPointer<EqualizerView> eq(new EqualizerView(dialog.get()));
            dialog->setContentWidget(eq.get(), false);
            dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
            dialog->setTitle(tr("EQ"));
            dialog->exec();
        }        
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

    (void)QObject::connect(ui_.naviBar, &TabListView::clickedTable, [this](auto table_id) {
        setCurrentTab(table_id);
        qAppSettings.setValue(kAppSettingLastTabName, ui_.naviBar->tabName(table_id));
    });

    ui_.seekSlider->setEnabled(false);
    ui_.startPosLabel->setText(formatDuration(0));
    ui_.endPosLabel->setText(formatDuration(0));

    if (qAppSettings.valueAsBool(kAppSettingHideNaviBar)) {
		ui_.sliderFrame2->setMaximumWidth(50);
    }
    else {
        ui_.sliderFrame2->setMaximumWidth(180);
    }

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
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Xamp::setCurrentTab(int32_t table_id) {
    switch (table_id) {
    case TAB_MUSIC_LIBRARY:
        music_library_page_->reload();
        //ui_.currentView->setCurrentWidget(album_page_.get());        
        break;
    case TAB_FILE_EXPLORER:
        //ui_.currentView->setCurrentWidget(file_system_view_page_.get());
        break;
    case TAB_PLAYLIST:        
        //ui_.currentView->setCurrentWidget(local_tab_widget_.get());
        ensureLocalOnePlaylistPage();
        playlist_tab_page_->reloadAll();
        break;
    case TAB_LYRICS:
        //ui_.currentView->setCurrentWidget(lrc_page_.get());
        break;
    case TAB_CD:
        //ui_.currentView->setCurrentWidget(cd_page_.get());
        break;
    case TAB_YT_MUSIC_SEARCH:
        if (YtMusicOAuth::parseOAuthJson()) {
            initialYtMusicService();            
            yt_music_search_page_->playlist()->reload();
        }        
        //ui_.currentView->setCurrentWidget(cloud_search_page_.get());
        break;
    case TAB_YT_MUSIC_PLAYLIST:
        if (YtMusicOAuth::parseOAuthJson()) {
            initialYtMusicService();
            if (!yt_music_tab_page_->count()) {
                initialCloudPlaylist();
            }
            else {
                yt_music_tab_page_->setCurrentIndex(0);
            }
        }        
        //ui_.currentView->setCurrentWidget(cloud_tab_widget_.get());
        yt_music_tab_page_->reloadAll();
        break;
    }
    ui_.currentView->setCurrentWidget(widgets_[table_id]);
}

void Xamp::onThemeChangedFinished(ThemeColor theme_color) {
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

    for (auto i = 0; i < playlist_tab_page_->tabBar()->count(); ++i) {
        auto* page = dynamic_cast<PlaylistPage*>(playlist_tab_page_->widget(i));
        if (!page) {
            continue;
        }
        setPlaylistPageCover(nullptr, page);
    }

    setPlaylistPageCover(nullptr, cd_page_->playlistPage());

    if (current_entity_) {
        if (current_entity_.value().cover_id == qImageCache.unknownCoverId()) {
            lrc_page_->setCover(qImageCache.getOrAddDefault(qImageCache.unknownCoverId()));
        }
    }
    else {
        lrc_page_->setCover(qImageCache.getOrAddDefault(qImageCache.unknownCoverId()));
    }

    if (YtMusicOAuth::parseOAuthJson()) {
        setAuthButton(ui_, true);
    }
    else {
        setAuthButton(ui_, false);
    }

    setCover(qImageCache.unknownCoverId());
}

void Xamp::setThemeColor(QColor background_color, QColor color) {
    qTheme.setBackgroundColor(background_color);
    setWidgetStyle(ui_);
    updateButtonState(ui_.playButton, player_->GetState());
    emit themeColorChanged(background_color, color);
}

void Xamp::onSearchArtistCompleted(const QString& artist, const QByteArray& image) {
    QPixmap cover;
    if (cover.loadFromData(image)) {        
        artist_dao_.updateArtistCoverId(artist_dao_.addOrUpdateArtist(artist), qImageCache.addImage(cover));
    }
}

void Xamp::onSearchLyricsCompleted(int32_t music_id, const QString& lyrics, const QString& trlyrics) {
    lrc_page_->lyrics()->onSetLrc(lyrics, trlyrics);
    music_dao_.addOrUpdateLyrics(music_id, lyrics, trlyrics);
}

void Xamp::setFullScreen() {
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
    
    playlist_tab_page_->forEachPlaylist([](auto* playlist) {
        playlist->setNowPlayState(PlayingState::PLAY_CLEAR);
        });
    playlist_tab_page_->resetAllTabIcon();

    yt_music_tab_page_->forEachPlaylist([](auto* playlist) {
        playlist->setNowPlayState(PlayingState::PLAY_CLEAR);
        });
    yt_music_tab_page_->resetAllTabIcon();

    music_library_page_->album()->albumViewPage()->playlistPage()->playlist()->setNowPlayState(PlayingState::PLAY_CLEAR);
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
        lrc_page_->lyrics()->onSetLrcTime(stream_time_as_ms);
    }
}

void Xamp::playLocalFile(const PlayListEntity& entity) {
    main_window_->setTaskbarPlayerPlaying();
    onPlayMusic(kInvalidDatabaseId, entity, false, false);
}

void Xamp::playOrPause() {
    XAMP_LOG_DEBUG("Player state:{}", player_->GetState());

    const auto tab_index = ui_.currentView->currentIndex();
    PlaylistPage* page = nullptr;
    PlaylistTabWidget* tab = nullptr;

    if (!last_playlist_tab_) {
        tab = playlist_tab_page_.get();
        page = localPlaylistPage();
    }
    else {
        tab = last_playlist_tab_;
        page = last_playlist_page_;
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
        else if (player_->GetState() == PlayerState::PLAYER_STATE_STOPPED || player_->GetState() == PlayerState::PLAYER_STATE_USER_STOPPED) {
            if (!ui_.currentView->count()) {
                return;
            }
            if (const auto select_item = page->playlist()->selectFirstItem()) {
                play_index_ = select_item.value();
            }
            play_index_ = page->playlist()->model()->index(
                play_index_.row(), PLAYLIST_IS_PLAYING);
            if (play_index_.row() == -1) {
                // Don't not show message box. when the application will receive other signal.
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
    // EQ
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

void Xamp::onFetchThumbnailUrlError(const DatabaseCoverId& id, const QString& thumbnail_url) {    
	download_thumbnail_pending_.insert(id, thumbnail_url);
}

void Xamp::onSetThumbnail(const DatabaseCoverId& id, const QString& cover_id) {
    if (id.isAlbumIdValid()) {
        album_dao_.setAlbumCover(id.get(), cover_id);
    }
    else {
        music_dao_.setMusicCover(id.get(), cover_id);
    }
}

void Xamp::onDelayedDownloadThumbnail() {
    constexpr auto kBatchesSize = 10;
	
    for (auto i = 0; !download_thumbnail_pending_.isEmpty() && i < kBatchesSize; ++i) {
	    const auto itr = download_thumbnail_pending_.keyValueBegin();
        const auto id = itr->first;
        const auto thumbnail_url = itr->second;
        emit fetchThumbnailUrl(id, thumbnail_url);
        download_thumbnail_pending_.remove(id);
    }
}

void Xamp::onPlayEntity(const PlayListEntity& entity, bool is_doubleclicked) {
    if (!device_info_) {
        showMeMessage(tr("Not found any audio device, please check your audio device."));
        return;
    }

    lrc_page_->spectrum()->reset();

    PlaybackFormat playback_format;
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

    setupSampleRateConverter(sample_rate_converter_factory,
        target_sample_rate,
        sample_rate_converter_type);

    if (player_->GetAudioDeviceManager()->IsSharedDevice(device_info_.value().device_type_id)) {
        AudioFormat default_format;
        if (device_info_.value().default_format) {
            default_format = device_info_.value().default_format.value();
        }
        else {
            default_format = AudioFormat::k16BitPCM441Khz;
        }

        auto sample_rate = entity.isFilePath() ? entity.sample_rate : AudioFormat::k16BitPCM48Khz.GetSampleRate();

        if (sample_rate != default_format.GetSampleRate()) {
            if (device_info_.value().connect_type == DeviceConnectType::BLUE_TOOTH) {
                const auto message =
                    tr("Playing blue-tooth device need set %1 to %2.")
                    .arg(formatSampleRate(sample_rate))
                    .arg(formatSampleRate(default_format.GetSampleRate()));
                showMeMessage(message);
            } else {
                const auto message =
                    tr("Playing Shared WASAPI device need set %1 to %2.")
                    .arg(formatSampleRate(sample_rate))
                    .arg(formatSampleRate(default_format.GetSampleRate()));
                showMeMessage(message);
            }
            player_->GetDspManager()->RemoveSampleRateConverter();
            sample_rate_converter_type = kSoxr;
            target_sample_rate = default_format.GetSampleRate();
            player_->GetDspManager()->AddPreDSP(makeSampleRateConverter(target_sample_rate));
        }
        byte_format = ByteFormat::SINT16;
    }
    
    const auto open_dsd_mode = getDsdModes(device_info_.value(),
        entity.file_path.toStdWString(),
        entity.sample_rate,
        target_sample_rate);

    auto* page = dynamic_cast<PlaylistPage*>(sender());

    try {
        player_->Open(entity.file_path.toStdWString(),
            device_info_.value(),
            target_sample_rate,
            open_dsd_mode);

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

        auto offset = !entity.offset ? 0.0 : entity.offset.value();
        player_->BufferStream(offset);

        ui_.mutedButton->updateState();
        open_done = true;
        updateUi(entity, playback_format, open_done, is_doubleclicked);
        return;
    }
    catch (...) {
        open_done = false;
        player_->Stop(false, true);
        logAndShowMessage(std::current_exception());
    }
    
    if (last_playlist_page_ != nullptr) {
        last_playlist_page_->playlist()->reload();
    }   
}

void Xamp::ensureLocalOnePlaylistPage() {
	if (playlist_tab_page_->count() == 0) {
		const auto playlist_id = playlist_dao_.addPlaylist(tr("Playlist"), 0, StoreType::PLAYLIST_LOCAL_STORE);
		newPlaylistPage(playlist_tab_page_.get(), playlist_id, kEmptyString, tr("Playlist"));
	}
}

void Xamp::updateUi(const PlayListEntity& entity, const PlaybackFormat& playback_format, bool open_done, bool is_doubleclicked) {
    qTheme.setPlayOrPauseButton(ui_.playButton, open_done);

    const int64_t max_duration_ms = Round(entity.duration) * 1000;
    ui_.seekSlider->setRange(0, max_duration_ms - 1000);
    ui_.seekSlider->setValue(0);

    ensureLocalOnePlaylistPage();

    const auto local_music = entity.isFilePath();

    auto* sender_playlist_page = qobject_cast<PlaylistPage*>(sender());   
    
	PlaylistTabWidget* playlist_tab_widget = nullptr;
	PlaylistPage* playlist_page = nullptr;  

    if (!sender_playlist_page) {
        sender_playlist_page = qobject_cast<PlaylistPage*>(yt_music_tab_page_->currentWidget());
    }

	// Switch playlist other page
    if (is_doubleclicked) {
        playlist_page = sender_playlist_page;
        playlist_tab_widget = qobject_cast<PlaylistTabWidget*>(playlist_page->parent()->parent());  
		// It's an signal from album page.
        if (playlist_tab_widget != nullptr) {
            playlist_tab_widget->setCurrentNowPlaying();
        }        
    }
    else {
        if (!last_playlist_tab_) {
            playlist_tab_widget = playlist_tab_page_.get();
            playlist_page = localPlaylistPage();            
		}
		else {
			playlist_tab_widget = last_playlist_tab_;
			playlist_page = last_playlist_page_;
		}
        playlist_tab_widget->setNowPlaying(playlist_page->playlist()->playlistId());
    }        

    onSetCover(entity.validCoverId(), playlist_page);

    last_playlist_tab_ = playlist_tab_widget;
    last_playlist_page_ = playlist_page;
    last_playlist_ = playlist_page->playlist();
    playlist_page->format()->setText(format2String(playback_format, entity.file_extension));
    playlist_page->title()->setText(entity.title);
    playlist_page->setHeart(entity.heart);    

    last_playlist_->removePlaying();
    playlist_page->playlist()->setNowPlayState(PlayingState::PLAY_PLAYING);
    playlist_page->playlist()->reload(false);

    music_library_page_->album()->setPlayingAlbumId(entity.album_id);
    updateButtonState(ui_.playButton, player_->GetState());

    const QFontMetrics title_metrics(ui_.titleLabel->font());
    const QFontMetrics artist_metrics(ui_.artistLabel->font());
    
    ui_.titleLabel->setText(title_metrics.elidedText(entity.title, Qt::ElideRight, ui_.titleLabel->width()));
    ui_.artistLabel->setText(artist_metrics.elidedText(entity.artist, Qt::ElideRight, ui_.artistLabel->width()));
 
    if (local_music) {
        lrc_page_->lyrics()->loadLrcFile(entity.file_path);
    } else {
        lrc_page_->lyrics()->loadLrcFile(kEmptyString);
    }

    lrc_page_->title()->setText(entity.title);
    lrc_page_->album()->setText(entity.album);
    lrc_page_->artist()->setText(entity.artist);
    lrc_page_->format()->setText(format2String(playback_format, entity.file_extension));
    lrc_page_->spectrum()->setFftSize(state_adapter_->fftSize());
    lrc_page_->spectrum()->setSampleRate(playback_format.output_format.GetSampleRate());

    if (!local_music) {
        const auto lyrics_opt = music_dao_.getLyrics(entity.music_id);
        if (!lyrics_opt) {
            emit searchLyrics(entity.music_id, entity.title, entity.artist);
        }
        else {
            onSearchLyricsCompleted(entity.music_id, std::get<0>(lyrics_opt.value()), std::get<1>(lyrics_opt.value()));
        }
    }    

    qTheme.setHeartButton(ui_.heartButton, current_entity_.value().heart);

    if (!entity.isHttpUrl()) {
        emit findAlbumCover(DatabaseCoverId(entity.music_id, entity.album_id));
    }

    music_dao_.updateMusicPlays(entity.music_id);
    album_dao_.updateAlbumPlays(entity.album_id);

    player_->Play();

    if (open_done) {
        setCover(entity.validCoverId());
    }

    update();
}

void Xamp::setCover(const QString &cover_id) {
    const auto cover = qImageCache.getOrAddDefault(cover_id, true);
    const QSize cover_size(ui_.coverLabel->size().width() - image_util::kPlaylistImageRadius,
        ui_.coverLabel->size().height() - image_util::kPlaylistImageRadius);
    const auto ui_cover = image_util::roundImage(
        image_util::resizeImage(cover, cover_size, false),
        image_util::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);
}

void Xamp::onUpdateMbDiscInfo(const MbDiscIdInfo& mb_disc_id_info) {
    const auto disc_id = QString::fromStdString(mb_disc_id_info.disc_id);
    const auto album = QString::fromStdWString(mb_disc_id_info.album);
    const auto artist = QString::fromStdWString(mb_disc_id_info.artist);
    
    if (!album.isEmpty()) {
        album_dao_.updateAlbumByDiscId(disc_id, album);
    }
    if (!artist.isEmpty()) {
        artist_dao_.updateArtistByDiscId(disc_id, artist);
    }

	const auto album_id = album_dao_.getAlbumIdByDiscId(disc_id);

    if (!mb_disc_id_info.tracks.empty()) {
        QList<PlayListEntity> entities;
        album_dao_.forEachAlbumMusic(album_id, [&entities](const auto& entity) {
            entities.append(entity);
            });
        std::sort(entities.begin(), entities.end(), [](const auto& a, const auto& b) {
            return b.track > a.track;
            });
        auto i = 0;
        Q_FOREACH(const auto track, mb_disc_id_info.tracks) {
            music_dao_.updateMusicTitle(entities[i++].music_id, QString::fromStdWString(track.title));
        }
    }    

    if (!album.isEmpty()) {
        cd_page_->playlistPage()->title()->setText(album);
        cd_page_->playlistPage()->playlist()->reload();
    }

    if (const auto album_stats = album_dao_.getAlbumStats(album_id)) {
        cd_page_->playlistPage()->format()->setText(tr("%1 Songs, %2, %3")
            .arg(QString::number(album_stats.value().songs))
            .arg(formatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year)));
    }
}

void Xamp::onUpdateDiscCover(const QString& disc_id, const QString& cover_id) {
	const auto album_id = album_dao_.getAlbumIdByDiscId(disc_id);
    album_dao_.setAlbumCover(album_id, cover_id);
}

void Xamp::onUpdateCdTrackInfo(const QString& disc_id, const ForwardList<TrackInfo>& track_infos) {
    const auto album_id = album_dao_.getAlbumIdByDiscId(disc_id);
    album_dao_.removeAlbum(album_id);
    album_dao_.removeAlbumArtist(album_id);
    cd_page_->playlistPage()->playlist()->removeAll();
    qDatabaseFacade.insertTrackInfo(track_infos, kCdPlaylistId, StoreType::LOCAL_STORE);
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
    const auto cover = qImageCache.getOrAddDefault(cover_id, true);

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
        lrc_page_->setCover(image_util::resizeImage(cover, lrc_page_->cover()->size(), true));
        lrc_page_->addCoverShadow(found_cover);        
    }

    main_window_->setIconicThumbnail(cover);
}

void Xamp::onPlayMusic(int32_t playlist_id, const PlayListEntity& entity, bool is_play, bool is_doubleclicked) {
    initialYtMusicService();

    main_window_->setTaskbarPlayerPlaying();
    current_entity_ = entity;

    if (entity.isHttpUrl()) {
        onPlayEntity(entity, is_doubleclicked);
        return;
    }

    if (entity.isFilePath()) {
        onPlayEntity(entity, is_doubleclicked);
    }
    else {
        playCloudVideoId(entity, entity.file_path, is_doubleclicked);
    }
}

void Xamp::playNextItem(int32_t forward, bool is_play) {
    if (!last_playlist_) {
        return;
    }
    const auto count = last_playlist_->model()->rowCount();
    if (count == 0) {
        stopPlay();
		// Don't not show message box. when the application will receive other signal.
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
    ui_.currentView->setCurrentWidget(music_library_page_.get());
}

void Xamp::onAddPlaylist(int32_t playlist_id, const QList<int32_t>& music_ids) {
    ensureLocalOnePlaylistPage();

    if (playlist_id == kInvalidDatabaseId) {
        const auto tab_index = playlist_tab_page_->count();
        const auto playlist_id = playlist_dao_.addPlaylist(tr("Playlist"), tab_index, StoreType::PLAYLIST_LOCAL_STORE);
        newPlaylistPage(playlist_tab_page_.get(), playlist_id, kEmptyString, tr("Playlist"));
        playlist_dao_.addMusicToPlaylist(music_ids, playlist_id);
	}
    else {
        playlist_dao_.addMusicToPlaylist(music_ids, playlist_id);
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

PlaylistPage* Xamp::newPlaylistPage(PlaylistTabWidget *tab_widget, int32_t playlist_id, const QString& cloud_playlist_id, const QString& name) {
    auto* playlist_page = createPlaylistPage(tab_widget, playlist_id, kAppSettingPlaylistColumnName, cloud_playlist_id);
    playlist_page->playlist()->setHeaderViewHidden(false);
    playlist_page->pageTitle()->hide();
    connectPlaylistPageSignal(playlist_page);
    onSetCover(kEmptyString, playlist_page);
    tab_widget->createNewTab(name, playlist_page);
    return playlist_page;
}

void Xamp::initialPlaylist() {
    lrc_page_.reset(new LrcPage(this));
    music_library_page_.reset(new AlbumArtistPage(this));
    playlist_tab_page_.reset(new PlaylistTabWidget(this));    
    yt_music_search_page_.reset(new PlaylistPage(this));
    yt_music_tab_page_.reset(new PlaylistTabWidget(this));
    yt_music_tab_page_->setStoreType(StoreType::CLOUD_STORE);
    yt_music_tab_page_->hidePlusButton();

    yt_music_search_page_->pageTitle()->hide();

    ui_.naviBar->addTab(translateText("Playlists"),        TAB_PLAYLIST,          qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
    ui_.naviBar->addTab(translateText("File explorer"),    TAB_FILE_EXPLORER,     qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_.naviBar->addTab(translateText("Lyrics"),           TAB_LYRICS,            qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
    ui_.naviBar->addTab(translateText("Library"),          TAB_MUSIC_LIBRARY,     qTheme.fontIcon(Glyphs::ICON_MUSIC_LIBRARY));
    ui_.naviBar->addTab(translateText("CD"),               TAB_CD,                qTheme.fontIcon(Glyphs::ICON_CD));
    ui_.naviBar->addTab(translateText("YouTube search"),   TAB_YT_MUSIC_SEARCH,   qTheme.fontIcon(Glyphs::ICON_YOUTUBE));
    ui_.naviBar->addTab(translateText("YouTube playlist"), TAB_YT_MUSIC_PLAYLIST, qTheme.fontIcon(Glyphs::ICON_YOUTUBE_PLAYLIST));    
        
    playlist_dao_.forEachPlaylist([this](auto playlist_id,
        auto index,
        auto store_type,
        const auto& cloud_playlist_id,
        const auto& name) {
        if (playlist_id == kAlbumPlaylistId || playlist_id == kCdPlaylistId) {
            return;
        }
        if (store_type == StoreType::LOCAL_STORE || store_type == StoreType::PLAYLIST_LOCAL_STORE) {
            auto* playlist_page = newPlaylistPage(playlist_tab_page_.get(), playlist_id, kEmptyString, name);
            playlist_page->playlist()->enableCloudMode(false);
            playlist_page->pageTitle()->hide();
        }
        else if (store_type == StoreType::CLOUD_STORE || playlist_id == kYtMusicSearchPlaylistId) {
            PlaylistPage* playlist_page;
            if (playlist_id != kYtMusicSearchPlaylistId) {
                playlist_page = newPlaylistPage(yt_music_tab_page_.get(), playlist_id, cloud_playlist_id, name);
            } else {
                playlist_page = yt_music_search_page_.get();
            }
            playlist_page->pageTitle()->hide();
            playlist_page->hidePlaybackInformation(true);
            playlist_page->playlist()->setPlayListGroup(PLAYLIST_GROUP_ALBUM);
            playlist_page->playlist()->enableCloudMode(true);
            playlist_page->playlist()->reload();
        }
    });    

    ensureLocalOnePlaylistPage();

    if (!playlist_dao_.isPlaylistExist(kAlbumPlaylistId)) {
        playlist_dao_.addPlaylist(tr("Album Playlist"), 0, StoreType::LOCAL_STORE);
    }
    if (!playlist_dao_.isPlaylistExist(kCdPlaylistId)) {
        playlist_dao_.addPlaylist(tr("CD Playlist"), 0, StoreType::LOCAL_STORE);
    }
    if (!playlist_dao_.isPlaylistExist(kYtMusicSearchPlaylistId)) {
        playlist_dao_.addPlaylist(tr("Yt Music Search Playlist"), 0, StoreType::CLOUD_STORE);
    }

    playlist_tab_page_->restoreTabOrder();
    playlist_tab_page_->setCurrentTabIndex(qAppSettings.valueAsInt(kAppSettingLastPlaylistTabIndex));

    (void)QObject::connect(yt_music_tab_page_.get(), &PlaylistTabWidget::reloadAllPlaylist,[this]() {
        initialCloudPlaylist();
    });

    (void)QObject::connect(yt_music_tab_page_.get(), &PlaylistTabWidget::reloadPlaylist, [this](auto tab_index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(yt_music_tab_page_->widget(tab_index));
        playlist_page->playlist()->removeAll();
        const auto playlist_id = playlist_page->playlist()->cloudPlaylistId().value();
        QCoro::connect(ytmusic_service_->fetchPlaylistAsync(playlist_id), this, [this, playlist_page](const auto& playlist) {
            XAMP_LOG_DEBUG("Get playlist done!");
            onFetchPlaylistTrackCompleted(playlist_page, playlist.tracks);
        });
    });

    (void)QObject::connect(dynamic_cast<PlaylistTabBar*>(yt_music_tab_page_->tabBar()), &PlaylistTabBar::textChanged,[this](auto index, const auto& text) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(yt_music_tab_page_->widget(index));
        auto playlist_id = playlist_page->playlist()->cloudPlaylistId().value();
        QCoro::connect(ytmusic_service_->editPlaylistAsync(playlist_id, text, kEmptyString, PrivateStatus::PRIVATE), this, [this](auto) {
            initialCloudPlaylist();
        });
    });

    (void)QObject::connect(yt_music_tab_page_.get(), &PlaylistTabWidget::deletePlaylist, [this](const auto &playlist_id) {
        QCoro::connect(ytmusic_service_->deletePlaylistAsync(playlist_id), this, [this](auto) {
            initialCloudPlaylist();
        });
    });

    (void)QObject::connect(yt_music_tab_page_.get(), &PlaylistTabWidget::createCloudPlaylist, [this]() {
        QScopedPointer<XDialog> dialog(new XDialog(this));
        const QScopedPointer<CreatePlaylistView> create_playlist_view(new CreatePlaylistView(dialog.get()));
        dialog->setContentWidget(create_playlist_view.get());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EDIT));
        dialog->setTitle(tr("Create playlist"));

        if (dialog->exec() != QDialog::Accepted) {
            return;
        }

        if (create_playlist_view->title().isEmpty()) {
            return;
        }

        const auto private_status = static_cast<PrivateStatus>(create_playlist_view->privateStatus());

        QCoro::connect(ytmusic_service_->createPlaylistAsync(create_playlist_view->title(),
            create_playlist_view->desc(),
            private_status, {}), this, [this](auto) {                
            initialCloudPlaylist();
        });
    });

    (void)QObject::connect(playlist_tab_page_.get(), &PlaylistTabWidget::createNewPlaylist,
        [this]() {
            const auto tab_index = playlist_tab_page_->count();
            const auto playlist_id = playlist_dao_.addPlaylist(tr("Playlist"), tab_index, StoreType::PLAYLIST_LOCAL_STORE);
            newPlaylistPage(playlist_tab_page_.get(), playlist_id, kEmptyString, tr("Playlist"));
        });

    (void)QObject::connect(playlist_tab_page_.get(), &PlaylistTabWidget::removeAllPlaylist,
        [this]() {
            last_playlist_ = nullptr;
        });    

    file_explorer_page_.reset(new FileSystemViewPage(this));

    cd_page_.reset(new CdPage(this));
    cd_page_->playlistPage()->playlist()->setPlaylistId(kCdPlaylistId, kAppSettingCdPlaylistColumnName);
    cd_page_->playlistPage()->playlist()->setHeaderViewHidden(false);
    cd_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    onSetCover(kEmptyString, cd_page_->playlistPage());
    connectPlaylistPageSignal(cd_page_->playlistPage());

    yt_music_search_page_->playlist()->setPlaylistId(kYtMusicSearchPlaylistId, kAppSettingPlaylistColumnName);
    onSetCover(kEmptyString, yt_music_search_page_.get());
    connectPlaylistPageSignal(yt_music_search_page_.get());

    setCover(kEmptyString);

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
        &Xamp::onUpdateMbDiscInfo,
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

    (void)QObject::connect(this,
        &Xamp::themeColorChanged,
        music_library_page_.get(),
        &AlbumArtistPage::onThemeColorChanged);

    (void)QObject::connect(music_library_page_->album(),
        &AlbumView::removeAll,        
        [this]() {
            yt_music_tab_page_->closeAllTab();
        });

    (void)QObject::connect(this,
        &Xamp::searchLyrics,
        background_service_.get(),
        &BackgroundService::onSearchLyrics);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::fetchLyricsCompleted,
        this,
        &Xamp::onSearchLyricsCompleted);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::fetchArtistCompleted,
        this,
        &Xamp::onSearchArtistCompleted);

    (void)QObject::connect(music_library_page_->album(),
        &AlbumView::clickedArtist,
        this,
        &Xamp::onArtistIdChanged);

    (void)QObject::connect(this,
        &Xamp::themeColorChanged,
        lrc_page_.get(),
        &LrcPage::onThemeColorChanged);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::blurImage,
        lrc_page_.get(),
        &LrcPage::setBackground);

    (void)QObject::connect(background_service_.get(),
        &BackgroundService::dominantColor,
        lrc_page_->lyrics(),
        &LyricsShowWidget::onSetLrcColor);

    (void)QObject::connect(music_library_page_->album(),
        &AlbumView::addPlaylist,
        this,
        &Xamp::onAddPlaylist);

    pushWidget(playlist_tab_page_.get());
    pushWidget(file_explorer_page_.get());
    pushWidget(lrc_page_.get());
    pushWidget(music_library_page_.get());
    pushWidget(cd_page_.get());
    pushWidget(yt_music_search_page_.get());
    pushWidget(yt_music_tab_page_.get());

    connectPlaylistPageSignal(music_library_page_->album()->albumViewPage()->playlistPage());
    connectPlaylistPageSignal(music_library_page_->year()->albumViewPage()->playlistPage());

    ui_.currentView->setCurrentIndex(0);
}

void Xamp::initialCloudPlaylist() {
    yt_music_tab_page_->closeAllTab();

    const auto process_dialog = makeProgressDialog(
        tr("Initial cloud playlist"),
        tr(""),
        tr("Cancel"));
    process_dialog->show();
    cloud_playlist_process_count_ = 0;

    QCoro::connect(ytmusic_service_->fetchLibraryPlaylistAsync(), this, [process_dialog, this](const auto& playlists) {
        static const std::string kEpisodesForLaterName("Episodes for Later");
        int32_t index = 1;
        for (const auto& playlist : playlists) {
            const auto playlist_id = playlist_dao_.addPlaylist(QString::fromStdString(playlist.title), index++, StoreType::CLOUD_STORE, QString::fromStdString(playlist.playlistId));
            if (playlist.title == kEpisodesForLaterName) {
                continue;
            }
            auto* playlist_page = newPlaylistPage(yt_music_tab_page_.get(), playlist_id, QString::fromStdString(playlist.playlistId), QString::fromStdString(playlist.title));
            playlist_page->pageTitle()->hide();
            playlist_page->hidePlaybackInformation(true);
            playlist_page->playlist()->setPlayListGroup(PLAYLIST_GROUP_ALBUM);
            playlist_page->playlist()->setCloudPlaylistId(QString::fromStdString(playlist.playlistId));
            playlist_page->playlist()->enableCloudMode(true);
            playlist_page->playlist()->reload();
            QCoro::connect(ytmusic_service_->fetchPlaylistAsync(QString::fromStdString(playlist.playlistId)),
                this, [this, playlists, playlist_page, process_dialog](const auto& playlist) {
                    process_dialog->setValue(cloud_playlist_process_count_++ * 100 / playlists.size() + 1);
                    onFetchPlaylistTrackCompleted(playlist_page, playlist.tracks);
                });
        }
        yt_music_tab_page_->setCurrentIndex(0);
        });
}

void Xamp::appendToPlaylist(const QString& file_name, bool append_to_playlist) {
    auto* playlist_page = localPlaylistPage();
    if (!playlist_page) {
        return;
    }

    if (!append_to_playlist) {
        emit extractFile(file_name, playlist_page->playlist()->playlistId(), false);
        music_library_page_->reload();
        return;
    }

    XAMP_TRY_LOG(playlist_page->playlist()->append(file_name));
}

void Xamp::addItem(const QString& file_name) {
    ensureLocalOnePlaylistPage();
    appendToPlaylist(file_name, true);
}

void Xamp::pushWidget(QWidget* widget) {
    /*auto* scroll_area = new QScrollArea();
    scroll_area->verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));
    scroll_area->setWidget(widget);
    scroll_area->setWidgetResizable(true);
    const auto id = ui_.currentView->addWidget(scroll_area);
    if (id == TAB_MUSIC_LIBRARY - 1) {
		widget->setMinimumHeight(6000);
        scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    }
    widgets_.push_back(scroll_area);*/
    const auto id = ui_.currentView->addWidget(widget);
    widgets_.push_back(widget);
    ui_.currentView->setCurrentIndex(id);
}

void Xamp::encodeFile(const PlayListEntity& entity,
    const EncodingProfile& profile, 
    const QString& file_filter,
    const QString& file_type, 
    const std::wstring &command,
    AlignPtr<IFileEncoder> encoder) {
    const auto last_dir = qAppSettings.valueAsString(kAppSettingLastOpenFolderPath);
    const auto save_file_name = last_dir + qTEXT("/") + entity.album + qTEXT("-") + entity.title;

    getSaveFileName(this,
        [this, entity, profile, command, file_type, &encoder](const auto& file_name) {
            const auto dialog = makeProgressDialog(
                tr("Export progress dialog"),
                tr("Export '") + entity.title + tr("' to ") + file_type + tr(" file"),
                tr("Cancel"));
            dialog->show();

            TrackInfo track_info;
            track_info.album = entity.album.toStdWString();
            track_info.artist = entity.artist.toStdWString();
            track_info.title = entity.title.toStdWString();
            track_info.track = entity.track;

            AnyMap config;
            config.AddOrReplace(FileEncoderConfig::kInputFilePath, Path(entity.file_path.toStdWString()));
            config.AddOrReplace(FileEncoderConfig::kOutputFilePath, Path(toNativeSeparators(file_name).toStdWString()));
            config.AddOrReplace(FileEncoderConfig::kEncodingProfile, profile);
            config.AddOrReplace(FileEncoderConfig::kCommand, command);

            try {
                read_util::encodeFile(config,
                    encoder,
                    [&](auto progress) -> bool {
                        dialog->setValue(progress);
                        qApp->processEvents();
                        delay(1);
                        return dialog->wasCanceled() != true;
                    }, track_info);
            }
            catch (...) {
                dialog->hide();
                logAndShowMessage(std::current_exception());
            }
        },
        tr("Save ") + file_type,
        save_file_name,
        file_filter);
}

void Xamp::encodeAacFile(const PlayListEntity& entity, const EncodingProfile& profile) {
    encodeFile(entity,
        profile, 
        tr("AAC Files (*.m4a)"),
        tr("m4a"),
        L"",
        StreamFactory::MakeAACEncoder());
}

void Xamp::encodeWavFile(const PlayListEntity& entity) {
    /*encodeFile(entity,
        EncodingProfile(),
        tr("Wav Files (*.wav)"),
        tr("wav"),
        L"", 
        StreamFactory::MakeWaveEncoder());*/
    encodeFile(entity,
        EncodingProfile(),
        tr("ALAC Files (*.m4a)"),
        tr("m4a"),
        L"",
        StreamFactory::MakeAlacEncoder());
}

void Xamp::encodeFlacFile(const PlayListEntity& entity) {
    const auto command
        = qSTR("-%1 -V").arg(qAppSettings.valueAs(kFlacEncodingLevel).toInt()).toStdWString();

    encodeFile(entity,
        EncodingProfile(),
        tr("FLAC Files (*.flac)"),
        tr("flac"),
        command,
        StreamFactory::MakeFlacEncoder());
}

void Xamp::connectPlaylistPageSignal(PlaylistPage* playlist_page) {
    if (playlist_page != yt_music_search_page_.get()) {
        (void)QObject::connect(playlist_page,
            &PlaylistPage::search,
            [this, playlist_page](const auto& text, Match match) {
                playlist_page->playlist()->search(text);
            });
    } else {
        (void)QObject::connect(playlist_page,
            &PlaylistPage::editFinished,
            [this, playlist_page](const auto& text) {
                if (text.isEmpty()) {
                    playlist_page->spinner()->stopAnimation();
                    return;
                }
                QCoro::connect(ytmusic_service_->searchAsync(text, "albums"), this, &Xamp::onSearchCompleted);
            });

        (void)QObject::connect(playlist_page,
            &PlaylistPage::search,
            [this, playlist_page](const auto& text, Match match) {
                if (text.isEmpty()) {
                    playlist_page->spinner()->stopAnimation();
                    return;
                }
                if (match == Match::MATCH_SUGGEST) {
                    QCoro::connect(ytmusic_service_->searchSuggestionsAsync(text), this, &Xamp::onSearchSuggestionsCompleted);
                    return;
                }
                if (playlist_page->spinner()->isAnimated()) {
                    return;
                }
                if (playlist_page->spinner()->isHidden()) {
                    playlist_page->spinner()->show();
                    centerParent(playlist_page->spinner());
                }
                playlist_page->spinner()->startAnimation();
                playlist_page->playlist()->removeAll();
            });
    }

    if (playlist_page->playlist()->isEnableCloudMode()) {
        (void)QObject::connect(playlist_page->playlist(),
            &PlaylistTableView::addToPlaylist,
            [this](const auto& source_playlist_id, const auto& playlist_id, const auto& video_ids) {
                std::vector<std::string> parsed_video_ids;
                for (auto video_id : video_ids) {
                    auto [parsed_video_id, set_video_id] = parseYtMusicPath(QString::fromStdString(video_id));
                    parsed_video_ids.push_back(parsed_video_id.toStdString());
                }
                QCoro::connect(ytmusic_service_->addPlaylistItemsAsync(playlist_id,
                    parsed_video_ids,
                    source_playlist_id.toStdString()), this, [this](auto) {
                        initialCloudPlaylist();
                    });
            });

        (void)QObject::connect(playlist_page->playlist(),
            &PlaylistTableView::likeSong,
            [this, playlist_page](auto like, const auto& entity) {                
                playlist_page->playlist()->removeAll();
                album_dao_.removeAlbumMusic(entity.album_id, entity.music_id);
                music_dao_.removeMusic(entity.music_id);
                auto [video_id, set_video_id] = parseYtMusicPath(entity.file_path);
                QCoro::connect(ytmusic_service_->rateSongAsync(video_id, like ? SongRating::DISLIKE : SongRating::LIKE),
                    this, [this, playlist_page](auto) {
                    initialCloudPlaylist();
                    });

            });

        (void)QObject::connect(playlist_page->playlist(),
            &PlaylistTableView::removePlaylistItems,
            [this, playlist_page](const auto& playlist_id, const auto& video_ids) {
                std::vector<edit::PlaylistEditResultData> result_data;
                result_data.reserve(video_ids.size());
                for (const auto &id : video_ids) {
                    edit::PlaylistEditResultData data;
                    auto [video_id, setVideoId] = parseYtMusicPath(QString::fromStdString(id));
                    if (video_id.isEmpty() || setVideoId.isEmpty()) {
                        continue;
                    }
                    data.videoId = video_id.toStdString();
                    data.setVideoId = setVideoId.toStdString();
                    result_data.push_back(data);
                }
                if (result_data.empty()) {
                    return;
                }
                playlist_page->spinner()->startAnimation();
                centerParent(playlist_page->spinner());
                QCoro::connect(ytmusic_service_->removePlaylistItemsAsync(playlist_id, result_data), this, [this, playlist_page, playlist_id](auto) {
                    playlist_page->playlist()->removeAll();
                    QCoro::connect(ytmusic_service_->fetchPlaylistAsync(playlist_id),
                        this, [this, playlist_page](const auto& playlist) {
                            XAMP_LOG_DEBUG("Reload playlist!");
                            onFetchPlaylistTrackCompleted(playlist_page, playlist.tracks);
                        });
                    });
            });

    }

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::addPlaylistItemFinished,
        music_library_page_.get(),
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
        &PlaylistTableView::encodeFlacFile,
        this,
        &Xamp::encodeFlacFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::encodeAacFile,
        this,
        &Xamp::encodeAacFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::encodeWavFile,
        this,
        &Xamp::encodeWavFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::downloadFile,
        this,
        &Xamp::cacheYtMusicFile);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::readReplayGain,
        background_service_.get(),
        &BackgroundService::onReadReplayGain);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::editTags,
        this,
        &Xamp::onEditTags);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::extractFile,
        file_system_service_.get(),
        &FileSystemService::onExtractFile);
    
    (void)QObject::connect(background_service_.get(),
        &BackgroundService::readReplayGain,
        playlist_page->playlist(),
        &PlaylistTableView::onUpdateReplayGain,
        Qt::QueuedConnection);    

    (void)QObject::connect(this,
        &Xamp::themeColorChanged,
        playlist_page->playlist(),
        &PlaylistTableView::onThemeColorChanged);

    (void)QObject::connect(&qTheme,
        &ThemeManager::themeChangedFinished,
        playlist_page,
        &PlaylistPage::onThemeChangedFinished);

    (void)QObject::connect(playlist_page->playlist(),
        &PlaylistTableView::addPlaylist,
        this,
        [this](const auto& playlist_id, const auto& entities) {
            if (auto* playlist_page = playlist_tab_page_->findPlaylistPage(playlist_id)) {
                for (const auto& entity : entities) {
                    playlist_dao_.addMusicToPlaylist(entity.music_id, playlist_id, entity.album_id);
                }
                playlist_page->playlist()->reload();
            }            
        });
}

void Xamp::addDropFileItem(const QUrl& url) {
    addItem(url.toLocalFile());
}

PlaylistPage* Xamp::localPlaylistPage() const {
    if (playlist_tab_page_->tabBar()->count() == 0) {
        return nullptr;
    }
    return dynamic_cast<PlaylistPage*>(playlist_tab_page_->currentWidget());
}

void Xamp::onInsertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id) {
    qDatabaseFacade.insertTrackInfo(result, playlist_id, StoreType::PLAYLIST_LOCAL_STORE, [this](auto music_id, auto album_id) {
        emit findAlbumCover(DatabaseCoverId(music_id, album_id));
    });
    ensureLocalOnePlaylistPage();
    localPlaylistPage()->playlist()->reload();
}

void Xamp::onReadFilePath(const QString& file_path) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->setLabelText(file_path);
}

void Xamp::onSetAlbumCover(int32_t album_id, const QString& cover_id) {
    album_dao_.setAlbumCover(album_id, cover_id);
    music_library_page_->album()->refreshCover();
    if (current_entity_) {
        if (current_entity_.value().album_id == album_id) {
            current_entity_.value().cover_id = cover_id;
            onSetCover(cover_id, nullptr);
		}
    }
}

void Xamp::onTranslationCompleted(const QString& keyword, const QString& result) {
    artist_dao_.updateArtistEnglishName(keyword, result);
}

void Xamp::onEditTags(int32_t /*playlist_id*/, const QList<PlayListEntity>& entities) {
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<TagEditPage> tag_edit_page(new TagEditPage(dialog.get(), entities));
    dialog->setContentWidget(tag_edit_page.get());
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EDIT));
    dialog->setTitle(tr("Edit track information"));    
    dialog->exec();
    if (playlist_tab_page_->count() > 0) {
        localPlaylistPage()->playlist()->reload();
    }
}

void Xamp::onReadFileProgress(int32_t progress) {
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->setValue(progress);    
}

void Xamp::onReadCompleted() {
    delay(1);
    if (playlist_tab_page_->count() > 0) {
        localPlaylistPage()->playlist()->reload();
    }
    music_library_page_->reload();
    if (!read_progress_dialog_) {
        return;
    }
    read_progress_dialog_->close();
    read_progress_dialog_.reset();    
}

void Xamp::onRemainingTimeEstimation(size_t total_work, size_t completed_work, int32_t secs) {
    if (!read_progress_dialog_) {
        return;
    }

    read_progress_dialog_->setTitle(qSTR("Remaining Time: %1 seconds, process file total: %2, completed: %3.")
        .arg(formatDuration(secs)).arg(total_work).arg(completed_work));
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
    ui_.naviBar->setTabText(tr("YouTube search"), TAB_YT_MUSIC_SEARCH);
    ui_.naviBar->setTabText(tr("YouTube playlist"), TAB_YT_MUSIC_PLAYLIST);

    music_library_page_->onRetranslateUi();
    cd_page_->onRetranslateUi();
    initialDeviceList();
}

void Xamp::onFoundFileCount(size_t file_count) {    
    if (!read_progress_dialog_) {
        read_progress_dialog_ = makeProgressDialog(kApplicationTitle,
            tr("Read track information"),
            tr("Cancel"));

        (void)QObject::connect(read_progress_dialog_.get(),
            &XProgressDialog::cancelRequested, [this]() {
                file_system_service_->cancelRequested();
                album_cover_service_->cancelRequested();
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
}

