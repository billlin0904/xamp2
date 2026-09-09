#include <thememanager.h>
#include <version.h>
#include <xamp.h>
#include <deviceselectormenu.h>
#include "playbackcontroller.h"
#include "playbackpresenter.h"
#include "applicationupdater.h"
#include "applicationservices.h"

#include <algorithm>
#include <iterator>

#include <QAction>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QApplication>
#include <QCryptographicHash>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QMap>
#include <QProcess>
#include <QPointer>
#include <QScreen>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QFileInfo>

#include <base/threadpool.h>
#include <base/threadpoolbuilder.h>
#include <base/crashhandler.h>
#include <base/scopeguard.h>
#include <base/stopwatch.h>

#include <player/audio_player.h>
#include <stream/api.h>
#include <stream/idspmanager.h>
#include <stream/mqafilestream.h>
#include <stream/mqaidentifier.h>

#include <output_device/audiodevicemanager.h>

#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>
#include <widget/equalizerview.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/jsonsettings.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/filesystemviewpage.h>
#include <widget/chatgpt/spectrogramwidget.h>
#include <widget/richplaylistpage.h>
#include <widget/databasefacade.h>
#include <widget/imagecache.h>
#include <widget/playlisttableview.h>
#include <widget/playlistpage.h>
#include <widget/encodejobwidget.h>
#include <widget/logview.h>
#include <widget/preferencepage.h>
#include <widget/cdpage.h>
#include <widget/waveformslider.h>
#include <widget/musicbrainzeditpage.h>
#include <widget/aboutpage.h>
#include <widget/scanfileprogresspage.h>
#include <widget/xmessagebox.h>
#include <widget/worker/albumcoverservice.h>
#include <widget/worker/backgroundservice.h>
#include <widget/worker/filesystemservice.h>

#include <QInputDialog>
#include <QMenu>
#include <QShortcut>
#include <QSignalBlocker>
#include <style_util.h>

namespace {
    size_t countTracks(const std::forward_list<TrackInfo>& tracks) {
        return static_cast<size_t>(std::distance(tracks.begin(), tracks.end()));
    }

    size_t countTrackBatches(const std::vector<std::forward_list<TrackInfo>>& batches) {
        size_t track_count = 0;
        for (const auto& tracks : batches) {
            track_count += countTracks(tracks);
        }
        return track_count;
    }

    void collectAlbumIds(const std::forward_list<TrackInfo>& tracks, QSet<int32_t>& album_ids) {
        for (const auto& track : tracks) {
            const auto music_id = qDaoFacade.music_dao.getMusicId(toQString(track.file_path));
            if (!music_id.has_value()) {
                continue;
            }
            const auto album_id = qDaoFacade.album_dao.getAlbumIdFromAlbumMusic(music_id.value());
            if (album_id > 0 && album_id != qDatabaseFacade.unknownAlbumId()) {
                album_ids.insert(album_id);
            }
        }
    }

    QSet<int32_t> collectAlbumIds(const std::vector<std::forward_list<TrackInfo>>& batches) {
        QSet<int32_t> album_ids;
        for (const auto& tracks : batches) {
            collectAlbumIds(tracks, album_ids);
        }
        return album_ids;
    }

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

    QSize dialogSizeFromHost(QWidget* dialog,
        const QWidget* content,
        const QWidget* host,
        double ratio) {
        auto* screen = dialog != nullptr ? dialog->screen() : QGuiApplication::primaryScreen();
        const auto fallback_size = dialogContentSize(dialog, content);
        if (screen == nullptr || host == nullptr) {
            return fallback_size;
        }

        const auto available = screen->availableGeometry().size();
        const auto host_size = host->size();
        if (host_size.isEmpty()) {
            return fallback_size;
        }

        const QSize target_size(static_cast<int>(host_size.width() * ratio),
            static_cast<int>(host_size.height() * ratio));
        const QSize max_size(static_cast<int>(available.width() * 0.95),
            static_cast<int>(available.height() * 0.95));
        return target_size
            .expandedTo(fallback_size)
            .boundedTo(max_size);
    }

    void applyParametricEqToPlayer(const std::shared_ptr<IAudioPlayer>& player,
        bool enabled,
        const EqSettings& settings) {
        player->setParametricEq(enabled, settings);
    }


}

Xamp::Xamp(QWidget* parent, const std::shared_ptr<IAudioPlayer>& player)
    : IXFrame(parent)
	, player_(player) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_.setupUi(this);
    ui_.sliderFrame2->setMinimumWidth(50);
    ui_.sliderFrame2->setMaximumWidth(204);
    ui_.currentView->setContentsMargins(0, 0, 0, 0);
    ui_.verticalSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui_.verticalSpacer_4->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    ui_.horizontalLayout->setStretch(0, 0);
    ui_.horizontalLayout->setStretch(1, 0);
    ui_.horizontalLayout->setStretch(2, 0);
    ui_.horizontalLayout->setStretch(3, 1);
    ui_.currentView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui_.horizontalLayout->invalidate();
    device_menu_.reset(new DeviceSelectorMenu(ui_.selectDeviceButton, ui_.deviceDescLabel, this));
    const auto refresh_transport = [this] {
        const bool dark = qTheme.isDarkTheme();
        ui_.seekSlider->setWaveformColors(
            dark ? QColor("#252D2F"_str) : QColor("#E3EAE6"_str),
            dark ? QColor("#53655E"_str) : QColor("#A3B4AB"_str),
            dark ? QColor("#94D8C3"_str) : QColor("#25745B"_str));
        qTheme.setPlayOrPauseButton(ui_.playButton, playback_ && playback_->snapshot().status == PlaybackStatus::Playing);
        ui_.prevButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_BACKWARD));
        ui_.nextButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_FORWARD));
    };
    refresh_transport();
    connect(&qTheme, &ThemeManager::themeChangedFinished, this, refresh_transport);
}

