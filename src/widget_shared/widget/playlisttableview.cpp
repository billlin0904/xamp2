﻿#include <widget/playlisttableview.h>

#include <QHeaderView>
#include <QDesktopServices>
#include <QClipboard>
#include <QScrollBar>
#include <QShortcut>
#include <QFormLayout>
#include <QLineEdit>
#include <QApplication>
#include <qevent.h>
#include <QPainter>
#include <QSqlError>
#include <QStyledItemDelegate>
#include <QElapsedTimer>

#include <base/logger_impl.h>
#include <base/assert.h>

#include <widget/playlistsqlquerytablemodel.h>
#include <widget/processindicator.h>
#include <widget/widget_shared.h>
#include <thememanager.h>

#include <widget/playlisttablemodel.h>
#include <widget/playlisttableproxymodel.h>
#include <widget/appsettings.h>
#include <widget/imagecache.h>
#include <widget/database.h>
#include <widget/util/str_util.h>
#include <widget/actionmap.h>
#include <widget/playlistentity.h>
#include <widget/util/ui_util.h>
#include <widget/fonticon.h>
#include <widget/util/zib_util.h>
#include <widget/tagio.h>
#include <widget/encodejobwidget.h>
#include <widget/scanfileprogresspage.h>

namespace {
    QString noOrderAlbum(int32_t playlist_id) {
    	return qFormat(R"(
    SELECT	
	albums.coverId,
	musics.musicId,
	playlistMusics.playing,
	musics.track,
	musics.path,
	musics.fileSize,
	musics.title,
	musics.fileName,
	artists.artist,
	albums.album,
	musics.bitRate,
	musics.sampleRate,
	albumMusic.albumId,
	albumMusic.artistId,
	musics.fileExt,
	musics.parentPath,
	musics.dateTime,
	playlistMusics.playlistMusicsId,
	musics.albumReplayGain,
	musics.albumPeak,
	musics.trackReplayGain,
	musics.trackPeak,
	musicLoudness.trackLoudness,
	musics.genre,	
	playlistMusics.isChecked,
	musics.heart,
	musics.duration,
	musics.comment,
	albums.year,
	musics.coverId as musicCoverId,
    musics.offset,
    musics.isCueFile,
    musics.ytMusicAlbumId,
	musics.ytMusicArtistId,
    musics.isZipFile,
    musics.archiveEntryName
FROM
	playlistMusics
	JOIN playlist ON playlist.playlistId = playlistMusics.playlistId
	JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId
	LEFT JOIN musicLoudness ON playlistMusics.musicId = musicLoudness.musicId
	JOIN musics ON playlistMusics.musicId = musics.musicId
	JOIN albums ON albumMusic.albumId = albums.albumId
	JOIN artists ON albumMusic.artistId = artists.artistId 
WHERE
	playlistMusics.playlistId = %1)").arg(playlist_id);
        };
    }

    QString groupAlbum(int32_t playlist_id) {
        return qFormat(R"(
    SELECT	
	albums.coverId,
	musics.musicId,
	playlistMusics.playing,
	musics.track,
	musics.path,
	musics.fileSize,
	musics.title,
	musics.fileName,
	artists.artist,
	albums.album,
	musics.bitRate,
	musics.sampleRate,
	albumMusic.albumId,
	albumMusic.artistId,
	musics.fileExt,
	musics.parentPath,
	musics.dateTime,
	playlistMusics.playlistMusicsId,
	musics.albumReplayGain,
	musics.albumPeak,
	musics.trackReplayGain,
	musics.trackPeak,
	musicLoudness.trackLoudness,
	musics.genre,	
	playlistMusics.isChecked,
	musics.heart,
	musics.duration,
	musics.comment,
	albums.year,
	musics.coverId as musicCoverId,
    musics.offset,
    musics.isCueFile,
	musics.ytMusicAlbumId,
	musics.ytMusicArtistId,
    musics.isZipFile,
    musics.archiveEntryName
FROM
	playlistMusics
	JOIN playlist ON playlist.playlistId = playlistMusics.playlistId
	JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId
	LEFT JOIN musicLoudness ON playlistMusics.musicId = musicLoudness.musicId
	JOIN musics ON playlistMusics.musicId = musics.musicId
	JOIN albums ON albumMusic.albumId = albums.albumId
	JOIN artists ON albumMusic.artistId = artists.artistId 
WHERE
	playlistMusics.playlistId = %1 
ORDER BY
	albums.album)").arg(playlist_id);
    }

    QString groupNone(int32_t playlist_id) {
        return qFormat(R"(
    SELECT
	albums.coverId,
	musics.musicId,
	playlistMusics.playing,
	musics.track,
	musics.path,
	musics.fileSize,
	musics.title,
	musics.fileName,
	artists.artist,
	albums.album,
	musics.bitRate,
	musics.sampleRate,
	albumMusic.albumId,
	albumMusic.artistId,
	musics.fileExt,
	musics.parentPath,
	musics.dateTime,
	playlistMusics.playlistMusicsId,
	musics.albumReplayGain,
	musics.albumPeak,
	musics.trackReplayGain,
	musics.trackPeak,
	musicLoudness.trackLoudness,
	musics.genre,	
	playlistMusics.isChecked,
	musics.heart,
	musics.duration,
	musics.comment,
	albums.year,
	musics.coverId as musicCoverId,
    musics.offset,
	musics.isCueFile,
	musics.ytMusicAlbumId,
	musics.ytMusicArtistId,
    musics.isZipFile,
    musics.archiveEntryName
FROM
	playlistMusics
	JOIN playlist ON playlist.playlistId = playlistMusics.playlistId
	JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId
	LEFT JOIN musicLoudness ON playlistMusics.musicId = musicLoudness.musicId
	JOIN musics ON playlistMusics.musicId = musics.musicId
	JOIN albums ON albumMusic.albumId = albums.albumId
	JOIN artists ON albumMusic.artistId = artists.artistId 
WHERE
	playlistMusics.playlistId = %1)").arg(playlist_id);
}

