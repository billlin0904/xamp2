#include <widget/albumview.h>

#include <widget/widget_shared.h>
#include <widget/scrolllabel.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlistpage.h>
#include <widget/appsettingnames.h>
#include <widget/processindicator.h>
#include <widget/util/str_util.h>
#include <widget/util/image_util.h>
#include <widget/imagecache.h>
#include <widget/util/ui_util.h>
#include <widget/xprogressdialog.h>
#include <widget/playlistentity.h>

#include <widget/dao/albumdao.h>
#include <widget/dao/musicdao.h>
#include <widget/dao/playlistdao.h>
#include <widget/dao/artistdao.h>

#include <base/scopeguard.h>

#include <thememanager.h>

#include <QGraphicsOpacityEffect>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QClipboard>
#include <QStandardPaths>
#include <QFileDialog>
#include <QPainterPath>
#include <QSqlError>
#include <QApplication>
#include <QHeaderView>

AlbumViewPage::AlbumViewPage(QWidget* parent)
    : QFrame(parent) {
    setObjectName(qTEXT("albumViewPage"));
    setFrameStyle(QFrame::StyledPanel);

    auto* default_layout = new QVBoxLayout(this);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 5, 0, 0);

    close_button_ = new QPushButton(this);
    close_button_->setObjectName(qTEXT("albumViewPageCloseButton"));
    close_button_->setCursor(Qt::PointingHandCursor);
    close_button_->setAttribute(Qt::WA_TranslucentBackground);
    close_button_->setFixedSize(qTheme.titleButtonIconSize() * 1.5);
    close_button_->setIconSize(qTheme.titleButtonIconSize() * 1.5);
    close_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW, qTheme.themeColor()));

    auto* hbox_layout = new QHBoxLayout();
    hbox_layout->setSpacing(0);
    hbox_layout->setContentsMargins(10, 10, 0, 0);

    auto* button_spacer = new QSpacerItem(20, 5, QSizePolicy::Expanding, QSizePolicy::Expanding);
    hbox_layout->addWidget(close_button_);
    hbox_layout->addSpacerItem(button_spacer);

    page_ = new PlaylistPage(this);
    page_->pageTitle()->hide();
    page_->playlist()->setPlaylistId(kAlbumPlaylistId, kAppSettingAlbumPlaylistColumnName);
    page_->playlist()->setOtherPlaylist(kDefaultPlaylistId);

    default_layout->addLayout(hbox_layout);
    default_layout->addWidget(page_);
    default_layout->setStretch(0, 0);
    default_layout->setStretch(1, 1);

    (void)QObject::connect(close_button_, &QPushButton::clicked, [this]() {
        emit leaveAlbumView();
    });

    page_->playlist()->disableDelete();
    page_->playlist()->disableLoadFile();

    if (qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        auto* shadow_effect = new QGraphicsDropShadowEffect();
        shadow_effect->setOffset(0, 0);
        shadow_effect->setColor(QColor(0, 0, 0, 80));
        shadow_effect->setBlurRadius(15);
        setGraphicsEffect(shadow_effect);
    }
}

void AlbumViewPage::onThemeChangedFinished(ThemeColor theme_color) {
    close_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW, theme_color));
}

void AlbumViewPage::setPlaylistMusic(const QString& album, int32_t album_id, const QString &cover_id, int32_t album_heart) {
    if (qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        setStyleSheet(qSTR(
            R"(
           QFrame#albumViewPage {
		        background-color: %1;
                border-radius: 4px;
				border: 1px solid #C9CDD0;
           }
        )"
        ).arg(qTheme.linearGradientStyle()));
    } else {
        setStyleSheet(qSTR(
            R"(
           QFrame#albumViewPage {
		        background-color: %1;
                border-radius: 4px;
           }
        )"
        ).arg(qTheme.linearGradientStyle()));
    }

    QList<int32_t> add_playlist_music_ids;

    page_->playlist()->horizontalHeader()->hide();
    page_->playlist()->setAlternatingRowColors(false);
    page_->playlist()->removeAll();

    dao::AlbumDao(qGuiDb.getDatabase()).forEachAlbumMusic(album_id,
        [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
            add_playlist_music_ids.push_back(entity.music_id);
        });

    dao::PlaylistDao(qGuiDb.getDatabase()).addMusicToPlaylist(add_playlist_music_ids,
        page_->playlist()->playlistId());

    page_->setAlbumId(album_id, album_heart);
    page_->playlist()->reload();
    page_->playlist()->disableDelete();
    page_->title()->setText(album);
    page_->onSetCoverById(cover_id);

    if (const auto album_stats = dao::AlbumDao(qGuiDb.getDatabase()).getAlbumStats(album_id)) {
        page_->format()->setText(tr("%1 Songs, %2, %3, %4")
            .arg(QString::number(album_stats.value().songs))
            .arg(formatDuration(album_stats.value().durations))
            .arg(QString::number(album_stats.value().year))
            .arg(formatBytes(album_stats.value().file_size))
        );
    }

    page_->show();
}