Xamp::~Xamp() {
    if (preference_page_) preference_page_->saveAll();
    destory();
}

void Xamp::setCurrentTab(int32_t tab_id) {
    QWidget* page = nullptr;
    switch (tab_id) {
    case TAB_FILE_EXPLORER: page = file_explorer_page_.get(); break;
    case TAB_RICH_PLAYLIST: page = rich_playlist_page_.get(); break;
    case TAB_LYRICS: page = lrc_page_.get(); break;
    case TAB_CD: page = cd_page_.get(); break;
    default: return;
    }
    if (!page || ui_.currentView->indexOf(page) < 0) return;
    if (tab_id == TAB_LYRICS) ui_.currentView->slideInWidget(page);
    else ui_.currentView->setCurrentWidget(page);
}

void Xamp::initialDeviceList(const std::string& device_id) {
    XAMP_LOG_DEBUG("initial device list");

    const auto device_type_id = qAppSettings.valueAsId(kAppSettingDeviceType);
    auto current_device_id = device_id;
    if (current_device_id.empty()) {
        current_device_id = qAppSettings.valueAsString(kAppSettingDeviceId).toStdString();
    }

    auto apply_device = [this](const DeviceInfo& device_info) {
        device_info_ = device_info;
        playback_->setDevice(device_info);
        qAppSettings.setValue(kAppSettingDeviceType, device_info.device_type_id);
        qAppSettings.setValue(kAppSettingDeviceId, device_info.device_id);
    };

    const auto selected_device = device_menu_->rebuild(player_->getAudioDeviceManager(),
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

    playback_->shutdown();
    services_->shutdown();
    qGuiDb.close();
    XampCrashHandler.cleanup();
    main_window_ = nullptr;


}

void Xamp::shortcutsPressed(const QKeySequence& shortcut) {
    XAMP_LOG_DEBUG("shortcutsPressed: {}", shortcut.toString().toStdString());

    const QMap<QKeySequence, std::function<void()>> shortcut_map{
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
            setVolume(player_->getVolume() + 1);
            }},
        { QKeySequence(Qt::Key_VolumeDown), [this]() {
            setVolume(player_->getVolume() - 1);
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
    const auto ui_cover = image_util::roundCoverImage(cover,
        ui_.coverLabel->size(),
        image_util::kPlaylistImageRadius);
    ui_.coverLabel->setPixmap(ui_cover);
}

void Xamp::invalidateAlbumCover(int32_t music_id, int32_t album_id) {
    if (album_id <= 0) {
        return;
    }

    try {
        const auto old_cover_id = qDaoFacade.album_dao.getAlbumCoverId(album_id);
        qImageCache.removeCoverId(old_cover_id);
        qDaoFacade.album_dao.clearAlbumCover(album_id);
    }
    catch (const std::exception& e) {
        XAMP_LOG_DEBUG("Failure to invalidate album cover. album:{} error:{}", album_id, e.what());
        return;
    }

    file_explorer_page_->playlistPage()->playlist()->setAlbumCoverId(album_id, QString());
    cd_page_->playlistPage()->playlist()->setAlbumCoverId(album_id, QString());
    rich_playlist_page_->onAlbumCoverLoaded(album_id);

    QMetaObject::invokeMethod(services_->covers(),
        [service = services_->covers(), music_id, album_id]() {
            service->removeAlbumCoverId(album_id);
            service->onFindAlbumCover(DatabaseCoverId(music_id, album_id));
        },
        Qt::QueuedConnection);
}

void Xamp::setMainWindow(IXMainWindow* main_window) {
    main_window_ = main_window;
    playback_.reset(new PlaybackController(player_, this));
    services_ = std::make_unique<ApplicationServices>();
    updater_.reset(new ApplicationUpdater(this));
    connect(playback_.get(), &PlaybackController::failed, this, [](std::exception_ptr error) { logAndShowMessage(error); });
    connect(playback_.get(), &PlaybackController::startRequested, this, [this] { rich_playlist_page_->playNextItem(1); });


    setThemeIcon(ui_);
    setWidgetStyle(ui_);
    setShufflePlayOrder(ui_);
    updateButtonState(ui_.playButton, PlayerState::PLAYER_STATE_STOPPED);
    initialDeviceList();

    if (!qDaoFacade.playlist_dao.isPlaylistExist(kDefaultPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("Default Playlist"),
            kDefaultPlaylistId);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kFileSystemPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("FileSystem Playlist"),
            kFileSystemPlaylistId);
    }    
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kCdPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("CD Playlist"),
            kCdPlaylistId);
    }
    if (!qDaoFacade.playlist_dao.isPlaylistExist(kAlbumPlaylistId)) {
        qDaoFacade.playlist_dao.addPlaylist(
            tr("Album Playlist"),
            kAlbumPlaylistId);
    }

    (void)QObject::connect(ui_.prevButton, &QToolButton::clicked, [this]() {
        playPrevious();
        });

    (void)QObject::connect(ui_.nextButton, &QToolButton::clicked, [this]() {
        playNext();
        });

    setAlbumCover(qTheme.unknownCover());

    setRepeatButtonIcon(ui_, qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder));

    ui_.naviBar->addTab(translateText("Playlist"), TAB_RICH_PLAYLIST, qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
    ui_.naviBar->addTab(translateText("Library"), TAB_FILE_EXPLORER, qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_.naviBar->addTab(translateText("Lyrics"), TAB_LYRICS, qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
    ui_.naviBar->addTab(translateText("CD"), TAB_CD, qTheme.fontIcon(Glyphs::ICON_CD));
    ui_.sliderFrame2->setFixedWidth(204);

    lrc_page_.reset(new LrcPage(this));
    rich_playlist_page_.reset(new RichPlaylistPage(this));
    file_explorer_page_.reset(new FileSystemViewPage(this));
    file_explorer_page_->setScannerThreadPool(ThreadPoolBuilder::makeBackgroundThreadPool());


    ui_.mutedButton->setAudioPlayer(player_);
    ui_.mutedButton->updateState();

    auto f = font();
    f.setPointSize(qTheme.fontSize(11));
    f.setWeight(QFont::DemiBold);
    ui_.titleLabel->setFont(f);
    ui_.titleLabel->setElideMode(Qt::ElideRight);
    f.setPointSize(qTheme.fontSize(9));
    f.setWeight(QFont::Normal);
    ui_.artistLabel->setFont(f);
    ui_.artistLabel->setWordWrap(false);
    ui_.artistLabel->setElideMode(Qt::ElideRight);

    connect(file_explorer_page_->spectrogramWidget(), &SpectrogramWidget::playAt, this, [this](float sec) {
        if (playback_->snapshot().source == PlaybackSource::Library) playback_->seek(sec);
    });

    (void)QObject::connect(playback_->adapter().get(),
        &UIPlayerStateAdapter::deviceChanged,
        this,
        &Xamp::onDeviceStateChanged,
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_->playlistPage()->playlist(),
        &PlaylistTableView::playMusic,
        this,
        [this](int32_t playlist_id, const PlayListEntity& item, bool is_play) {
            (void)is_play;
            playback_->play(item, playlist_id, PlaybackSource::Library, file_explorer_page_->playlistPage()->playlist()->items());
        });

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::playMusic,
        this,
        [this](int32_t playlist_id, const PlayListEntity& item, bool is_play) {
            (void)is_play;
            playback_->play(item, playlist_id, PlaybackSource::Playlist, rich_playlist_page_->tracks());
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
        ui_.shuffleButton->setChecked(order == PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM);
        playback_->refreshOrder();
        });

    connect(ui_.seekSlider, &WaveformSlider::leftButtonValueChanged, this, [this](auto value) {
        playback_->seek(value / 1000.0);
    });

    (void)QObject::connect(ui_.eqButton, &QToolButton::clicked, [this]() {
        if (player_->getDsdModes() == DsdModes::DSD_MODE_DOP
            || player_->getDsdModes() == DsdModes::DSD_MODE_NATIVE) {
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
        (void)QObject::connect(playback_->adapter().get(),
            static_cast<void (UIPlayerStateAdapter::*)(int32_t, size_t)>(&UIPlayerStateAdapter::outputFormatChanged),
            eq.get(),
            &EqualizerView::outputFormatChanged,
            Qt::QueuedConnection);
        (void)QObject::connect(playback_->adapter().get(),
            &UIPlayerStateAdapter::samplesChanged,
            eq.get(),
            &EqualizerView::samplesChanged,
            Qt::QueuedConnection);
        eq->outputFormatChanged(playback_->adapter()->sampleRate(), playback_->adapter()->outputBufferSize());
        dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
        dialog->setTitle(tr("Equalizer"));
        dialog->setContentWidget(eq.get(), false);
        dialog->setTitleBarBackgroundColor(QColor(qTheme.backgroundColorString()));
        dialog->resize(dialogContentSize(dialog.get(), eq.get()));
        dialog->setFixedSize(dialog->size());
        dialog->exec();
        });

    //setupSystemMenu();

    cd_page_.reset(new CdPage(this));
    cd_page_->playlistPage()->playlist()->setPlaylistId(kCdPlaylistId, kAppSettingCdPlaylistColumnName);
    cd_page_->playlistPage()->playlist()->setHeaderViewHidden(false);
    cd_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);
    file_explorer_page_->playlistPage()->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    auto connect_playlist_cover_service = [this](PlaylistTableView* playlist) {
        (void)QObject::connect(playlist->styledDelegate(),
            &PlaylistStyledItemDelegate::findAlbumCover,
            services_->covers(),
            &AlbumCoverService::onFindAlbumCover,
            Qt::QueuedConnection);
        };

    connect_playlist_cover_service(file_explorer_page_->playlistPage()->playlist());
    connect_playlist_cover_service(cd_page_->playlistPage()->playlist());

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::findAlbumCover,
        services_->covers(),
        &AlbumCoverService::onFindAlbumCover,
        Qt::QueuedConnection);

    auto connect_playlist_changed = [this](PlaylistTableView* playlist) {
        (void)QObject::connect(playlist,
            &PlaylistTableView::playlistChanged,
            this,
            [this](int32_t) {
                rich_playlist_page_->reload();
                ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
                setCurrentTab(TAB_RICH_PLAYLIST);
            });
        };

    connect_playlist_changed(file_explorer_page_->playlistPage()->playlist());
    connect_playlist_changed(cd_page_->playlistPage()->playlist());

    (void)QObject::connect(services_->covers(),
        &AlbumCoverService::albumCoverLoaded,
        this,
        [this](int32_t album_id, const QImage& image, bool save_only) {
            Stopwatch total_elapsed;
            Stopwatch stage_elapsed;

            const auto cover = QPixmap::fromImage(image);
            const auto convert_elapsed = stage_elapsed.elapsedSeconds();
            if (cover.isNull()) {
                XAMP_LOG_DEBUG("Album cover loaded but pixmap is null. album:{} save_only:{}",
                    album_id,
                    save_only);
                return;
            }

            stage_elapsed.reset();
            const auto cover_id = qImageCache.addImage(cover, save_only);
            const auto cache_elapsed = stage_elapsed.elapsedSeconds();

            stage_elapsed.reset();
            qDaoFacade.album_dao.setAlbumCover(album_id, cover_id);
            const auto db_elapsed = stage_elapsed.elapsedSeconds();

            stage_elapsed.reset();
            file_explorer_page_->playlistPage()->playlist()->setAlbumCoverId(album_id, cover_id);
            const auto file_playlist_elapsed = stage_elapsed.elapsedSeconds();

            stage_elapsed.reset();
            cd_page_->playlistPage()->playlist()->setAlbumCoverId(album_id, cover_id);
            const auto cd_playlist_elapsed = stage_elapsed.elapsedSeconds();

            stage_elapsed.reset();
            rich_playlist_page_->onAlbumCoverLoaded(album_id);
            playback_->updateAlbumCover(album_id, cover);
            const auto rich_reload_elapsed = stage_elapsed.elapsedSeconds();

            XAMP_LOG_DEBUG("Album cover loaded. album:{} cover:{} save_only:{} size:{}x{} convert:{:.3f}s cache:{:.3f}s db:{:.3f}s file_playlist:{:.3f}s cd_playlist:{:.3f}s rich_reload:{:.3f}s total:{:.3f}s",
                album_id,
                cover_id.toStdString(),
                save_only,
                cover.width(),
                cover.height(),
                convert_elapsed,
                cache_elapsed,
                db_elapsed,
                file_playlist_elapsed,
                cd_playlist_elapsed,
                rich_reload_elapsed,
                total_elapsed.elapsedSeconds());
        },
        Qt::QueuedConnection);

    auto request_album_covers = [this](const QSet<int32_t>& album_ids) {
        for (const auto album_id : album_ids) {
            QMetaObject::invokeMethod(services_->covers(),
                [service = services_->covers(), album_id]() {
                    service->onFindAlbumCover(DatabaseCoverId(kInvalidDatabaseId, album_id));
                },
                Qt::QueuedConnection);
        }
        };

    auto* rich_playlist_progress = rich_playlist_page_->progressPage();

    (void)QObject::connect(rich_playlist_page_.get(),
        &RichPlaylistPage::extractFile,
        services_->files(),
        &FileSystemService::onExtractFile,
        Qt::QueuedConnection);

    (void)QObject::connect(file_explorer_page_.get(),
        &FileSystemViewPage::addPathToPlaylist,
        this,
        [this](const QString& path, bool append_to_playlist) {
            rich_playlist_page_->loadPath(path, append_to_playlist);
            ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
            setCurrentTab(TAB_RICH_PLAYLIST);
        });

    (void)QObject::connect(rich_playlist_progress,
        &ScanFileProgressPage::cancelRequested,
        services_->files(),
        &FileSystemService::cancelRequested,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->files(),
        &FileSystemService::batchInsertDatabase,
        this,
        [this, request_album_covers](const std::vector<std::forward_list<TrackInfo>>& results, int32_t playlist_id) {
            Stopwatch total_elapsed;
            Stopwatch stage_elapsed;
            const auto track_count = countTrackBatches(results);
            qDatabaseFacade.insertMultipleTrackInfo(results,
                playlist_id);
            const auto insert_seconds = stage_elapsed.elapsedSeconds();
            stage_elapsed.reset();
            request_album_covers(collectAlbumIds(results));
            rich_playlist_page_->reload();
            const auto reload_seconds = stage_elapsed.elapsedSeconds();
            XAMP_LOG_DEBUG("Metadata DB batch write playlist:{} batches:{} tracks:{} insert:{:.3f}s reload:{:.3f}s total:{:.3f}s",
                playlist_id,
                results.size(),
                track_count,
                insert_seconds,
                reload_seconds,
                total_elapsed.elapsedSeconds());
        },
        Qt::QueuedConnection);

    (void)QObject::connect(services_->files(),
        &FileSystemService::readFileStart,
        rich_playlist_progress,
        &ScanFileProgressPage::onReadFileStart,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->files(),
        &FileSystemService::readFileProgress,
        rich_playlist_progress,
        &ScanFileProgressPage::setFileProgress,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->files(),
        &FileSystemService::foundFileCount,
        rich_playlist_progress,
        &ScanFileProgressPage::setFileCount,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->files(),
        &FileSystemService::readFilePath,
        rich_playlist_progress,
        &ScanFileProgressPage::onReadFilePath,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->files(),
        &FileSystemService::remainingTimeEstimation,
        rich_playlist_progress,
        &ScanFileProgressPage::onRemainingTimeEstimation,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->files(),
        &FileSystemService::readCompleted,
        rich_playlist_progress,
        &ScanFileProgressPage::onReadCompleted,
        Qt::QueuedConnection);

    ui_.currentView->addWidget(lrc_page_.get());
    ui_.currentView->addWidget(rich_playlist_page_.get());
    ui_.currentView->addWidget(file_explorer_page_.get());
    ui_.currentView->addWidget(cd_page_.get());

    ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
    setCurrentTab(TAB_RICH_PLAYLIST);

    initializeModernControls();

#ifdef Q_OS_WIN
    (void)QObject::connect(this,
        &Xamp::fetchCdInfo,
        services_->background(),
        &BackgroundService::onFetchCdInfo);
#endif

    (void)QObject::connect(this,
        &Xamp::searchLyrics,
        services_->background(),
        &BackgroundService::onSearchLyrics,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->background(),
        &BackgroundService::fetchLyricsCompleted,
        lrc_page_.get(),
        &LrcPage::onFetchLyricsCompleted,
        Qt::QueuedConnection);

    (void)QObject::connect(services_->background(),
        &BackgroundService::readCdTrackInfo,
        this,
        &Xamp::onUpdateCdTrackInfo,
        Qt::QueuedConnection);

    auto connect_encode_jobs = [this](PlaylistTableView* playlist) {
        (void)QObject::connect(playlist,
            &PlaylistTableView::encodeAlacFiles,
            this,
            &Xamp::showEncodeJobs);
        };

    connect_encode_jobs(file_explorer_page_->playlistPage()->playlist());
    connect_encode_jobs(cd_page_->playlistPage()->playlist());

    (void)QObject::connect(file_explorer_page_->playlistPage()->playlist(),
        &PlaylistTableView::findMusicbrainRecording,
        this,
        &Xamp::OnReadMusicBrainzAlbums);

    presenter_.reset(new PlaybackPresenter(*playback_, ui_, *main_window_, *lrc_page_,
        *rich_playlist_page_, *file_explorer_page_, *cd_page_,
        [this](const PlayListEntity& track) { emit searchLyrics(track); }, this));
    connect(rich_playlist_page_.get(), &RichPlaylistPage::playQueuedTrack, playback_.get(), &PlaybackController::playQueued);
    connect(rich_playlist_page_.get(), &RichPlaylistPage::favoriteRequested, playback_.get(), &PlaybackController::setFavorite);
    connect(cd_page_->playlistPage()->playlist(), &PlaylistTableView::playMusic, this,
        [this](int id, const PlayListEntity& track, bool) {
            playback_->play(track, id, PlaybackSource::Cd, cd_page_->playlistPage()->playlist()->items());
        });
    connect(updater_.get(), &ApplicationUpdater::installing, this, [this] {
        if (preference_page_) preference_page_->saveAll();
        playback_->saveUpdateSession();
    });
    QTimer::singleShot(0, this, [this] { playback_->restoreUpdateSession(); });
    updater_->start();
}