void PlaylistTableView::search(const QString& keyword) const {
	const QRegularExpression reg_exp(keyword, QRegularExpression::CaseInsensitiveOption);
    proxy_model_->addFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->addFilterByColumn(PLAYLIST_ARTIST);
    proxy_model_->addFilterByColumn(PLAYLIST_ALBUM);
    proxy_model_->setFilterRegularExpression(reg_exp);
}

PlaylistStyledItemDelegate* PlaylistTableView::styledDelegate() {
    return dynamic_cast<PlaylistStyledItemDelegate*>(itemDelegate());
}

void PlaylistTableView::moveUp() {
    auto selected_indexes = selectionModel()->selectedRows();
    if (selected_indexes.isEmpty()) return;

    // 將選取索引依顯示列排序（升冪）
    std::sort(selected_indexes.begin(), selected_indexes.end(), [](const QModelIndex& a, const QModelIndex& b) {
        return a.row() < b.row();
        });

    // 在執行任何交換動作前，先取得所有 source row，避免中途re-map引發混亂
    std::vector<int> source_rows;
    source_rows.reserve(selected_indexes.size());
    for (auto& index : selected_indexes) {
        source_rows.push_back(proxy_model_->mapToSource(index).row());
    }

    // 若最上面的元素已在最上方(row=0)，無法再上移
    if (source_rows.front() == 0) {
        return;
    }

    // 若有多個項目被移動，取得第一個項目前一行的entity，以及最後一個項目行的index以便之後updateIndex
    std::optional<int> last_source_row_opt;
    std::optional<PlayListEntity> first_entity_opt;

    if (source_rows.size() > 1) {
        int first_source_row = source_rows.front();
        auto first_entity = item(model_->index(first_source_row - 1, 0));
        int last_source_row = source_rows.back();

        last_source_row_opt = last_source_row;
        first_entity_opt = first_entity;
    }

    // 依照 source_rows 中的順序一一上移（從上到下處理不會干擾後續行）
    for (auto row : source_rows) {
        swapPositions(row, row - 1);
    }

    // 若有記錄first_entity與last_source_row，更新之
    if (last_source_row_opt && first_entity_opt) {
        updateIndex(last_source_row_opt.value(), first_entity_opt.value());
    }

    // 再次選取移動後的新位置
    selectMovedRows(selected_indexes, -1);
}

void PlaylistTableView::moveDown() {
    auto selected_indexes = selectionModel()->selectedRows();
    if (selected_indexes.isEmpty()) {
        return;
    }

    // 將選取的列以顯示列(row)升冪排序
    std::sort(selected_indexes.begin(), selected_indexes.end(), [](const QModelIndex& a, const QModelIndex& b) {
        return a.row() < b.row();
        });

    // 將選取列的 source row 一次記錄下來
    std::vector<int> source_rows;
    source_rows.reserve(selected_indexes.size());
    for (auto& index : selected_indexes) {
        source_rows.push_back(proxy_model_->mapToSource(index).row());
    }

    // 若底部列已在最後一行，無法往下移
    if (source_rows.back() == model_->rowCount() - 1) {
        return;
    }

    // 處理更新用的變數
    std::optional<int> first_source_row_opt;
    std::optional<PlayListEntity> last_entity_opt;

    // 若有多個項目被選取，抓取範圍最上層項目的下一行資料
    if (source_rows.size() > 1) {
        int last_source_row_of_selection = source_rows.back();
        // 往下移之後,最上面的項目將影響上一行的參考
        auto last_entity = item(model_->index(last_source_row_of_selection + 1, 0));
        first_source_row_opt = source_rows.front();
        last_entity_opt = last_entity;
    }

    // 開始從「底部」往「上」依序往下移行，避免順序錯亂
    // 如有選取多行 (1,2)，會先處理 row=2 -> swap(2,3)，再處理 row=1 -> swap(1,2)
    for (auto it = source_rows.rbegin(); it != source_rows.rend(); ++it) {
        int row = *it;
        swapPositions(row, row + 1);
    }

    if (first_source_row_opt && last_entity_opt) {
        updateIndex(first_source_row_opt.value(), last_entity_opt.value());
    }

    selectMovedRows(selected_indexes, 1);
}

void PlaylistTableView::updateIndex(int index, const PlayListEntity& entity) {
    const auto target = item(model_->index(index, 0));
    auto [playing1, is_checked1] = qDaoFacade.playlist_dao.getPlaylistMusic(playlist_id_, entity.playlist_music_id);
    qDaoFacade.playlist_dao.updatePlaylistMusic(target.playlist_music_id, entity.music_id, entity.album_id, playing1, is_checked1);
}

void PlaylistTableView::swapPositions(int row1, int row2) {
	const auto item1 = item(model_->index(row1, 0));
    const auto item2 = item(model_->index(row2, 0));
    qDaoFacade.playlist_dao.swapPlaylistMusicId(playlist_id_, item1, item2);
}