AlbumView::AlbumView(QWidget* parent)
    : QListView(parent)
    , refresh_cover_timer_(this)
    , page_(nullptr)
	, styled_delegate_(new AlbumViewStyledDelegate(this))
    , animation_(nullptr)
	, model_(this)
    , proxy_model_(new PlayListTableFilterProxyModel(this)) {
    proxy_model_->addFilterByColumn(ALBUM_INDEX_ALBUM);
    proxy_model_->setSourceModel(&model_);
    setModel(proxy_model_);

    setUniformItemSizes(true);
    setDragEnabled(false);
    setSelectionRectVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWrapping(true);
    setFlow(QListView::LeftToRight);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setFrameStyle(QFrame::StyledPanel);    
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(styled_delegate_);
    setAutoScroll(false);
    viewport()->setAttribute(Qt::WA_StaticContents);
    setMouseTracking(true);    
    dao::AlbumDao(qGuiDb.getDatabase()).updateAlbumSelectState(kInvalidDatabaseId, false);

    (void)QObject::connect(&model_, &QAbstractTableModel::modelReset,
        [this] {
            while (model_.canFetchMore()) {
                model_.fetchMore();
            }
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::showAlbumMenu, [this](auto index, auto pt) {
        showMenu(pt);
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::editAlbumView, [this](auto index, auto state) {
        auto album = indexValue(index, ALBUM_INDEX_ALBUM).toString();
        auto album_id = indexValue(index, ALBUM_INDEX_ALBUM_ID).toInt();
        dao::AlbumDao(qGuiDb.getDatabase()).updateAlbumSelectState(album_id, !state);
        reload();
        });

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::enterAlbumView, [this](auto index) {
        albumViewPage();

        auto album           = indexValue(index, ALBUM_INDEX_ALBUM).toString();
        auto cover_id        = indexValue(index, ALBUM_INDEX_COVER).toString();
        auto artist          = indexValue(index, ALBUM_INDEX_ARTIST).toString();
        auto album_id        = indexValue(index, ALBUM_INDEX_ALBUM_ID).toInt();
        auto artist_id       = indexValue(index, ALBUM_INDEX_ARTIST_ID).toInt();
        auto artist_cover_id = indexValue(index, ALBUM_INDEX_ARTIST_COVER_ID).toString();
        auto album_heart     = indexValue(index, ALBUM_INDEX_ALBUM_HEART).toInt();

        const auto list_view_rect = this->rect();
        page_->setPlaylistMusic(album, album_id, cover_id, album_heart);
        page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));

        if (enable_page_) {
            page_->show();
        }

        verticalScrollBar()->hide();
        emit clickedAlbum(album, album_id, cover_id);
        }); 

    (void)QObject::connect(styled_delegate_, &AlbumViewStyledDelegate::stopRefreshCover, [this]() {
        refresh_cover_timer_.stop();
        });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        showAlbumViewMenu(pt);
    });

   verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));

    (void)QObject::connect(&refresh_cover_timer_, &QTimer::timeout, this, &AlbumView::reload);
}

void AlbumView::setPlayingAlbumId(int32_t album_id) {
    styled_delegate_->setPlayingAlbumId(album_id);
}