void Xamp::OnReadMusicBrainzAlbums(const QList<PlayListEntity>& entities) {
    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<MusicbrainzEditPage> eq(new MusicbrainzEditPage(entities, dialog.get()));
    (void)QObject::connect(eq.get(),
        &MusicbrainzEditPage::albumCoverChanged,
        this,
        &Xamp::invalidateAlbumCover);
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_DRAFT));
    dialog->setTitle(tr("Musicbrainz Edit Page"));
    dialog->setContentWidget(eq.get(), false, false);
    dialog->setMinimumSize(eq->minimumSizeHint());
    dialog->resize(dialogSizeFromHost(dialog.get(), eq.get(), main_window_, 0.8));
    dialog->exec();
}

void Xamp::setupSystemMenu() {
    if (main_window_ == nullptr) {
        return;
    }

    main_window_->clearSystemMenuActions();

    preference_action_ = new QAction(tr("Settings") + "..."_str, this);
    preference_action_->setProperty("translationSource", QStringLiteral("Settings"));
    (void)QObject::connect(preference_action_, &QAction::triggered, this, &Xamp::showPreference);
    main_window_->addSystemMenuAction(preference_action_);

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    auto* check_for_update_action = new QAction(tr("Check for update"), this);
    check_for_update_action->setProperty("translationSource", QStringLiteral("Check for update"));
    (void)QObject::connect(check_for_update_action, &QAction::triggered, this, &Xamp::onCheckForUpdate);
    main_window_->addSystemMenuAction(check_for_update_action);
#endif

    auto* about_action = new QAction(tr("About") + "..."_str, this);
    about_action->setProperty("translationSource", QStringLiteral("About"));
    (void)QObject::connect(about_action, &QAction::triggered, this, &Xamp::showAbout);
    main_window_->addSystemMenuAction(about_action);

    auto* log_action = new QAction(tr("Log viewer") + "..."_str, this);
    log_action->setProperty("translationSource", QStringLiteral("Log viewer"));
    (void)QObject::connect(log_action, &QAction::triggered, this, &Xamp::showLogViewer);
    main_window_->addSystemMenuAction(log_action);
}

