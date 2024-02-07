#include <widget/playlisttableview.h>

#include <QHeaderView>
#include <QDesktopServices>
#include <QClipboard>
#include <QScrollBar>
#include <QShortcut>
#include <QFormLayout>
#include <QLineEdit>
#include <QApplication>
#include <QCheckBox>
#include <qevent.h>
#include <QPainter>
#include <QSqlError>
#include <QStyledItemDelegate>
#include <ranges>

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
#include <widget/util/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/playlistentity.h>
#include <widget/util/ui_utilts.h>
#include <widget/fonticon.h>
#include <widget/util/zib_utiltis.h>
#include <widget/tagio.h>

namespace {
    QString groupAlbum(int32_t playlist_id) {
        return qSTR(R"(
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
	musics.coverId as musicCoverId    
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
        return qSTR(R"(
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
	musics.coverId as musicCoverId    
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
	musics.parentPath ASC,
	musics.track ASC)").arg(playlist_id);
    }
}


class PlayListStyledItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    static constexpr auto kPlayingStateIconSize = 10;
    
    mutable LruCache<QString, QIcon> icon_cache_;

    static QIcon uniformIcon(QIcon icon, QSize size) {
        QIcon result;
        const auto base_pixmap = icon.pixmap(size);
        for (const auto state : { QIcon::Off, QIcon::On }) {
            for (const auto mode : { QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected })
                result.addPixmap(base_pixmap, mode, state);
        }
        return result;
    }

    explicit PlayListStyledItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {
    }    