void PlaylistTableView::selectMovedRows(const QModelIndexList& selectedIndexes, int direction) {
    selectionModel()->clearSelection();

    for (const QModelIndex& index : selectedIndexes) {
        int new_row = index.row() + direction;
        QModelIndex newIndex = proxy_model_->mapFromSource(model_->index(new_row, 0));
        selectionModel()->select(newIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    proxy_model_->invalidate();
    reload();
}

void PlaylistTableView::reload(bool is_scroll_to, bool order_by) {
    std::optional<PlayListEntity> entity;
    if (play_index_.isValid()) {
        entity.emplace(item(play_index_));
    }

    QString query_string;

    if (order_by) {
        if (group_ != PLAYLIST_GROUP_ALBUM) {
            query_string = groupNone(playlist_id_);
        }
        else {
            query_string = groupAlbum(playlist_id_);
        }
    }
    else {
        query_string = noOrderAlbum(playlist_id_);
    }

    const QSqlQuery query(query_string, qGuiDb.database());
    model_->setQuery(query);
    if (model_->lastError().type() != QSqlError::NoError) {
        XAMP_LOG_DEBUG("SqlException: {}", model_->lastError().text().toStdString());
    }

    // NOTE: 呼叫此函數就會更新index, 會導致playing index失效    
    model_->dataChanged(QModelIndex(), QModelIndex());

    // NOTE: playing index失效必須要重新尋找playlist_music_id
    if (entity) {
        for (auto i = 0; i < proxy_model_->rowCount(); ++i) {
            auto temp = item(proxy_model_->index(i, 0));
            if (temp.playlist_music_id == entity->playlist_music_id) {
                play_index_ = proxy_model_->index(i, 0);
                scrollToIndex(play_index_);
                update();
                return;
            }
        }
    }

    play_index_ = QModelIndex();
    album_songs_id_cache_.clear();
    update();
}

PlaylistTableView::PlaylistTableView(QWidget* parent, int32_t playlist_id)
    : QTableView(parent)
    , playlist_id_(playlist_id)
    , model_(new PlayListSqlQueryTableModel(this))
	, proxy_model_(new PlayListTableFilterProxyModel(this)) {
    initial();
}

PlaylistTableView::~PlaylistTableView() = default;

void PlaylistTableView::enableCloudMode(bool mode) {
    cloud_mode_ = mode;
}

void PlaylistTableView::setPlaylistId(const int32_t playlist_id, const QString &column_setting_name) {
    playlist_id_ = playlist_id;
    column_setting_name_ = column_setting_name;

    qDaoFacade.playlist_dao.clearNowPlaying(playlist_id_);

    reload();

    model_->setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("Id"));
    model_->setHeaderData(PLAYLIST_IS_PLAYING, Qt::Horizontal, tr("IsPlaying"));
    model_->setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, tr("   #"));
    model_->setHeaderData(PLAYLIST_FILE_PATH, Qt::Horizontal, tr("FilePath"));
    model_->setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, tr("Title"));
    model_->setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, tr("FileName"));
    model_->setHeaderData(PLAYLIST_FILE_SIZE, Qt::Horizontal, tr("FileSize"));
    model_->setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, tr("Album"));
    model_->setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, tr("Artist"));
    model_->setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, tr("Duration"));
    model_->setHeaderData(PLAYLIST_BIT_RATE, Qt::Horizontal, tr("BitRate"));
    model_->setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, tr("SampleRate"));
    model_->setHeaderData(PLAYLIST_ALBUM_RG, Qt::Horizontal, tr("AlbumRG"));
    model_->setHeaderData(PLAYLIST_ALBUM_PK, Qt::Horizontal, tr("AlbumPK"));
    model_->setHeaderData(PLAYLIST_LAST_UPDATE_TIME, Qt::Horizontal, tr("LastUpdateTime"));
    model_->setHeaderData(PLAYLIST_TRACK_RG, Qt::Horizontal, tr("TrackRG"));
    model_->setHeaderData(PLAYLIST_TRACK_PK, Qt::Horizontal, tr("TrackPK"));
    model_->setHeaderData(PLAYLIST_TRACK_LOUDNESS, Qt::Horizontal, tr("Loudness"));
    model_->setHeaderData(PLAYLIST_GENRE, Qt::Horizontal, tr("Genre"));
    model_->setHeaderData(PLAYLIST_ALBUM_ID, Qt::Horizontal, tr("AlbumId"));
    model_->setHeaderData(PLAYLIST_PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("PlaylistId"));
    model_->setHeaderData(PLAYLIST_FILE_EXT, Qt::Horizontal, tr("FileExt"));
    model_->setHeaderData(PLAYLIST_FILE_PARENT_PATH, Qt::Horizontal, tr("ParentPath"));
    model_->setHeaderData(PLAYLIST_ALBUM_COVER_ID, Qt::Horizontal, tr(""));
    model_->setHeaderData(PLAYLIST_ARTIST_ID, Qt::Horizontal, tr("ArtistId"));
    model_->setHeaderData(PLAYLIST_LIKE, Qt::Horizontal, tr("Like"));
    model_->setHeaderData(PLAYLIST_COMMENT, Qt::Horizontal, tr("Comment"));
    model_->setHeaderData(PLAYLIST_YEAR, Qt::Horizontal, tr("Year"));
    model_->setHeaderData(PLAYLIST_CHECKED, Qt::Horizontal, tr(""));

    auto column_list = qAppSettings.valueAsStringList(column_setting_name);

    const QList<int> always_hidden_columns{
        PLAYLIST_MUSIC_ID,
        PLAYLIST_IS_PLAYING,
        PLAYLIST_PLAYLIST_MUSIC_ID,
        PLAYLIST_CHECKED,
        PLAYLIST_ALBUM_ID,
        PLAYLIST_MUSIC_COVER_ID,
        PLAYLIST_OFFSET,
        PLAYLIST_IS_CUE_FILE,
        PLAYLIST_IS_ZIP_FILE,
        PLAYLIST_ARCHVI_ENTRY_NAME
    };

    always_hidden_columns_ = always_hidden_columns;

    for (const auto column : std::as_const(always_hidden_columns_)) {
        hideColumn(column);
    }

    if (column_list.empty()) {
        const QList<int> hidden_columns{
            PLAYLIST_MUSIC_ID,
            PLAYLIST_IS_PLAYING,
            PLAYLIST_FILE_PATH,
            PLAYLIST_FILE_NAME,
            PLAYLIST_FILE_SIZE,
            PLAYLIST_ALBUM,
            PLAYLIST_ARTIST,
            PLAYLIST_BIT_RATE,
            PLAYLIST_SAMPLE_RATE,
            PLAYLIST_ALBUM_RG,
            PLAYLIST_ALBUM_PK,
            PLAYLIST_LAST_UPDATE_TIME,
            PLAYLIST_TRACK_RG,
            PLAYLIST_TRACK_PK,
            PLAYLIST_TRACK_LOUDNESS,
            PLAYLIST_GENRE,
            PLAYLIST_ALBUM_ID,
            PLAYLIST_ARTIST_ID,
            PLAYLIST_FILE_EXT,
            PLAYLIST_FILE_PARENT_PATH,
            PLAYLIST_PLAYLIST_MUSIC_ID,
            PLAYLIST_COMMENT,
            PLAYLIST_LIKE,
            PLAYLIST_YEAR,
            PLAYLIST_OFFSET,
            PLAYLIST_YT_MUSIC_ALBUM_ID,
            PLAYLIST_YT_MUSIC_ARIST_ID,
            PLAYLIST_IS_ZIP_FILE,
            PLAYLIST_ARCHVI_ENTRY_NAME
        };

        for (const auto column : qAsConst(hidden_columns)) {
            hideColumn(column);
        }

        if (column_setting_name_.isEmpty()) {
            return;
        }

        for (auto column = 0; column < horizontalHeader()->count(); ++column) {
            if (!isColumnHidden(column)) {
                qAppSettings.addList(column_setting_name_, QString::number(column));
            } else {
                setColumnHidden(column, true);
            }
        }
    } else {
        for (auto column = 0; column < horizontalHeader()->count(); ++column) {
            setColumnHidden(column, true);
        }
        Q_FOREACH(auto column, column_list) {
            setColumnHidden(column.toInt(), false);
        }
    }
}