void Xamp::showPreference() {
    if (!settings_panel_) {
        settings_panel_ = new QWidget(ui_.currentView);
        auto* layout = new QVBoxLayout(settings_panel_);
        layout->setContentsMargins(24, 20, 24, 16);
        auto* title = new QLabel(tr("Settings"), settings_panel_);
        title->setObjectName("playlistTitle"_str);
        layout->addWidget(title);
        auto* scroll = new QScrollArea(settings_panel_);
        scroll->setWidgetResizable(true);
        preference_page_ = new PreferencePage(scroll);
        preference_page_->loadSettings();
        connect(preference_page_, &PreferencePage::retranslateUi, this, [this, title] {
            title->setText(tr("Settings"));
            ui_.settingsButton->setText(tr("Settings"));
            ui_.sidebarSearch->setPlaceholderText(tr("Search music"));
            ui_.playlistSectionLabel->setText(tr("My playlists"));
            ui_.mutedButton->setToolTip(tr("Mute / Unmute"));
            ui_.volumeSlider->setToolTip(tr("Volume"));
            ui_.volumeSlider->setAccessibleName(tr("Volume"));
            ui_.queueButton->setToolTip(tr("Show queue"));
            ui_.shuffleButton->setToolTip(tr("Shuffle"));
            ui_.naviBar->setTabText(TAB_RICH_PLAYLIST, tr("Playlist"));
            ui_.naviBar->setTabText(TAB_FILE_EXPLORER, tr("Library"));
            ui_.naviBar->setTabText(TAB_LYRICS, tr("Lyrics"));
            ui_.naviBar->setTabText(TAB_CD, tr("CD"));
            for (auto* action : findChildren<QAction*>()) {
                const auto source = action->property("translationSource").toByteArray();
                if (!source.isEmpty()) action->setText(QCoreApplication::translate("Xamp", source.constData())
                    + (source == "Check for update" ? QString() : "..."_str));
            }
            if (auto* item = ui_.sidebarPlaylists->currentItem()) {
                refreshPlaylistNavigation(item->data(Qt::UserRole).toInt());
            }
            rich_playlist_page_->retranslate();
        });
        preference_page_->addUpdatesPage(updater_->createSettingsPage(preference_page_));
        scroll->setWidget(preference_page_);
        layout->addWidget(scroll, 1);
        ui_.currentView->addWidget(settings_panel_);
        connect(ui_.currentView, &QStackedWidget::currentChanged, this, [this] {
            const bool active = ui_.currentView->currentWidget() == settings_panel_;
            ui_.settingsButton->setChecked(active);
            if (!active) preference_page_->saveAll();
        });
    }
    ui_.naviBar->setCurrentIndex(-1);
    ui_.currentView->setCurrentWidget(settings_panel_);
    ui_.settingsButton->setChecked(true);
}