void AlbumView::showAlbumViewMenu(const QPoint& pt) {
	const auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    QString action_name = tr("Enter select mode");
    if (styled_delegate_->isSelectedMode()) {
        action_name = tr("Leave select move");
    }
    auto* selected_album_mode_act = action_map.addAction(action_name, [this]() {
        // Check all album state
        for (auto index = 0; index < proxy_model_->rowCount(); ++index) {            
            auto album_id = indexValue(proxy_model_->index(index, ALBUM_INDEX_ALBUM_ID), ALBUM_INDEX_ALBUM_ID).toInt();
			dao::AlbumDao(qGuiDb.getDatabase()).updateAlbumSelectState(album_id, !styled_delegate_->isSelectedMode());
        }
        styled_delegate_->setSelectedMode(!styled_delegate_->isSelectedMode());
        // Update selected album state
        reload();
        });

    action_map.addSeparator();

    XAMP_ON_SCOPE_EXIT(
        action_map.exec(pt);
    );

	const auto show_mode = dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->showModes();
    if (show_mode == SHOW_NORMAL) {
        return;
    }

    if (styled_delegate_->isSelectedMode()) {
        QString action_name;
        if (!styled_delegate_->isSelectedAll()) {
			action_name = tr("Selected all");
            styled_delegate_->setSelectedAll(true);
        }
        else {
            action_name = tr("Unselected all");
            styled_delegate_->setSelectedAll(false);
        }
        action_map.addAction(action_name, [this]() {
            TransactionScope scope([&]() {
                dao::AlbumDao album_dao(qGuiDb.getDatabase());
                if (!styled_delegate_->isSelectedAll()) {
                    Q_FOREACH(auto album_id, dao::AlbumDao(qGuiDb.getDatabase()).getSelectedAlbums()) {
                        album_dao.updateAlbumSelectState(album_id, false);
                    }
                }
                else {
                    for (auto index = 0; index < proxy_model_->rowCount(); ++index) {
                        auto album_id = indexValue(proxy_model_->index(index, ALBUM_INDEX_ALBUM_ID), ALBUM_INDEX_ALBUM_ID).toInt();
                        album_dao.updateAlbumSelectState(album_id, true);
                    }
                }
                reload();
                }
            );
            });


        auto* sub_menu = action_map.addSubMenu(tr("Add albums to playlist"));
        
        dao::PlaylistDao(qGuiDb.getDatabase()).forEachPlaylist([sub_menu, this](auto playlist_id, auto, auto store_type, auto cloud_playlist_id, auto name) {
            if (store_type == StoreType::CLOUD_STORE || store_type == StoreType::CLOUD_SEARCH_STORE) {
                return;
            }
            if (playlist_id == kAlbumPlaylistId || playlist_id == kCdPlaylistId) {
                return;
            }
            sub_menu->addAction(name, [playlist_id, this]() {
                QList<int32_t> add_playlist_music_ids;
                Q_FOREACH(auto album_id, dao::AlbumDao(qGuiDb.getDatabase()).getSelectedAlbums()) {
                    dao::AlbumDao(qGuiDb.getDatabase()).forEachAlbumMusic(album_id,
                        [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
                            add_playlist_music_ids.push_back(entity.music_id);
                        });                    
                }
                emit addPlaylist(playlist_id, add_playlist_music_ids);
                });
            });
        action_map.addAction(tr("Add albums to new playlist"), [this]() {
            QList<int32_t> add_playlist_music_ids;
            Q_FOREACH(auto album_id, dao::AlbumDao(qGuiDb.getDatabase()).getSelectedAlbums()) {
                dao::AlbumDao(qGuiDb.getDatabase()).forEachAlbumMusic(album_id,
                    [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
                        add_playlist_music_ids.push_back(entity.music_id);
                    });
            }
            emit addPlaylist(kInvalidDatabaseId, add_playlist_music_ids);
            });

        action_map.addSeparator();

        auto* remove_selected_album_act = action_map.addAction(tr("Remove selected albums"), [this]() {
			if (!qGuiDb.transaction()) {
				return;
			}

            auto process_dialog = makeProgressDialog(
                tr("Remove selected album"),
                kEmptyString,
                tr("Cancel"));
            
            process_dialog->show();

            int32_t count = 0;

            TransactionScope scope([&]() {
                dao::AlbumDao album_dao(qGuiDb.getDatabase());
                QList<int32_t> selected_albums = album_dao.getSelectedAlbums();
                Q_FOREACH(auto album_id, selected_albums) {
                    album_dao.removeAlbumArtist(album_id);
                    album_dao.removeAlbum(album_id);
                    process_dialog->setValue(count++ * 100 / selected_albums.size() + 1);
                }
                process_dialog->setValue(100);
                reload();
                });

			});

        return;
    }
    else {
        if (index.isValid()) {
            const auto album = indexValue(index, ALBUM_INDEX_ALBUM).toString();
            const auto artist = indexValue(index, ALBUM_INDEX_ARTIST).toString();

            auto* copy_album_act = action_map.addAction(tr("Copy album"), [album]() {
                QApplication::clipboard()->setText(album);
                });
            copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

            action_map.addAction(tr("Copy artist"), [artist]() {
                QApplication::clipboard()->setText(artist);
                });
        }
    }

    action_map.addSeparator();

    auto remove_album = [this]() {
        if (!model_.rowCount()) {
            return;
        }

        auto process_dialog = makeProgressDialog(
            tr("Remove all album"),
            kEmptyString,
            tr("Cancel"));

        process_dialog->show();

        TransactionScope scope([&]() {
            QList<int32_t> albums;
            dao::AlbumDao album_dao(qGuiDb.getDatabase());
            dao::ArtistDao artist_dao(qGuiDb.getDatabase());

            album_dao.forEachAlbum([&albums](auto album_id) {
                albums.push_back(album_id);
                });
            process_dialog->setRange(0, albums.size() + 1);
            int32_t count = 0;
            Q_FOREACH(const auto album_id, albums) {
                album_dao.removeAlbumArtist(album_id);
            }
            Q_FOREACH(const auto album_id, albums) {
                album_dao.removeAlbum(album_id);
                process_dialog->setValue(count++ * 100 / albums.size() + 1);
                qApp->processEvents();
            }
            artist_dao.removeAllArtist();
            process_dialog->setValue(100);
            emit removeAll();
            qImageCache.clear();
            qIconCache.Clear();
            update();
            });
    };

    auto* load_file_act = action_map.addAction(tr("Load local file"), [this]() {
        getOpenMusicFileName(this, [this](const auto& file_name) {
            append(file_name);
            });
        });
    load_file_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FILE_OPEN));

    auto* load_dir_act = action_map.addAction(tr("Load file directory"), [this]() {
        const auto dir_name = getExistingDirectory(this);
        if (dir_name.isEmpty()) {
            return;
        }
        append(dir_name);
        });
    load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER));

    action_map.addSeparator();
    auto* remove_all_album_act = action_map.addAction(tr("Remove all album"), [=]() {
        remove_album();
        });
    remove_all_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));
    auto* search_album_cover_act = action_map.addAction(tr("Search album cover"), [=]() {
        if (!index.isValid()) {
            return;
        }
        const auto album_id = indexValue(index, ALBUM_INDEX_ALBUM_ID).toInt();
        emit styled_delegate_->findAlbumCover(DatabaseCoverId(kInvalidDatabaseId, album_id));
        });
}