void PlaylistTableView::setHeaderViewHidden(bool enable) {
	if (enable) {
        horizontalHeader()->hide();
        horizontalHeader()->setContextMenuPolicy(Qt::NoContextMenu);
        return;
	}

    horizontalHeader()->show();
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlaylistTableView> action_map(this);

    auto* header_view = horizontalHeader();

    auto last_referred_logical_column = header_view->logicalIndexAt(pt);
    auto hide_this_column_act = action_map.addAction(tr("Hide this column"), [last_referred_logical_column, this]() {
        setColumnHidden(last_referred_logical_column, true);
		qAppSettings.removeList(column_setting_name_, QString::number(last_referred_logical_column));
        });
    hide_this_column_act->setIcon(qTheme.fontIcon(Glyphs::ICON_HIDE));

    auto select_column_show_act = action_map.addAction(tr("Select columns to show..."), [pt, header_view, this]() {
        ActionMap<PlaylistTableView> action_map(this);
        for (auto column = 0; column < header_view->count(); ++column) {
            if (always_hidden_columns_.contains(column)) {
                continue;
            }
            auto header_name = model()->headerData(column, Qt::Horizontal).toString();
            action_map.addAction(header_name, [this, column]() {
                setColumnHidden(column, false);
			    qAppSettings.addList(column_setting_name_, QString::number(column));
            }, false, !isColumnHidden(column));
        }
        action_map.exec(pt);
    });

    select_column_show_act->setIcon(qTheme.fontIcon(Glyphs::ICON_SHOW));
    action_map.exec(pt);
    });
}