void Xamp::showLogViewer() {
    static QPointer<XDialog> log_dialog;
    if (log_dialog != nullptr) {
        log_dialog->show();
        log_dialog->raise();
        log_dialog->activateWindow();
        return;
    }

    auto* dialog = new XDialog(nullptr, false);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlag(Qt::Window, true);
    dialog->setTitle(tr("Log viewer"));
    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_REPORT_BUG));

    auto* log_view = new LogView(dialog);
    log_view->loadLogFile("logs/xamp.log"_str);
    log_view->setMinimumSize(QSize(900, 480));
    dialog->setContentWidget(log_view, false, false);
    dialog->resize(dialogSizeFromHost(dialog, log_view, main_window_, 0.6));
    log_dialog = dialog;
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void Xamp::showEncodeJobs(int32_t encode_type, const QList<PlayListEntity>& entities) {
    if ((!services_ || services_->background() == nullptr) || entities.isEmpty()) {
        return;
    }

    const auto dir_name = getExistingDirectory(this, tr("Select output directory"));
    if (dir_name.isEmpty()) {
        return;
    }

    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<EncodeJobWidget> encode_job_widget(new EncodeJobWidget(dialog.get()));
    encode_job_widget->setMinimumSize(QSize(1100, 520));

    const auto jobs = encode_job_widget->addJobs(encode_type, entities);
    if (jobs.isEmpty()) {
        return;
    }

    (void)QObject::connect(services_->background(),
        &BackgroundService::updateJobProgress,
        encode_job_widget.get(),
        &EncodeJobWidget::onUpdateProgress,
        Qt::QueuedConnection);
    (void)QObject::connect(services_->background(),
        &BackgroundService::jobError,
        encode_job_widget.get(),
        &EncodeJobWidget::onJobError,
        Qt::QueuedConnection);

    QMetaObject::invokeMethod(services_->background(),
        [service = services_->background(), dir_name, jobs]() {
            service->onAddJobs(dir_name, jobs);
        },
        Qt::QueuedConnection);

    dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_EXPORT_FILE));
    dialog->setTitle(tr("encode Jobs"));
    dialog->setContentWidget(encode_job_widget.get(), false);
    dialog->resize(dialogSizeFromHost(dialog.get(), encode_job_widget.get(), main_window_, 0.75));
    dialog->exec();
}