void AlbumView::enterEvent(QEnterEvent* event) {
    refresh_cover_timer_.stop();
}

void AlbumView::showMenu(const QPoint &pt) {
	const auto index = indexAt(pt);

    ActionMap<AlbumView> action_map(this);

    auto album           = indexValue(index, ALBUM_INDEX_ALBUM).toString();
    auto artist          = indexValue(index, ALBUM_INDEX_ARTIST).toString();
    auto album_id        = indexValue(index, ALBUM_INDEX_ALBUM_ID).toInt();
    auto artist_id       = indexValue(index, ALBUM_INDEX_ARTIST_ID).toInt();
    auto artist_cover_id = indexValue(index, ALBUM_INDEX_ARTIST_COVER_ID).toString();

    auto* sub_menu = action_map.addSubMenu(tr("Add album to playlist"));

	dao::PlaylistDao playlist_dao(qGuiDb.getDatabase());
    
    playlist_dao.forEachPlaylist([sub_menu, album_id, this](auto playlist_id, auto, auto store_type, auto cloud_playlist_id, auto name) {
        if (store_type == StoreType::CLOUD_STORE || store_type == StoreType::CLOUD_SEARCH_STORE) {
            return;
        }
        if (playlist_id == kAlbumPlaylistId || playlist_id == kCdPlaylistId) {
            return;
        }
        sub_menu->addAction(name, [playlist_id, album_id, this]() {
            dao::AlbumDao album_dao(qGuiDb.getDatabase());
            QList<int32_t> add_playlist_music_ids;
            album_dao.forEachAlbumMusic(album_id,
                [&add_playlist_music_ids](const PlayListEntity& entity) mutable {
                    add_playlist_music_ids.push_back(entity.music_id);
                });
            emit addPlaylist(playlist_id, add_playlist_music_ids);
            });
        });

    auto* copy_album_act = action_map.addAction(tr("Copy album"), [album]() {
        QApplication::clipboard()->setText(album);
    });
    copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

    action_map.addAction(tr("Copy artist"), [artist]() {
        QApplication::clipboard()->setText(artist);
    });

    action_map.addSeparator();

    const auto remove_select_album_act = action_map.addAction(tr("Remove select album"), [album_id, this]() {
        dao::AlbumDao album_dao(qGuiDb.getDatabase());
		TransactionScope scope([&]() {
            album_dao.removeAlbumArtist(album_id);
            album_dao.removeAlbum(album_id);
            reload();
			});
    });
    remove_select_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

    action_map.exec(pt);
}