void PlaylistTableView::initial() {
    proxy_model_->addFilterByColumn(PLAYLIST_ARTIST);
    proxy_model_->addFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->addFilterByColumn(PLAYLIST_ALBUM);
    proxy_model_->setSourceModel(model_);
    setModel(proxy_model_);

    progress_page_ = new ScanFileProgressPage(this);
    progress_page_->hide();
    progress_page_->move(0, height() - 80);

    setTabViewStyle(this);
    verticalHeader()->setDefaultSectionSize(kColumnHeight);
    setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);

    installEventFilter(this);
    setItemDelegate(new PlaylistStyledItemDelegate(this));

    (void)QObject::connect(model_, &QAbstractTableModel::modelReset,
    [this] {
        while (model_->canFetchMore()) {
            model_->fetchMore();
        }
    });

    (void)QObject::connect(this, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        if (model_->rowCount() > 0) {
            playItem(index);
        }
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);
		
        ActionMap<PlaylistTableView> action_map(this);

        PlayListEntity entity;
        if (model_->rowCount() > 0 && index.isValid()) {
            entity = getEntity(index);
        }             

        if (cloud_mode_) {
            auto* copy_album_act = action_map.addAction(tr("Copy album"));
            copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

            auto* copy_artist_act = action_map.addAction(tr("Copy artist"));
            auto* copy_title_act = action_map.addAction(tr("Copy title"));

            action_map.setCallback(copy_album_act, [entity]() {
                QApplication::clipboard()->setText(entity.album);
                });
            action_map.setCallback(copy_artist_act, [entity]() {
                QApplication::clipboard()->setText(entity.artist);
                });
            action_map.setCallback(copy_title_act, [entity]() {
                QApplication::clipboard()->setText(entity.title);
                });

            action_map.addSeparator();

            const auto rows = selectItemIndex();
            std::vector<std::string> video_ids;
            std::vector<PlayListEntity> play_list_entities;
            video_ids.reserve(rows.size());
            play_list_entities.reserve(rows.size());
            for (const auto& row : rows) {
                const auto play_list_entity = this->item(row.second);
                video_ids.push_back(play_list_entity.file_path.toStdString());
                play_list_entities.push_back(play_list_entity);
            }

            auto* remove_select_cloud_music_act = action_map.addAction(tr("Remove select music"));
            action_map.setCallback(remove_select_cloud_music_act, [this, video_ids]() {
                emit removePlaylistItems(cloud_playlist_id_.value(), video_ids);
            });

            QString menu_name;
            QIcon like_icon;
            PlayListEntity play_list_entity;
            if (!play_list_entities.empty()) {
                play_list_entity = play_list_entities.front();
                if (play_list_entity.heart) {
                    menu_name = tr("DisLike the music");
                    like_icon = qTheme.fontIcon(Glyphs::ICON_DISLIKE);
                }
                else {
                    menu_name = tr("Like the music");
                    like_icon = qTheme.fontIcon(Glyphs::ICON_LIKE);
                }
            } else {
                menu_name = tr("Like the music");
                like_icon = qTheme.fontIcon(Glyphs::ICON_LIKE);
            }
            auto* like_song_act = action_map.addAction(menu_name);
            like_song_act->setIcon(like_icon);
            if (!play_list_entities.empty()) {
                action_map.setCallback(like_song_act, [this, &play_list_entity, play_list_entities]() {
                    for (const auto &entity : play_list_entities) {
                        emit likeSong(play_list_entity.heart, entity);
                    }                    
                    });
            }
            
            auto* remove_all_act = action_map.addAction(tr("Remove all"));
            remove_all_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

            action_map.setCallback(remove_all_act, [this]() {
                if (!model_->rowCount()) {
                    return;
                }

                qDaoFacade.playlist_dao.removePlaylistAllMusic(playlistId());
            	reload();
                removePlaying();
                });

            if (navigation_view_mode_ != NavigationViewMode::NAVIGATION_VIEW_NONE) {
                action_map.addSeparator();

                auto* navigate_to_album_page = action_map.addAction("Navigate To album"_str);
                action_map.setCallback(navigate_to_album_page, [this, entity]() {
                    emit navigateToAlbumPage(entity);
                    });
                if (navigation_view_mode_ == NavigationViewMode::NAVIGATION_VIEW_ALBUM_AND_ARTIST) {
                    auto* navigate_to_artist_page = action_map.addAction("Navigate To artist"_str);
                    action_map.setCallback(navigate_to_artist_page, [this, entity]() {
                        emit navigateToArtistPage(entity.artist_id, entity.yt_music_artist_id);
                        });
                }
            }

            if (model_->rowCount() > 0 && index.isValid()) {
                XAMP_TRY_LOG(
                    action_map.exec(pt);
                    );
            }
            return;
        }

        auto* load_file_act = action_map.addAction(tr("Load local file"), [this]() {
            getOpenMusicFileName(this, tr("Open file"), tr("Music Files "), [this](const auto& file_name) {
                append(file_name);
                });
            });
        load_file_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FILE_OPEN));

        auto* load_dir_act = action_map.addAction(tr("Load file directory"), [this]() {
            const auto dir_name = getExistingDirectory(this, tr("Select a directory"));
            if (dir_name.isEmpty()) {
                return;
            }
            showProgressPage();
            append(dir_name);
            });
        load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER_OPEN));

        if (enable_delete_ && model()->rowCount() > 0) {            
            auto* remove_all_act = action_map.addAction(tr("Remove all"));
            remove_all_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

            action_map.setCallback(remove_all_act, [this]() {
                if (!model_->rowCount()) {
                    return;
                }
                
                qDaoFacade.playlist_dao.removePlaylistAllMusic(playlistId());
                reload();
                removePlaying();
            });
        }       

        action_map.addSeparator();

        auto* add_music_to_new_playlist_menu = action_map.addSubMenu(tr("Add music to new playlist"));
        qDaoFacade.playlist_dao.forEachPlaylist([add_music_to_new_playlist_menu, this](auto playlist_id, auto, auto store_type, auto cloud_playlist_id, auto name) {
			if (notAddablePlaylist(playlist_id)) {
				return;
			}
            add_music_to_new_playlist_menu->addAction(name, [playlist_id, this]() {
                emit addPlaylist(playlist_id, items());
                });
            });

        action_map.addSeparator();
        auto * copy_album_act = action_map.addAction(tr("Copy album"));
        copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

        auto * copy_artist_act = action_map.addAction(tr("Copy artist"));
        auto * copy_title_act = action_map.addAction(tr("Copy title"));

        if (model_->rowCount() == 0 || !index.isValid()) {
            XAMP_TRY_LOG(
                action_map.exec(pt);
                );
            return;
        }

        action_map.addSeparator();

        auto* encode_pcm_file_act = action_map.addAction(tr("Encode to PCM File"));
        action_map.setCallback(encode_pcm_file_act, [this]() {
            const auto rows = selectItemIndex();
            QList<PlayListEntity> entities;
            for (const auto& row : rows) {
                const auto entity = this->item(row.second);
                entities.push_back(entity);
            }
            emit encodeAlacFiles(EncodeType::ENCODE_PCM, entities);
            });

        auto* encode_alac_file_act = action_map.addAction(tr("Encode to ALAC File"));
        action_map.setCallback(encode_alac_file_act, [this]() {
            const auto rows = selectItemIndex();
            QList<PlayListEntity> entities;
            for (const auto& row : rows) {
                const auto entity = this->item(row.second);
                entities.push_back(entity);
            }
            emit encodeAlacFiles(EncodeType::ENCODE_ALAC, entities);
            });

        auto* encode_aac_file_act = action_map.addAction(tr("Encode to AAC File (256Kbps)"));
        action_map.setCallback(encode_aac_file_act, [this]() {
            const auto rows = selectItemIndex();
            QList<PlayListEntity> entities;
            for (const auto& row : rows) {
                const auto entity = this->item(row.second);
                entities.push_back(entity);
            }
            emit encodeAlacFiles(EncodeType::ENCODE_AAC, entities);
            });

        action_map.addSeparator();

        auto* move_up_select_item_act = action_map.addAction(tr("Move up select items"));
        action_map.setCallback(move_up_select_item_act, [this]() {
			moveUp();
            });

        auto* move_down_select_item_act = action_map.addAction(tr("Move down select items"));
        action_map.setCallback(move_down_select_item_act, [this]() {
            moveDown();
            });

        action_map.addSeparator();

        auto* scan_select_item_replay_gain_act = action_map.addAction(tr("Scan EBU R128 select items"));
        action_map.setCallback(scan_select_item_replay_gain_act, [this]() {
            QList<PlayListEntity> entities;
            const auto rows = selectItemIndex();
            for (const auto& row : rows) {
                const auto entity = this->item(row.second);
                entities.push_back(entity);
            }
            showProgressPage();
            emit scanReplayGain(entities);
            });

        action_map.addSeparator();

        auto* add_to_playlist_act = action_map.addAction(tr("Add file to playlist"));
        add_to_playlist_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FILE_CIRCLE_PLUS));
        action_map.setCallback(add_to_playlist_act, [this]() {
            if (const auto other_playlist_id = other_playlist_id_) {
                const auto rows = selectItemIndex();
                for (const auto& row : rows) {
	                const auto entity = this->item(row.second);
                    qDaoFacade.playlist_dao.addMusicToPlaylist(entity.music_id, other_playlist_id.value(), entity.album_id);
                }
            }
            });

        auto* select_item_edit_track_info_act = action_map.addAction(tr("Edit track information"));
        select_item_edit_track_info_act->setIcon(qTheme.fontIcon(Glyphs::ICON_EDIT));

        auto reload_track_info_act = action_map.addAction(tr("Reload track information"));
        reload_track_info_act->setIcon(qTheme.fontIcon(Glyphs::ICON_RELOAD));

        auto* open_local_file_path_act = action_map.addAction(tr("Open local file path"));
        open_local_file_path_act->setIcon(qTheme.fontIcon(Glyphs::ICON_OPEN_FILE_PATH));
        action_map.setCallback(open_local_file_path_act, [entity]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(entity.parent_path));
        });

        action_map.setCallback(reload_track_info_act, [this, entity]() {
            onReloadEntity(entity);
        });
    	
        action_map.addSeparator();
        action_map.setCallback(copy_album_act, [entity]() {
            QApplication::clipboard()->setText(entity.album);
        });
        action_map.setCallback(copy_artist_act, [entity]() {
            QApplication::clipboard()->setText(entity.artist);
        });
        action_map.setCallback(copy_title_act, [entity]() {
            QApplication::clipboard()->setText(entity.title);
        });

        action_map.setCallback(select_item_edit_track_info_act, [this]() {
            const auto rows = selectItemIndex();
            QList<PlayListEntity> entities;
            for (const auto& row : rows) {
                entities.push_front(this->item(row.second));
            }
            if (entities.isEmpty()) {
                return;
            }
            emit editTags(playlistId(), entities);
            Q_FOREACH(auto play_list_entity, entities) {
                onReloadEntity(play_list_entity);
            }
        });

        XAMP_TRY_LOG(
            action_map.exec(pt);
        );
    });

    const auto *control_A_key = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    (void) QObject::connect(control_A_key, &QShortcut::activated, [this]() {
        selectAll();
    });

    // note: Fix QTableView select color issue.
    setFocusPolicy(Qt::StrongFocus);
}