void Xamp::onCheckForUpdate() { showPreference(); preference_page_->showUpdatesPage(); updater_->checkNow(); }

void Xamp::showAbout() {
    QScopedPointer<MaskWidget> mask_widget(new MaskWidget(this));
    QScopedPointer<XDialog> dialog(new XDialog(this));
    QScopedPointer<AboutPage> about_page(new AboutPage(dialog.get()));
    (void)QObject::connect(about_page.get(),
        &AboutPage::checkForUpdate,
        this,
        &Xamp::onCheckForUpdate);
    (void)QObject::connect(about_page.get(),
        &AboutPage::restartApp,
        []() {
            qApp->exit(kRestartExistCode);
        });
    dialog->setIcon(qTheme.applicationIcon());
    dialog->setTitle(tr("About"));
    dialog->setContentWidget(about_page.get(), false);
    dialog->setFixedSize(dialog->size());
    dialog->exec();

}

void Xamp::onDeviceStateChanged(DeviceState state, const QString& device_id) {
    XAMP_LOG_DEBUG("OnDeviceStateChanged: {}", state);

    if (state == DeviceState::DEVICE_STATE_REMOVED) {
        playback_->stop(true);
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
        disc_id);

    cd_page_->playlistPage()->playlist()->reload();
    cd_page_->showPlaylistPage(true);
}