void AlbumView::enablePage(bool enable) {
    enable_page_ = enable;
    styled_delegate_->enableAlbumView(enable);
}

void AlbumView::onThemeColorChanged(QColor backgroundColor, QColor color) {
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->setAlbumTextColor(color);
}

void AlbumView::setShowMode(ShowModes mode) {
    dynamic_cast<AlbumViewStyledDelegate*>(itemDelegate())->setShowMode(mode);
}

void AlbumView::filterByArtistId(int32_t artist_id) {
    last_query_ = qSTR(R"(
    SELECT
        album,
        albums.coverId,
        artist,
        albums.albumId,
        artists.artistId,
        artists.coverId as artistCover,
        albums.year,
        albums.heart,
		albums.isHiRes,
        albums.isSelected
    FROM
        albumArtist
    LEFT JOIN
        albums ON albums.albumId = albumArtist.albumId
    LEFT JOIN
        artists ON artists.artistId = albumArtist.artistId
    WHERE
        (artists.artistId = %1) AND albums.storeType != -3
	GROUP BY 
		albums.albumId
    ORDER BY
        albums.year DESC
    )").arg(artist_id);
    setShowMode(SHOW_YEAR);
}

void AlbumView::filterCategories(const QString & category) {
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart,
	albums.isHiRes,
    albums.isSelected
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
LEFT 
	JOIN albumCategories ON albumCategories.albumId = albums.albumId
WHERE 
	albumCategories.category = '%1' AND albums.storeType != -3
ORDER BY
    albums.album DESC
    )").arg(category);
    setShowMode(SHOW_ARTIST);
}

void AlbumView::sortYears() {
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart,
	albums.isHiRes,
    albums.isSelected
FROM
    albums
LEFT JOIN
	artists ON artists.artistId = albums.artistId
WHERE
	albums.year > 0 AND albums.storeType != -3
ORDER BY
    albums.year DESC
    )");
    setShowMode(SHOW_ARTIST);
}

void AlbumView::filterYears(const QSet<QString>& years) {
    QStringList year_list;
    Q_FOREACH(auto & c, years) {
        year_list.append(qSTR("'%1'").arg(c));
    }
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart,
	albums.isHiRes,
    albums.isSelected
FROM
    albums
LEFT JOIN
	artists ON artists.artistId = albums.artistId
WHERE 
	albums.year IN (%1) AND albums.storeType != -3
ORDER BY
    albums.album DESC
    )").arg(year_list.join(qTEXT(",")));
    setShowMode(SHOW_ARTIST);
}