void PlaylistTableView::showProgressPage() {
    progress_page_->show();
    const auto list_view_rect = this->rect();
    progress_page_->setFixedSize(QSize(list_view_rect.size().width() - 2, 80));
    progress_page_->move(0, height() - 80);
}

void PlaylistTableView::onReloadEntity(const PlayListEntity& item) {
    dao::MusicDao music_dao(qGuiDb.getDatabase());

    XAMP_TRY_LOG(
        auto track_info = TagIO::getTrackInfo(item.file_path.toStdWString());
        if (track_info) {
            music_dao.addOrUpdateMusic(track_info.value());
            reload();
            play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
        }        
        );
}

void PlaylistTableView::pauseItem(const QModelIndex& index) {
    const auto entity = item(index);
    qDaoFacade.playlist_dao.setNowPlayingState(playlistId(), entity.playlist_music_id, PlayingState::PLAY_PAUSE);
    update();
}

PlayListEntity PlaylistTableView::item(const QModelIndex& index) const {
    return getEntity(index);
}

void PlaylistTableView::playItem(const QModelIndex& index) {
    setNowPlaying(index);
    if (!play_index_.isValid()) {
        return;
    }
    const auto play_item = item(play_index_);
    emit playMusic(playlistId(), play_item, true);
}

void PlaylistTableView::onRetranslateUi() {
}

void PlaylistTableView::updateReplayGain(const QList<PlayListEntity>& entities) {
    for (const auto& entity : entities) {
        if (entity.replay_gain) {
            qDaoFacade.music_dao.updateMusicReplayGain(entity.music_id,
                entity.replay_gain.value().album_gain,
                entity.replay_gain.value().album_peak,
                entity.replay_gain.value().track_gain,
                entity.replay_gain.value().track_peak);
        }
    }
    reload();
}

void PlaylistTableView::keyPressEvent(QKeyEvent *event) {
    QAbstractItemView::keyPressEvent(event);
}