void Xamp::refreshPlaylistNavigation(int selected_id) {
    const QSignalBlocker blocker(ui_.sidebarPlaylists);
    ui_.sidebarPlaylists->clear();
    QListWidgetItem* selected = nullptr;
    qDaoFacade.playlist_dao.forEachPlaylist([this, selected_id, &selected](int id, int, const QString& name) {
        if (id != kDefaultPlaylistId && id < kMaxExistPlaylist) return;
        auto* item = new QListWidgetItem(qTheme.fontIcon(Glyphs::ICON_PLAYLIST),
            id == kDefaultPlaylistId ? (qAppSettings.contains("defaultPlaylistDisplayName"_str)
                ? qAppSettings.valueAsString("defaultPlaylistDisplayName"_str) : tr("My music")) : name, ui_.sidebarPlaylists);
        item->setData(Qt::UserRole, id);
        item->setFlags((item->flags() | Qt::ItemIsDragEnabled) & ~Qt::ItemIsDropEnabled);
        item->setSizeHint(QSize(160, 42));
        if (id == selected_id) selected = item;
    });
    const auto order = qAppSettings.valueAsStringList("sidebarPlaylistOrder"_str);
    int destination = 0;
    QSet<int> restored;
    for (const auto& value : order) {
        bool ok = false;
        const int id = value.toInt(&ok);
        if (!ok || restored.contains(id)) continue;
        for (int row = destination; row < ui_.sidebarPlaylists->count(); ++row) {
            if (ui_.sidebarPlaylists->item(row)->data(Qt::UserRole).toInt() != id) continue;
            ui_.sidebarPlaylists->insertItem(destination++, ui_.sidebarPlaylists->takeItem(row));
            restored.insert(id);
            break;
        }
    }
    if (selected) {
        ui_.sidebarPlaylists->setCurrentItem(selected);
        rich_playlist_page_->setPlaylist(selected_id, selected->text());
    }
}

void Xamp::addDropFileItem(const QUrl& url) {
    PlayListEntity track;
    track.file_path = url.toLocalFile();
    playback_->play(track, -1, PlaybackSource::External, {});
}

void Xamp::playPrevious() { playback_->next(-1); }
void Xamp::playNext() { playback_->next(1); }
void Xamp::playOrPause() { playback_->togglePause(); }
void Xamp::stopPlay() { playback_->stop(); }