void AlbumView::filterRecentPlays() {
    last_query_ = qTEXT(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart,
	albums.isHiRes,
    albums.isSelected
FROM
    albums
LEFT JOIN
	artists ON artists.artistId = albums.artistId
WHERE
    albums.plays > 0 AND albums.storeType != -3
ORDER BY
    albums.plays DESC
LIMIT 5
    )");
    setShowMode(SHOW_ARTIST);
}

void AlbumView::filterCategories(const QSet<QString>& category) {
    QStringList categories;
    Q_FOREACH(auto & c, category) {
        categories.append(qSTR("'%1'").arg(c));
	}
    last_query_ = qSTR(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart,
	albums.isHiRes,
    albums.isSelected
FROM
    albums
LEFT JOIN
	artists ON artists.artistId = albums.artistId
LEFT JOIN
	albumCategories ON albumCategories.albumId = albums.albumId
WHERE 
	albumCategories.category IN (%1) AND albums.storeType != -3
GROUP BY
    albums.album
    )").arg(categories.join(qTEXT(",")));
    setShowMode(SHOW_ARTIST);
}

void AlbumView::showAll() {
    // Note: If the sql use order by, refresh cover will be slow.
    // Because the view will be refresh many times.
    last_query_ = qTEXT(R"(
SELECT
    albums.album,
    albums.coverId,
    artists.artist,
    albums.albumId,
    artists.artistId,
    artists.coverId as artistCover,
    albums.year,
    albums.heart,
	albums.isHiRes,
    albums.isSelected
FROM
    albums
LEFT JOIN
	artists ON artists.artistId = albums.artistId
WHERE
    albums.storeType != -3
    )");   
    setShowMode(SHOW_ARTIST);
}

void AlbumView::search(const QString& keyword) {
    const QRegularExpression reg_exp(keyword, QRegularExpression::CaseInsensitiveOption);
    proxy_model_->addFilterByColumn(ALBUM_INDEX_ALBUM);
    proxy_model_->addFilterByColumn(ALBUM_INDEX_ARTIST);
    proxy_model_->setFilterRegularExpression(reg_exp);
    setShowMode(SHOW_ARTIST);
}

AlbumViewPage* AlbumView::albumViewPage() {
    if (!page_) {
        page_ = new AlbumViewPage(this);
        page_->hide();

        const auto list_view_rect = this->rect();
        page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));

        (void)QObject::connect(page_,
            &AlbumViewPage::clickedArtist,
            this,
            &AlbumView::clickedArtist);
        (void)QObject::connect(page_->playlistPage()->playlist(),
            &PlayListTableView::updatePlayingState,
            this,
            [this](auto entity, auto playing_state) {
                styled_delegate_->setPlayingAlbumId(playing_state == PlayingState::PLAY_CLEAR ? -1 : entity.album_id);
            });
        (void)QObject::connect(page_, &AlbumViewPage::leaveAlbumView, [this]() {
            verticalScrollBar()->show();
            page_->hide();
            });
        (void)QObject::connect(verticalScrollBar(), &QAbstractSlider::valueChanged, [this](auto value) {
            if (value == verticalScrollBar()->maximum()) {
                model_.fetchMore();
            }
            });
    }
    return page_;
}

void AlbumView::reload() {
    if (last_query_.isEmpty()) {
        showAll();
    }
    model_.setQuery(last_query_, qGuiDb.database());
    if (model_.lastError().type() != QSqlError::NoError) {
        XAMP_LOG_DEBUG("SqlException: {}", model_.lastError().text().toStdString());
    }
}

void AlbumView::onThemeChangedFinished(ThemeColor theme_color) {
    if (page_ != nullptr) {
        page_->onThemeChangedFinished(theme_color);
    }    
}

void AlbumView::append(const QString& file_name) {
    emit extractFile(file_name, kInvalidDatabaseId);
}

void AlbumView::hideWidget() {
    if (!page_) {
        return;
    }
    page_->hide();
}

void AlbumView::resizeEvent(QResizeEvent* event) {
    if (page_ != nullptr) {
        if (!page_->isHidden()) {
            const auto list_view_rect = this->rect();
            page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));
        }
    }    
    QListView::resizeEvent(event);
}

void AlbumView::refreshCover() {
    refresh_cover_timer_.start();
}