bool PlaylistTableView::eventFilter(QObject* obj, QEvent* ev) {
    const auto type = ev->type();
    if (this == obj && type == QEvent::KeyPress) {
	    const auto* event = dynamic_cast<QKeyEvent*>(ev);
        if (event->key() == Qt::Key_Delete && enable_delete_) {
            removeSelectItems();
            return true;
        }
    }
    return QAbstractItemView::eventFilter(obj, ev);
}

void PlaylistTableView::append(const QString& file_name) {
    showProgressPage();
    emit extractFile(file_name, playlistId());
}

void PlaylistTableView::onProcessDatabase(int32_t playlist_id, const QList<PlayListEntity>& entities) {
    if (playlistId() != playlist_id) {
        return;
    }

    for (const auto& entity : entities) {
        qDaoFacade.playlist_dao.addMusicToPlaylist(entity.music_id, playlistId(), entity.album_id);
    }
    reload();
    emit addPlaylistItemFinished();
}

void PlaylistTableView::onProcessTrackInfo(int32_t total_album, int32_t total_tracks) {
    reload();
    emit addPlaylistItemFinished();
}

void PlaylistTableView::resizeEvent(QResizeEvent* event) {
    QTableView::resizeEvent(event);
    resizeColumn();

    if (progress_page_ != nullptr) {
        if (!progress_page_->isHidden()) {
            const auto list_view_rect = this->rect();
            progress_page_->setFixedSize(QSize(list_view_rect.size().width() - 2, 80));
            progress_page_->move(0, height() - 80);
        }
    }
}

void PlaylistTableView::mouseMoveEvent(QMouseEvent* event) {
    QTableView::mouseMoveEvent(event);

    const auto index = indexAt(event->pos());
    const auto old_hover_row = hover_row_;
    const auto old_hover_column = hover_column_;
    hover_row_ = index.row();
    hover_column_ = index.column();

    if (selectionBehavior() == SelectRows && old_hover_row != hover_row_) {
        for (auto i = 0; i < model()->columnCount(); ++i)
            update(model()->index(hover_row_, i));
    }

    if (selectionBehavior() == SelectColumns && old_hover_column != hover_column_) {
        for (auto i = 0; i < model()->rowCount(); ++i) {
            update(model()->index(i, hover_column_));
            update(model()->index(i, old_hover_column));
        }
    }
}

void PlaylistTableView::resizeColumn() {
    auto* header = horizontalHeader();
	
    auto not_hide_column = 0;

    for (auto column = 0; column < header->count(); ++column) {
        if (!isColumnHidden(column)) {
            ++not_hide_column;
        }

	    switch (column) {
        case PLAYLIST_IS_PLAYING:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            header->resizeSection(column, kColumnPlayingWidth);
            break;
        case PLAYLIST_TRACK:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, kColumnTrackWidth);
            break;
        case PLAYLIST_TITLE:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
        	break;        
        case PLAYLIST_ARTIST:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, kColumnArtistWidth);
            break;
        case PLAYLIST_ALBUM:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        case PLAYLIST_CHECKED:
        case PLAYLIST_LIKE:
        case PLAYLIST_ALBUM_COVER_ID:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, kColumnWidth);
            break;
        default:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, kColumnDefaultWidth);
            break;
        }
    }
}

int32_t PlaylistTableView::playlistId() const {
    return playlist_id_;
}

QModelIndex PlaylistTableView::firstIndex() const {
    return model()->index(0, 0);
}

void PlaylistTableView::setOtherPlaylist(int32_t playlist_id) {
    other_playlist_id_ = playlist_id;
}

QModelIndex PlaylistTableView::nextIndex(int forward) const {
    const auto count = proxy_model_->rowCount();
    const auto play_index = currentIndex();
    const auto next_index = (play_index.row() + forward) % count;
    return model()->index(next_index, PLAYLIST_IS_PLAYING);
}