    /*QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.column() == PLAYLIST_CHECKED) {
            auto* editor = new QCheckBox(parent);            
            return editor;s
        }        
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override {
        if (index.column() != PLAYLIST_CHECKED) {
            return;
        }

        auto* check_box = qobject_cast<QCheckBox*>(editor);
        check_box->setChecked(index.data().toBool());
    }

    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
        if (index.column() != PLAYLIST_CHECKED) {
            return;
        }

        const auto playlist_music_id = index.model()->data(index.model()->index(index.row(), PLAYLIST_PLAYLIST_MUSIC_ID)).toInt();
        const auto* check_box = qobject_cast<QCheckBox*>(editor);
        model->setData(index, check_box->isChecked(), Qt::EditRole);
        qMainDb.updatePlaylistMusicChecked(playlist_music_id, check_box->isChecked());        
    }*/

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) {
            return;
        }

        painter->setRenderHints(QPainter::Antialiasing, true);
        painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
        painter->setRenderHints(QPainter::TextAntialiasing, true);

        QStyleOptionViewItem opt(option);
        QStyleOptionButton check_box_opt;
        check_box_opt.rect = opt.rect;
        
        opt.state &= ~QStyle::State_HasFocus;

        const auto* view = qobject_cast<const PlayListTableView*>(opt.styleObject);
        const auto behavior = view->selectionBehavior();
        const auto hover_index = view->hoverIndex();

        if (!(option.state & QStyle::State_Selected) && behavior != QTableView::SelectItems) {
            if (behavior == QTableView::SelectRows && hover_index.row() == index.row())
                opt.state |= QStyle::State_MouseOver;
            if (behavior == QTableView::SelectColumns && hover_index.column() == index.column())
                opt.state |= QStyle::State_MouseOver;
        }

        const auto value  = index.model()->data(index.model()->index(index.row(), index.column()));
        auto use_default_style = false;
        auto use_checkbox_style = false;

        opt.decorationSize = QSize(view->columnWidth(index.column()), view->verticalHeader()->defaultSectionSize());
        opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
        opt.font.setFamily(qTEXT("MonoFont"));

        switch (index.column()) {
        case PLAYLIST_TITLE:
        case PLAYLIST_ALBUM:
            opt.font.setFamily(qTEXT("UIFont"));
            opt.text = value.toString();
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignLeft;
            break;
        case PLAYLIST_ARTIST:
            opt.font.setFamily(qTEXT("UIFont"));
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignLeft;
            opt.text = value.toString();
            break;
        case PLAYLIST_TRACK:
	        {
				static constexpr QSize icon_size(kPlayingStateIconSize, kPlayingStateIconSize);
                auto is_playing  = index.model()->data(index.model()->index(index.row(), PLAYLIST_IS_PLAYING));
                auto playing_state = is_playing.toInt();
                if (playing_state == PlayingState::PLAY_PLAYING) {
                    opt.icon = qTheme.playlistPlayingIcon(icon_size);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignCenter;
                    opt.displayAlignment = Qt::AlignCenter;
                }
                else if (playing_state == PlayingState::PLAY_PAUSE) {
                    opt.icon = qTheme.playlistPauseIcon(icon_size);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignCenter;
                    opt.displayAlignment = Qt::AlignCenter;
                } else {
                    opt.displayAlignment = Qt::AlignCenter;
                    opt.text = value.toString();
                }
	        }
            break;
        case PLAYLIST_FILE_SIZE:
            opt.text = formatBytes(value.toULongLong());
            break;
        case PLAYLIST_BIT_RATE:
            opt.text = formatBitRate(value.toUInt());
            break;
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_TRACK_PK:
        case PLAYLIST_TRACK_RG:
        case PLAYLIST_TRACK_LOUDNESS:
            switch (index.column()) {
            case PLAYLIST_ALBUM_PK:
            case PLAYLIST_TRACK_PK:
                opt.text = qSTR("%1")
            		.arg(value.toDouble(), 4, 'f', 6, QLatin1Char('0'));
                break;
            case PLAYLIST_ALBUM_RG:
            case PLAYLIST_TRACK_RG:
                opt.text = qSTR("%1 dB")
            		.arg(value.toDouble(), 4, 'f', 2, QLatin1Char('0'));
                break;
            case PLAYLIST_TRACK_LOUDNESS:
                opt.text = qSTR("%1 LUFS")
                    .arg(value.toDouble(), 4, 'f', 2, QLatin1Char('0'));
                break;
            }
            break;
        case PLAYLIST_SAMPLE_RATE:
            opt.text = formatSampleRate(value.toUInt());
            break;
        case PLAYLIST_DURATION:
            opt.text = formatDuration(value.toDouble());            
            break;
        case PLAYLIST_LAST_UPDATE_TIME:
            opt.text = formatTime(value.toULongLong());
            break;
        case PLAYLIST_LIKE:
	        {
            auto is_heart_pressed = index.model()->data(index.model()->index(index.row(), PLAYLIST_LIKE)).toInt();
        	if (is_heart_pressed > 0) {
                QVariantMap font_options;
                font_options.insert(FontIconOption::kScaleFactorAttr, QVariant::fromValue(0.4));
                font_options.insert(FontIconOption::kColorAttr, QColor(Qt::red));

                opt.icon = qTheme.fontRawIconOption(is_heart_pressed ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART, font_options);
                // note: 解決圖示再選擇的時候會蓋掉顏色的問題
                opt.icon = uniformIcon(opt.icon, opt.decorationSize);

                opt.features = QStyleOptionViewItem::HasDecoration;
                opt.decorationAlignment = Qt::AlignCenter;
                opt.displayAlignment = Qt::AlignCenter;
        	}
	        }
            break;
        case PLAYLIST_ALBUM_COVER_ID:
	        {
				auto music_cover_id = index.model()->data(index.model()->index(index.row(), PLAYLIST_MUSIC_COVER_ID)).toString();
                auto id = value.toString();
				if (!music_cover_id.isEmpty()) {
                    id = music_cover_id;
				}

				opt.icon = icon_cache_.GetOrAdd(id, [id]() {
                    const QIcon icon(image_utils::roundImage(qImageCache.getOrDefault(id), PlayListTableView::kCoverSize));
                    return uniformIcon(icon, PlayListTableView::kCoverSize);
                });
				opt.features = QStyleOptionViewItem::HasDecoration;
				opt.decorationAlignment = Qt::AlignCenter;
				opt.displayAlignment = Qt::AlignCenter;
	        }
            break;
        /*case PLAYLIST_CHECKED: {
            use_checkbox_style = true;
            const auto is_checked = index.model()->data(index.model()->index(index.row(), PLAYLIST_CHECKED)).toBool();
            if (is_checked) {
                check_box_opt.state |= QStyle::State_On;
            } else {
                check_box_opt.state |= QStyle::State_Off;
            }
			}
            break;*/
		default:
            use_default_style = true;            
            break;
        }

        if (!use_default_style) {
            if (!use_checkbox_style) {
                option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);
            } else {
                option.widget->style()->drawControl(QStyle::CE_CheckBox, &check_box_opt, painter, option.widget);
            }
        } else {
            QStyledItemDelegate::paint(painter, opt, index);
        }
    }
};