void Xamp::initializeModernControls() {
    ui_.mutedButton->setToolTip(tr("Mute / Unmute"));
    connect(ui_.mutedButton, &QToolButton::clicked, this,
        [this, previous_volume = 50]() mutable {
            if (player_->isHardwareControlVolume()) return;
            const int volume = ui_.volumeSlider->value();
            if (volume > 0) {
                previous_volume = volume;
                ui_.volumeSlider->setValue(0);
            } else {
                ui_.volumeSlider->setValue(previous_volume);
            }
        });
    ui_.settingsButton->setCheckable(true);
    ui_.volumeSlider->setValue(qAppSettings.valueAsInt(kAppSettingVolume));
    ui_.volumeSlider->setAccessibleName(tr("Volume"));
    ui_.volumeSlider->setToolTip(tr("Volume"));
    connect(ui_.volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (!player_->isHardwareControlVolume()) ui_.mutedButton->onVolumeChanged(value);
    });
    connect(ui_.mutedButton, &VolumeButton::volumeChanged, this, [this](int value) {
        const QSignalBlocker blocker(ui_.volumeSlider);
        ui_.volumeSlider->setValue(value);
    });
    connect(playback_->adapter().get(), &UIPlayerStateAdapter::volumeChanged, this, [this](float value) {
        const QSignalBlocker blocker(ui_.volumeSlider);
        ui_.volumeSlider->setValue(qRound(value));
    });
    connect(ui_.settingsButton, &QPushButton::clicked, this, &Xamp::showPreference);
    ui_.settingsButton->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
    ui_.sidebarSearch->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::LeadingPosition);
    connect(ui_.sidebarSearch, &QLineEdit::textChanged, this, [this](const QString& text) {
        setCurrentTab(TAB_RICH_PLAYLIST);
        ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
        rich_playlist_page_->search(text);
    });
    ui_.sidebarPlaylists->setDragDropMode(QAbstractItemView::InternalMove);
    ui_.sidebarPlaylists->setDefaultDropAction(Qt::MoveAction);
    ui_.sidebarPlaylists->setDragDropOverwriteMode(false);
    ui_.sidebarPlaylists->setDropIndicatorShown(true);
    ui_.sidebarPlaylists->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(ui_.sidebarPlaylists->model(), &QAbstractItemModel::rowsMoved, this, [this] {
        QStringList order;
        for (int row = 0; row < ui_.sidebarPlaylists->count(); ++row)
            order.append(QString::number(ui_.sidebarPlaylists->item(row)->data(Qt::UserRole).toInt()));
        qAppSettings.setValue("sidebarPlaylistOrder"_str, order);
        qAppSettings.save();
    });
    connect(ui_.sidebarPlaylists, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* item) {
        if (!item) return;
        rich_playlist_page_->setPlaylist(item->data(Qt::UserRole).toInt(), item->text());
        ui_.sidebarSearch->clear();
        setCurrentTab(TAB_RICH_PLAYLIST);
        ui_.naviBar->setCurrentIndex(TAB_RICH_PLAYLIST);
    });
    const auto rename_playlist = [this](QListWidgetItem* item) {
        if (!item) return;
        bool ok = false;
        const auto name = QInputDialog::getText(this, tr("Rename playlist"), tr("Playlist name"),
            QLineEdit::Normal, item->text(), &ok).trimmed();
        if (!ok || name.isEmpty() || name == item->text()) return;
        const auto id = item->data(Qt::UserRole).toInt();
        qDaoFacade.playlist_dao.setPlaylistName(id, name);
        if (id == kDefaultPlaylistId) {
            qAppSettings.setValue("defaultPlaylistDisplayName"_str, name);
            qAppSettings.save();
        }
        item->setText(name);
        if (item == ui_.sidebarPlaylists->currentItem()) rich_playlist_page_->setPlaylistName(name);
    };
    const auto delete_playlist = [this](QListWidgetItem* item) {
        if (!item) return;
        const int id = item->data(Qt::UserRole).toInt();
        if (id < kMaxExistPlaylist || id == kDefaultPlaylistId) return;
        XMessageBox confirmation(tr("Delete playlist"),
            tr("Delete playlist \"%1\"? Music files will not be deleted.").arg(item->text()),
            this, QDialogButtonBox::Yes | QDialogButtonBox::No, QDialogButtonBox::No, false);
        confirmation.exec();
        if (confirmation.standardButton(confirmation.clickedButton()) != QDialogButtonBox::Yes) return;
        try {
            qDaoFacade.playlist_dao.removePlaylist(id);
            playback_->discardPlaylist(id);
            auto order = qAppSettings.valueAsStringList("sidebarPlaylistOrder"_str);
            order.removeAll(QString::number(id));
            qAppSettings.setValue("sidebarPlaylistOrder"_str, order);
            qAppSettings.save();
            const auto* current = ui_.sidebarPlaylists->currentItem();
            const int next_id = current && current->data(Qt::UserRole).toInt() != id
                ? current->data(Qt::UserRole).toInt() : kDefaultPlaylistId;
            refreshPlaylistNavigation(next_id);
        } catch (...) { logAndShowMessage(std::current_exception()); }
    };
    ui_.sidebarPlaylists->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_.sidebarPlaylists, &QListWidget::customContextMenuRequested, this,
        [this, rename_playlist, delete_playlist](const QPoint& pos) {
            auto* item = ui_.sidebarPlaylists->itemAt(pos);
            if (!item) return;
            QMenu menu(this);
            auto* rename = menu.addAction(tr("Rename playlist"));
            auto* remove = menu.addAction(tr("Delete playlist"));
            const int id = item->data(Qt::UserRole).toInt();
            remove->setEnabled(id >= kMaxExistPlaylist && id != kDefaultPlaylistId);
            auto* chosen = menu.exec(ui_.sidebarPlaylists->viewport()->mapToGlobal(pos));
            if (chosen == rename) rename_playlist(item);
            else if (chosen == remove) delete_playlist(item);
        });
    auto* delete_shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), ui_.sidebarPlaylists);
    delete_shortcut->setContext(Qt::WidgetShortcut);
    connect(delete_shortcut, &QShortcut::activated, this, [this, delete_playlist] {
        delete_playlist(ui_.sidebarPlaylists->currentItem());
    });
    auto* rename_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), ui_.sidebarPlaylists);
    rename_shortcut->setContext(Qt::WidgetShortcut);
    connect(rename_shortcut, &QShortcut::activated, this, [this, rename_playlist] {
        rename_playlist(ui_.sidebarPlaylists->currentItem());
    });
    connect(ui_.addPlaylistButton, &QToolButton::clicked, this, [this] {
        bool ok = false;
        const auto name = QInputDialog::getText(this, tr("New playlist"), tr("Playlist name"),
            QLineEdit::Normal, QString(), &ok).trimmed();
        if (!ok || name.isEmpty()) return;
        int id = kMaxExistPlaylist;
        qDaoFacade.playlist_dao.forEachPlaylist([&id](int playlist_id, int, const QString&) { id = qMax(id, playlist_id + 1); });
        qDaoFacade.playlist_dao.addPlaylist(name, id);
        if (!qDaoFacade.playlist_dao.isPlaylistExist(id)) return;
        refreshPlaylistNavigation(id);
    });
    refreshPlaylistNavigation(kDefaultPlaylistId);
    ui_.queueButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
    ui_.queueButton->setToolTip(tr("Show queue"));
    connect(ui_.queueButton, &QToolButton::clicked, this, [this] {
        setCurrentTab(TAB_RICH_PLAYLIST);
        rich_playlist_page_->toggleQueue();
    });
    ui_.shuffleButton->setIcon(qTheme.fontIcon(Glyphs::ICON_SHUFFLE_PLAY_ORDER));
    ui_.shuffleButton->setCheckable(true);
    ui_.shuffleButton->setToolTip(tr("Shuffle"));
    connect(ui_.shuffleButton, &QToolButton::clicked, this, [this](bool checked) {
        qAppSettings.setEnumValue(kAppSettingOrder, checked ? PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM : PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
        setRepeatButtonIcon(ui_, qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder));
        playback_->refreshOrder();
    });
    connect(rich_playlist_page_.get(), &RichPlaylistPage::playOrderChanged, this, [this] {
        const auto order = qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder);
        setRepeatButtonIcon(ui_, order);
        ui_.shuffleButton->setChecked(order == PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM);
        playback_->refreshOrder();
    });
    ui_.shuffleButton->setChecked(qAppSettings.valueAsEnum<PlayerOrder>(kAppSettingOrder) == PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM);
    connect(playback_->adapter().get(), &UIPlayerStateAdapter::stateChanged, this, [this] {
        ui_.volumeSlider->setEnabled(!player_->isHardwareControlVolume());
    });

}