QModelIndex PlaylistTableView::shuffleIndex() {
    auto current_playlist_music_id = 0;
    if (play_index_.isValid()) {
        current_playlist_music_id = indexValue(play_index_, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    }    
    if (current_playlist_music_id != 0) {
        rng_.SetSeed(current_playlist_music_id);
    }
    const auto count = proxy_model_->rowCount();
    const auto selected = rng_.NextInt32(0, count - 1);
    return model()->index(selected, PLAYLIST_IS_PLAYING);
}

QModelIndex PlaylistTableView::shuffleAlbumIndex() {
    auto current_playlist_album_id = 0;
    auto current_playlist_music_id = 0;

    if (play_index_.isValid()) {
        current_playlist_album_id = indexValue(play_index_, PLAYLIST_ALBUM_ID).toInt();
        current_playlist_music_id = indexValue(play_index_, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    }

    const auto count = proxy_model_->rowCount();
    if (count == 0 || current_playlist_album_id == 0) {
        return QModelIndex();
    }

	// Avoid reappearing the same album.
    rng_.SetSeed(current_playlist_album_id);

    if (album_songs_id_cache_.isEmpty()) {
        for (auto index = 0; index < count; ++index) {
            auto album_id = proxy_model_->index(index, PLAYLIST_ALBUM_ID).data().toInt();
            if (album_id != 0) {
                album_songs_id_cache_[album_id].append(index);
            }
        }
    }    

    auto album_ids = album_songs_id_cache_.keys();
    auto selected_album_id = current_playlist_album_id;

    if (album_ids.size() > 1) {
        do {
            const auto selected_album_index = rng_.NextInt32(0, album_ids.size() - 1);
            selected_album_id = album_ids[selected_album_index];
        } while (selected_album_id == current_playlist_album_id);
        Q_ASSERT(selected_album_id != current_playlist_album_id);
    }

	// Avoid reappearing the same music.
    if (current_playlist_music_id != 0) {
        rng_.SetSeed(current_playlist_music_id);
    }    

    const auto& selected_album_songs = album_songs_id_cache_[selected_album_id];
    if (selected_album_songs.isEmpty()) {
        throw std::exception("Not found song id in cache");
    }

    const auto selected_song_index = rng_.NextInt32(0, selected_album_songs.size() - 1);
    const auto selected_row = selected_album_songs[selected_song_index];

    return model()->index(selected_row, PLAYLIST_IS_PLAYING);
}

void PlaylistTableView::setNowPlaying(const QModelIndex& index) {
    play_index_ = index;
    setCurrentIndex(play_index_);    
    const auto entity = item(play_index_);
    qDaoFacade.playlist_dao.clearNowPlaying(playlist_id_);
    qDaoFacade.playlist_dao.setNowPlayingState(playlist_id_, entity.playlist_music_id, PlayingState::PLAY_PLAYING);
    play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
}

void PlaylistTableView::setAlbumCoverId(int32_t album_id, const QString& cover_id) {
    // 為了避免多次觸發 dataChanged，可以先收集要改的 index
    QVector<QModelIndex> changed_indexes;

    for (int row = 0; row < proxy_model_->rowCount(); ++row) {
        auto index = proxy_model_->index(row, PLAYLIST_MUSIC_COVER_ID);
        auto entity = item(index);
        // 僅針對同一個 album_id 的列進行更新
        if (entity.album_id == album_id) {
            // 若要改變顯示資料(暫時儲存在 model)，直接 setData
            proxy_model_->setData(index, cover_id);
            changed_indexes.push_back(index);
        }
    }

    // 最後再一次性觸發 dataChanged
    // 若 changed_indexes 為空就不必通知
    if (!changed_indexes.isEmpty()) {
        // 如果這些 row 連續，可一次 dataChanged(first, last)
        // 若不連續，則可用循環或另一種方式通知
        const auto first_index = changed_indexes.first();
        const auto last_index = changed_indexes.last();
        reload();
    }
}

ScanFileProgressPage* PlaylistTableView::progressPage() const {
    return progress_page_;
}

void PlaylistTableView::setNowPlayState(PlayingState playing_state) {
    if (proxy_model_->rowCount() == 0) {
        return;
    }
    if (!play_index_.isValid()) {
        return;
    }
    const auto entity = item(play_index_);
    qDaoFacade.playlist_dao.setNowPlayingState(playlistId(), entity.playlist_music_id, playing_state);
    scrollToIndex(play_index_);
    reload(playing_state != PlayingState::PLAY_CLEAR ? true : false);
    play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
    emit updatePlayingState(entity, playing_state);
}

void PlaylistTableView::scrollToIndex(const QModelIndex& index) {
    if (!enable_scroll_) {
        return;
    }
    QTableView::scrollTo(index, PositionAtCenter);
}

std::optional<QModelIndex> PlaylistTableView::selectFirstItem() const {
    auto select_row = selectionModel()->selectedRows();
    if (select_row.isEmpty()) {
        return std::nullopt;
    }
    return MakeOptional<QModelIndex>(std::move(select_row[0]));
}

QList<PlayListEntity> PlaylistTableView::items() const {
    QList<PlayListEntity> items;
    items.reserve(proxy_model_->rowCount());
    for (auto i = 0; i < proxy_model_->rowCount(); ++i) {
        items.push_back(item(proxy_model_->index(i, 0)));
    }
    return items;
}

OrderedMap<int32_t, QModelIndex> PlaylistTableView::selectItemIndex() const {
    OrderedMap<int32_t, QModelIndex> select_items;

    Q_FOREACH(auto index, selectionModel()->selectedRows()) {
        if (!index.isValid()) {
            continue;
        }
        auto const row = index.row();
        select_items.emplace(row, index);
    }
    return select_items;
}

QModelIndex PlaylistTableView::playOrderIndex(PlayerOrder order) {
    QModelIndex index;

    QElapsedTimer timer;

    switch (order) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        index = nextIndex(1);
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        index = play_index_;
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM:
        timer.start();
        index = shuffleAlbumIndex();        
        XAMP_LOG_DEBUG("shuffleAlbumIndex: {} ms", timer.nsecsElapsed());
        break;
    }
    if (!index.isValid()) {
        index = firstIndex();
    }
    return index;
}

void PlaylistTableView::play(PlayerOrder order, bool is_plays) {
    if (!proxy_model_->rowCount()) {
        return;
    }
    onPlayIndex(playOrderIndex(order), is_plays);
}

void PlaylistTableView::onPlayIndex(const QModelIndex& index, bool is_play) {
    if (!index.isValid()) {
        return;
    }
    play_index_ = index;
    setNowPlaying(play_index_);
    const auto entity = item(play_index_);
    emit playMusic(playlistId(), entity, is_play);
}

void PlaylistTableView::removePlaying() {
    qDaoFacade.playlist_dao.clearNowPlaying(playlist_id_);
    reload();
}

void PlaylistTableView::removeAll() {
    qDaoFacade.playlist_dao.removePlaylistAllMusic(playlistId());
    reload();
}

void PlaylistTableView::removeItem(const QModelIndex& index) {
    proxy_model_->removeRows(index.row(), 1, index);
}

void PlaylistTableView::removeSelectItems() {
    const auto rows = selectItemIndex();

    QVector<int> remove_music_ids;

    for (auto itr = rows.begin(); itr != rows.end(); ++itr) {
        const auto it = item(itr->second);
        qDaoFacade.playlist_dao.clearNowPlaying(playlist_id_, it.playlist_music_id);
        remove_music_ids.push_back(it.music_id);
    }

    const auto count = proxy_model_->rowCount();
	if (!count) {
        qDaoFacade.playlist_dao.clearNowPlaying(playlist_id_);
	}
    
    qDaoFacade.playlist_dao.removePlaylistMusic(playlist_id_, remove_music_ids);
    reload();
}