void PlayListTableView::search(const QString& keyword) const {
	const QRegularExpression reg_exp(keyword, QRegularExpression::CaseInsensitiveOption);
    proxy_model_->addFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->setFilterRegularExpression(reg_exp);
}

void PlayListTableView::reload() {
    QString query_string;

    if (group_ != PLAYLIST_GROUP_ALBUM) {
        query_string = groupNone(playlist_id_);
    } else {
        query_string = groupAlbum(playlist_id_);
    }

    const QSqlQuery query(query_string, qMainDb.database());
    model_->setQuery(query);
    if (model_->lastError().type() != QSqlError::NoError) {
        XAMP_LOG_DEBUG("SqlException: {}", model_->lastError().text().toStdString());
    }

    // NOTE: 呼叫此函數就會更新index, 會導致playing index失效    
    model_->dataChanged(QModelIndex(), QModelIndex());
}

PlayListTableView::PlayListTableView(QWidget* parent, int32_t playlist_id)
    : QTableView(parent)
    , playlist_id_(playlist_id)
    , model_(new PlayListSqlQueryTableModel(this))
	, proxy_model_(new PlayListTableFilterProxyModel(this)) {
    initial();
}

PlayListTableView::~PlayListTableView() = default;

void PlayListTableView::setPlaylistId(const int32_t playlist_id, const QString &column_setting_name) {
    playlist_id_ = playlist_id;
    column_setting_name_ = column_setting_name;

    qMainDb.clearNowPlaying(playlist_id_);

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
        PLAYLIST_LIKE,
        PLAYLIST_PLAYLIST_MUSIC_ID,
        PLAYLIST_CHECKED,
        PLAYLIST_ALBUM_ID,
        PLAYLIST_ALBUM_COVER_ID,
        PLAYLIST_MUSIC_COVER_ID
    };

    always_hidden_columns_ = always_hidden_columns;

    for (const auto column : qAsConst(always_hidden_columns_)) {
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
            PLAYLIST_ALBUM_COVER_ID,
            PLAYLIST_FILE_EXT,
            PLAYLIST_FILE_PARENT_PATH,
            PLAYLIST_PLAYLIST_MUSIC_ID,
            PLAYLIST_COMMENT,
            PLAYLIST_LIKE,
            PLAYLIST_YEAR
        };

        for (const auto column : qAsConst(hidden_columns)) {
            hideColumn(column);
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

void PlayListTableView::setHeaderViewHidden(bool enable) {
	if (enable) {
        horizontalHeader()->hide();
        horizontalHeader()->setContextMenuPolicy(Qt::NoContextMenu);
        return;
	}

    horizontalHeader()->show();
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlayListTableView> action_map(this);

    auto* header_view = horizontalHeader();

    auto last_referred_logical_column = header_view->logicalIndexAt(pt);
    auto hide_this_column_act = action_map.addAction(tr("Hide this column"), [last_referred_logical_column, this]() {
        setColumnHidden(last_referred_logical_column, true);
		qAppSettings.removeList(column_setting_name_, QString::number(last_referred_logical_column));
        });
    hide_this_column_act->setIcon(qTheme.fontIcon(Glyphs::ICON_HIDE));

    auto select_column_show_act = action_map.addAction(tr("Select columns to show..."), [pt, header_view, this]() {
        ActionMap<PlayListTableView> action_map(this);
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

void PlayListTableView::initial() {
    proxy_model_->addFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->addFilterByColumn(PLAYLIST_ALBUM);
    proxy_model_->setSourceModel(model_);
    setModel(proxy_model_);

    auto f = font();
    f.setPointSize(qTheme.defaultFontSize());
    setFont(f);

    setUpdatesEnabled(true);
    setAcceptDrops(true);
    setDragEnabled(true);
    setShowGrid(false);
    setMouseTracking(true);
    setAlternatingRowColors(true);

    setDragDropMode(InternalMove);
    setFrameShape(NoFrame);
    setFocusPolicy(Qt::NoFocus);

    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
    setSelectionMode(ExtendedSelection);
    setSelectionBehavior(SelectRows);

    viewport()->setAttribute(Qt::WA_StaticContents);

    verticalHeader()->setVisible(false);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(kColumnHeight);

    horizontalScrollBar()->setDisabled(true);

    horizontalHeader()->setVisible(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    setItemDelegate(new PlayListStyledItemDelegate(this));
 
    installEventFilter(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));

    (void)QObject::connect(model_, &QAbstractTableModel::modelReset,
    [this] {
        while (model_->canFetchMore()) {
            model_->fetchMore();
        }
    });

    setEditTriggers(DoubleClicked | SelectedClicked);
    verticalHeader()->setSectionsMovable(false);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    (void)QObject::connect(this, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        if (model_->rowCount() > 0) {
            playItem(index);
        }        
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);
		
        ActionMap<PlayListTableView> action_map(this);

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

            auto *add_to_playlist = action_map.addSubMenu(tr("Add to cloud playlist"));
            QMap<QString, QString> playlist_ids;
            qMainDb.forEachPlaylist([&playlist_ids](auto, auto, auto store_type, auto cloud_playlist_id, auto name) {
                if (store_type == StoreType::CLOUD_STORE) {
                    playlist_ids.insert(cloud_playlist_id, name);
                }
                });

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
                if (play_list_entity.heart > 0) {
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
                action_map.setCallback(like_song_act, [this, &play_list_entity]() {
                    emit likeSong(play_list_entity.heart > 0, play_list_entity);
                    });
            }

            for (auto itr = playlist_ids.begin(); itr != playlist_ids.end(); ++itr) {
                const auto& playlist_id = itr.key();
                add_to_playlist->addAction(qSTR("Add to playlist (%1)").arg(itr.value()), [playlist_id, video_ids, this]() {
                    QString source_playlist_id;
                    if (cloudPlaylistId()) {
                        source_playlist_id = cloudPlaylistId().value();
                    }
                    emit addToPlaylist(source_playlist_id, playlist_id, video_ids);
                    });
            }

            action_map.setCallback(action_map.addAction(tr("Download file")), [this]() {
                const auto rows = selectItemIndex();
                for (const auto& row : rows) {
                    auto play_list_entity = this->item(row.second);
                    emit downloadFile(play_list_entity);
                }
                });

            auto* remove_all_act = action_map.addAction(tr("Remove all"));
            remove_all_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

            action_map.setCallback(remove_all_act, [this]() {
                if (!model_->rowCount()) {
                    return;
                }

                IGNORE_DB_EXCEPTION(qMainDb.removePlaylistAllMusic(playlistId()))
            	reload();
                removePlaying();
                });

            if (model_->rowCount() > 0 && index.isValid()) {
                TRY_LOG(
                    action_map.exec(pt);
                )
            }
            return;
        }

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
        load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER_OPEN));

        if (enable_delete_ && model()->rowCount() > 0) {
            auto* remove_all_act = action_map.addAction(tr("Remove all"));
            remove_all_act->setIcon(qTheme.fontIcon(Glyphs::ICON_REMOVE_ALL));

            action_map.setCallback(remove_all_act, [this]() {
                if (!model_->rowCount()) {
                    return;
                }
                
                IGNORE_DB_EXCEPTION(qMainDb.removePlaylistAllMusic(playlistId()))
                reload();
                removePlaying();
            });
        }       

        action_map.addSeparator();

        auto* scan_select_item_replaygain_act = action_map.addAction(tr("Scan file ReplayGain"));
        scan_select_item_replaygain_act->setIcon(qTheme.fontIcon(Glyphs::ICON_SCAN_REPLAY_GAIN));

        action_map.addSeparator();
        auto* export_flac_file_act = action_map.addAction(tr("Export FLAC file"));
        export_flac_file_act->setIcon(qTheme.fontIcon(Glyphs::ICON_EXPORT_FILE));

        const auto select_row = selectionModel()->selectedRows();
        if (!select_row.isEmpty()) {
            auto* export_aac_file_submenu = action_map.addSubMenu(tr("Export AAC file"));
            for (const auto& profile : StreamFactory::GetAvailableEncodingProfile()) {
                if (profile.num_channels != AudioFormat::kMaxChannel
                    || profile.sample_rate < AudioFormat::k16BitPCM441Khz.GetSampleRate()
                    || profile.bitrate < kMinimalEncodingBitRate) {
                    continue;
                }

                auto profile_desc = qSTR("%0 bit, %1, %2").arg(
                    QString::number(profile.bit_per_sample),
                    formatSampleRate(profile.sample_rate),
                    formatBitRate(profile.bitrate));

                export_aac_file_submenu->addAction(profile_desc, [profile, this]() {
                    const auto rows = selectItemIndex();
                    for (const auto& row : rows) {
                        auto play_list_entity = this->item(row.second);
                        if (play_list_entity.sample_rate != profile.sample_rate) {
                            continue;
                        }
                        emit encodeAacFile(play_list_entity, profile);
                    }
                });
            }
        }
        else {
            action_map.addAction(tr("Export AAC file"));
        }       

        auto* export_wav_file_act = action_map.addAction(tr("Export WAV file"));

        action_map.addSeparator();
        auto * copy_album_act = action_map.addAction(tr("Copy album"));
        copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

        auto * copy_artist_act = action_map.addAction(tr("Copy artist"));
        auto * copy_title_act = action_map.addAction(tr("Copy title"));

        if (model_->rowCount() == 0 || !index.isValid()) {
            TRY_LOG(
                action_map.exec(pt);
            )
            return;
        }

        action_map.addSeparator();        

        auto* add_to_playlist_act = action_map.addAction(tr("Add file to playlist"));
        add_to_playlist_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FILE_CIRCLE_PLUS));
        action_map.setCallback(add_to_playlist_act, [this]() {
            if (const auto other_playlist_id = other_playlist_id_) {
                const auto rows = selectItemIndex();
                for (const auto& row : rows) {
	                const auto entity = this->item(row.second);
                    qMainDb.addMusicToPlaylist(entity.music_id, other_playlist_id.value(), entity.album_id);
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

        action_map.setCallback(scan_select_item_replaygain_act, [this]() {
            const auto rows = selectItemIndex();
            QList<PlayListEntity> entities;
            for (const auto& row : rows) {
                entities.push_front(this->item(row.second));
            }
            emit readReplayGain(playlistId(), entities);
        });

        action_map.setCallback(export_flac_file_act, [this]() {
            const auto rows = selectItemIndex();
            for (const auto& row : rows) {
                auto play_list_entity = this->item(row.second);
                emit encodeFlacFile(play_list_entity);
            }
            });

        action_map.setCallback(export_wav_file_act, [this]() {
            const auto rows = selectItemIndex();
            for (const auto& row : rows) {
                auto play_list_entity = this->item(row.second);
                emit encodeWavFile(play_list_entity);
            }
        });
        TRY_LOG(
            action_map.exec(pt);
        )
    });

    const auto *control_A_key = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    (void) QObject::connect(control_A_key, &QShortcut::activated, [this]() {
        selectAll();
    });

    // note: Fix QTableView select color issue.
    setFocusPolicy(Qt::StrongFocus);
}

void PlayListTableView::onReloadEntity(const PlayListEntity& item) {
    TRY_LOG(
        qMainDb.addOrUpdateMusic(TagIO::getTrackInfo(item.file_path.toStdWString()));
		reload();
		play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
    )
}

void PlayListTableView::pauseItem(const QModelIndex& index) {
    const auto entity = item(index);
    CATCH_DB_EXCEPTION(qMainDb.setNowPlayingState(playlistId(), entity.playlist_music_id, PlayingState::PLAY_PAUSE))
    update();
}

PlayListEntity PlayListTableView::item(const QModelIndex& index) const {
    return getEntity(index);
}

void PlayListTableView::playItem(const QModelIndex& index) {
    setNowPlaying(index);
    if (!play_index_.isValid()) {
        return;
    }
    const auto play_item = item(play_index_);
    emit playMusic(play_item);
}

void PlayListTableView::onThemeColorChanged(QColor /*backgroundColor*/, QColor /*color*/) {
    setStyleSheet(qTEXT("border: none"));
    horizontalHeader()->setStyleSheet(qTEXT("QHeaderView { background-color: transparent; }"));
}

void PlayListTableView::onUpdateReplayGain(int32_t playlistId,
    const PlayListEntity& entity,
    double track_loudness,
    double album_rg_gain,
    double album_peak,
    double track_rg_gain,
    double track_peak) {
    if (playlistId != playlist_id_) {
        return;
    }

    CATCH_DB_EXCEPTION(qMainDb.updateReplayGain(
        entity.music_id,
        album_rg_gain,
        album_peak, 
        track_rg_gain,
        track_peak))

    CATCH_DB_EXCEPTION(qMainDb.addOrUpdateTrackLoudness(entity.album_id,
        entity.artist_id,
        entity.music_id,
        track_loudness))

    XAMP_LOG_DEBUG(
        "Update DB music id: {} artist id: {} album id id: {},"
        "track_loudness: {:.2f} LUFS album_rg_gain: {:.2f} dB album_peak: {:.2f} track_rg_gain: {:.2f} dB track_peak: {:.2f}",
        entity.music_id,
        entity.artist_id,
        entity.album_id,
        track_loudness,
        album_rg_gain,
        album_peak, 
        track_rg_gain,
        track_peak);

    reload();

    play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
}

void PlayListTableView::keyPressEvent(QKeyEvent *event) {
    QAbstractItemView::keyPressEvent(event);
}

bool PlayListTableView::eventFilter(QObject* obj, QEvent* ev) {
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

void PlayListTableView::append(const QString& file_name) {
    emit extractFile(file_name, playlistId());
}

void PlayListTableView::onProcessDatabase(int32_t playlist_id, const QList<PlayListEntity>& entities) {
    if (playlistId() != playlist_id) {
        return;
    }

    for (const auto& entity : entities) {
        CATCH_DB_EXCEPTION(qMainDb.addMusicToPlaylist(entity.music_id, playlistId(), entity.album_id))
    }
    reload();
    emit addPlaylistItemFinished();
}

void PlayListTableView::onProcessTrackInfo(int32_t total_album, int32_t total_tracks) {
    reload();
    emit addPlaylistItemFinished();
}

void PlayListTableView::resizeEvent(QResizeEvent* event) {
    QTableView::resizeEvent(event);
    resizeColumn();
}

void PlayListTableView::mouseMoveEvent(QMouseEvent* event) {
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

void PlayListTableView::resizeColumn() {
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

int32_t PlayListTableView::playlistId() const {
    return playlist_id_;
}

QModelIndex PlayListTableView::firstIndex() const {
    return model()->index(0, 0);
}

void PlayListTableView::setOtherPlaylist(int32_t playlist_id) {
    other_playlist_id_ = playlist_id;
}

QModelIndex PlayListTableView::nextIndex(int forward) const {
    const auto count = proxy_model_->rowCount();
    const auto play_index = currentIndex();
    const auto next_index = (play_index.row() + forward) % count;
    return model()->index(next_index, PLAYLIST_IS_PLAYING);
}

QModelIndex PlayListTableView::shuffleIndex() {
    auto current_playlist_music_id = 0;
    if (play_index_.isValid()) {
        current_playlist_music_id = indexValue(play_index_, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    }
    const auto count = proxy_model_->rowCount();
    if (current_playlist_music_id != 0) {
        rng_.SetSeed(current_playlist_music_id);
    }    
    const auto selected = rng_.NextInt32(0) % count;
    return model()->index(selected, PLAYLIST_IS_PLAYING);
}

void PlayListTableView::setNowPlaying(const QModelIndex& index, bool is_scroll_to) {
    play_index_ = index;
    setCurrentIndex(play_index_);
    if (is_scroll_to) {
        QTableView::scrollTo(play_index_, PositionAtCenter);
    }
    const auto entity = item(play_index_);
    CATCH_DB_EXCEPTION(qMainDb.clearNowPlaying(playlist_id_))
	CATCH_DB_EXCEPTION(qMainDb.setNowPlayingState(playlist_id_, entity.playlist_music_id, PlayingState::PLAY_PLAYING))
    reload();
    play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
}

void PlayListTableView::setNowPlayState(PlayingState playing_state) {
    if (model_->rowCount() == 0) {
        return;
    }
    if (!play_index_.isValid()) {
        return;
    }
    const auto entity = item(play_index_);
    CATCH_DB_EXCEPTION(qMainDb.setNowPlayingState(playlistId(), entity.playlist_music_id, playing_state))
    reload();
    play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
    scrollToIndex(play_index_);
    emit updatePlayingState(entity, playing_state);
}

void PlayListTableView::scrollToIndex(const QModelIndex& index) {
    QTableView::scrollTo(index, PositionAtTop);
}

std::optional<PlayListEntity> PlayListTableView::selectPlayListEntity() const {
    if (const auto select_item = selectItem()) {
        return item(select_item.value());
	}
    return std::nullopt;
}

std::optional<QModelIndex> PlayListTableView::selectItem() const {
    auto select_row = selectionModel()->selectedRows();
    if (select_row.isEmpty()) {
        return std::nullopt;
    }
    return select_row[0];
}

OrderedMap<int32_t, QModelIndex> PlayListTableView::selectItemIndex() const {
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

void PlayListTableView::play(PlayerOrder order) {
    QModelIndex index;

    switch (order) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        index = nextIndex(1);
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:            
        index = play_index_;
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
        index = shuffleIndex();
        break;
    }
    if (!index.isValid()) {
        index = firstIndex();
    }

    const auto entity = item(index);
    onPlayIndex(index);
}

void PlayListTableView::onPlayIndex(const QModelIndex& index) {
    play_index_ = index;
    setNowPlaying(play_index_, true);
    const auto entity = item(play_index_);
    emit playMusic(entity);
}

void PlayListTableView::removePlaying() {
    CATCH_DB_EXCEPTION(qMainDb.clearNowPlaying(playlist_id_))
    reload();
}

void PlayListTableView::removeAll() {
    IGNORE_DB_EXCEPTION(qMainDb.removePlaylistAllMusic(playlistId()))
    reload();
}

void PlayListTableView::removeItem(const QModelIndex& index) {
    model_->removeRows(index.row(), 1, index);
}

void PlayListTableView::removeSelectItems() {
    const auto rows = selectItemIndex();

    QVector<int> remove_music_ids;

    for (const auto& row : std::ranges::reverse_view(rows)) {
        const auto it = item(row.second);
        CATCH_DB_EXCEPTION(qMainDb.clearNowPlaying(playlist_id_, it.playlist_music_id))
        remove_music_ids.push_back(it.music_id);
    }

    const auto count = model_->rowCount();
	if (!count) {
        CATCH_DB_EXCEPTION(qMainDb.clearNowPlaying(playlist_id_))
	}

    CATCH_DB_EXCEPTION(qMainDb.removePlaylistMusic(playlist_id_, remove_music_ids))
    reload();
}
