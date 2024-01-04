#include <widget/playlisttableview.h>

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
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/playlistentity.h>
#include <widget/ui_utilts.h>
#include <widget/fonticon.h>
#include <widget/zib_utiltis.h>
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
	musics.heart,
	musics.duration,
	musics.comment,
	albums.year
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
	albums.album ASC,
	musics.track ASC)").arg(playlist_id);
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
	musics.heart,
	musics.duration,
	musics.comment,
	albums.year
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
-- GROUP BY
--	musics.parentPath,
--	musics.track 
ORDER BY
	musics.parentPath ASC,
	musics.track ASC)").arg(playlist_id);
    }
}

class PlayListStyledItemDelegate final : public QStyledItemDelegate {
public:
    static constexpr auto kPlaylistHeartSize = QSize(16, 16);
    static constexpr auto kPlaylistCoverSize = QSize(32, 32);

    using QStyledItemDelegate::QStyledItemDelegate;

    mutable LruCache<QString, QIcon> icon_cache_;

    explicit PlayListStyledItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {
    }

    static QIcon UniformIcon(QIcon icon, QSize size) {
        QIcon result;
        const auto base_pixmap = icon.pixmap(size);
        for (const auto state : { QIcon::Off, QIcon::On }) {
            for (const auto mode : { QIcon::Normal, QIcon::Disabled, QIcon::Active, QIcon::Selected })
                result.addPixmap(base_pixmap, mode, state);
        }
        return result;
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) {
            return;
        }

        painter->setRenderHints(QPainter::Antialiasing, true);
        painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
        painter->setRenderHints(QPainter::TextAntialiasing, true);

        QStyleOptionViewItem opt(option);
        
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

        opt.decorationSize = QSize(view->columnWidth(index.column()), view->verticalHeader()->defaultSectionSize());
        opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
        opt.font.setFamily(qTEXT("MonoFont"));

        switch (index.column()) {
        case PLAYLIST_TITLE:
        case PLAYLIST_ALBUM:
            opt.font.setFamily(qTEXT("UIFont"));
#ifdef Q_OS_WIN
            opt.font.setWeight(QFont::Weight::Medium);
#endif
            opt.text = value.toString();
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignLeft;
            break;
        case PLAYLIST_ARTIST:
            opt.font.setFamily(qTEXT("UIFont"));
#ifdef Q_OS_WIN
            opt.font.setWeight(QFont::Weight::Medium);
#endif
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = value.toString();
            break;
        case PLAYLIST_TRACK:
	        {
				constexpr auto kPlayingStateIconSize = 10;

                auto is_playing  = index.model()->data(index.model()->index(index.row(), PLAYLIST_PLAYING));
                auto playing_state = is_playing.toInt();
                QSize icon_size(kPlayingStateIconSize, kPlayingStateIconSize);

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
        case PLAYLIST_HEART:
	        {
            auto is_heart_pressed = index.model()->data(index.model()->index(index.row(), PLAYLIST_HEART)).toInt();
        	if (is_heart_pressed > 0) {
                QVariantMap font_options;
                font_options.insert(FontIconOption::kScaleFactorAttr, QVariant::fromValue(0.4));
                font_options.insert(FontIconOption::kColorAttr, QColor(Qt::red));

                opt.icon = qTheme.fontIcon(is_heart_pressed ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART, font_options);
                // note: 解決圖示再選擇的時候會蓋掉顏色的問題
                opt.icon = UniformIcon(opt.icon, opt.decorationSize);

                opt.features = QStyleOptionViewItem::HasDecoration;
                opt.decorationAlignment = Qt::AlignCenter;
                opt.displayAlignment = Qt::AlignCenter;
        	}
	        }
            break;
        case PLAYLIST_COVER_ID:
	        {
				opt.icon = icon_cache_.GetOrAdd(value.toString(), [&value]() {
                    constexpr QSize icon_size(42, 46);
                    const QIcon icon(image_utils::roundImage(qImageCache.getOrDefault(value.toString()), kPlaylistCoverSize));
                    return UniformIcon(icon, icon_size);
                });
				opt.features = QStyleOptionViewItem::HasDecoration;
				opt.decorationAlignment = Qt::AlignCenter;
				opt.displayAlignment = Qt::AlignCenter;
	        }
            break;
		default:
            use_default_style = true;            
            break;
        }

        if (!use_default_style) {
            option.widget->style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);
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

    model_->setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, qTR("Id"));
    model_->setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, qTR("IsPlaying"));
    model_->setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, qTR("   #"));
    model_->setHeaderData(PLAYLIST_FILE_PATH, Qt::Horizontal, qTR("FilePath"));
    model_->setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, qTR("Title"));
    model_->setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, qTR("FileName"));
    model_->setHeaderData(PLAYLIST_FILE_SIZE, Qt::Horizontal, qTR("FileSize"));
    model_->setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, qTR("Album"));
    model_->setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, qTR("Artist"));
    model_->setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, qTR("Duration"));
    model_->setHeaderData(PLAYLIST_BIT_RATE, Qt::Horizontal, qTR("BitRate"));
    model_->setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, qTR("SampleRate"));
    model_->setHeaderData(PLAYLIST_ALBUM_RG, Qt::Horizontal, qTR("AlbumRG"));
    model_->setHeaderData(PLAYLIST_ALBUM_PK, Qt::Horizontal, qTR("AlbumPK"));
    model_->setHeaderData(PLAYLIST_LAST_UPDATE_TIME, Qt::Horizontal, qTR("LastUpdateTime"));
    model_->setHeaderData(PLAYLIST_TRACK_RG, Qt::Horizontal, qTR("TrackRG"));
    model_->setHeaderData(PLAYLIST_TRACK_PK, Qt::Horizontal, qTR("TrackPK"));
    model_->setHeaderData(PLAYLIST_TRACK_LOUDNESS, Qt::Horizontal, qTR("Loudness"));
    model_->setHeaderData(PLAYLIST_GENRE, Qt::Horizontal, qTR("Genre"));
    model_->setHeaderData(PLAYLIST_ALBUM_ID, Qt::Horizontal, qTR("AlbumId"));
    model_->setHeaderData(PLAYLIST_PLAYLIST_MUSIC_ID, Qt::Horizontal, qTR("PlaylistId"));
    model_->setHeaderData(PLAYLIST_FILE_EXT, Qt::Horizontal, qTR("FileExt"));
    model_->setHeaderData(PLAYLIST_FILE_PARENT_PATH, Qt::Horizontal, qTR("ParentPath"));
    model_->setHeaderData(PLAYLIST_COVER_ID, Qt::Horizontal, qTR(""));
    model_->setHeaderData(PLAYLIST_ARTIST_ID, Qt::Horizontal, qTR("ArtistId"));
    model_->setHeaderData(PLAYLIST_HEART, Qt::Horizontal, qTR(""));
    model_->setHeaderData(PLAYLIST_COMMENT, Qt::Horizontal, qTR("Comment"));
    model_->setHeaderData(PLAYLIST_YEAR, Qt::Horizontal, qTR("Year"));

    auto column_list = qAppSettings.valueAsStringList(column_setting_name);

    if (column_list.empty()) {
        const QList<int> hidden_columns{
            PLAYLIST_MUSIC_ID,
            PLAYLIST_PLAYING,
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
            PLAYLIST_COVER_ID,
            PLAYLIST_FILE_EXT,
            PLAYLIST_FILE_PARENT_PATH,
            PLAYLIST_PLAYLIST_MUSIC_ID,
            PLAYLIST_COMMENT,
            PLAYLIST_HEART,
            PLAYLIST_YEAR
        };

        for (auto column : qAsConst(hidden_columns)) {
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
    auto hide_this_column_act = action_map.addAction(qTR("Hide this column"), [last_referred_logical_column, this]() {
        setColumnHidden(last_referred_logical_column, true);
    qAppSettings.removeList(column_setting_name_, QString::number(last_referred_logical_column));
        });
    hide_this_column_act->setIcon(qTheme.fontIcon(Glyphs::ICON_HIDE));

    auto select_column_show_act = action_map.addAction(qTR("Select columns to show..."), [pt, header_view, this]() {
        ActionMap<PlayListTableView> action_map(this);
    for (auto column = 0; column < header_view->count(); ++column) {
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
#ifdef Q_OS_WIN
    f.setWeight(QFont::Weight::Medium);
#else
    f.setWeight(QFont::Weight::Normal);
#endif
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

        PlayListEntity item;
        if (model_->rowCount() > 0 && index.isValid()) {
            item = getEntity(index);
        }             

        if (cloud_mode_) {            
            auto* copy_album_act = action_map.addAction(qTR("Copy album"));
            copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

            auto* copy_artist_act = action_map.addAction(qTR("Copy artist"));
            auto* copy_title_act = action_map.addAction(qTR("Copy title"));

            action_map.setCallback(copy_album_act, [item]() {
                QApplication::clipboard()->setText(item.album);
                });
            action_map.setCallback(copy_artist_act, [item]() {
                QApplication::clipboard()->setText(item.artist);
                });
            action_map.setCallback(copy_title_act, [item]() {
                QApplication::clipboard()->setText(item.title);
                });

            action_map.addSeparator();

            auto* remove_all_act = action_map.addAction(qTR("Remove all"));
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
                try {
                    action_map.exec(pt);
                }
                catch (Exception const& e) {
                }
                catch (std::exception const& e) {
                }                
            }
            return;
        }

        auto* load_file_act = action_map.addAction(qTR("Load local file"), [this]() {
            getOpenMusicFileName(this, [this](const auto& file_name) {
                append(file_name);
                });
            });
        load_file_act->setIcon(qTheme.fontIcon(Glyphs::ICON_LOAD_FILE));

        auto* load_dir_act = action_map.addAction(qTR("Load file directory"), [this]() {
            const auto dir_name = getExistingDirectory(this);
            if (dir_name.isEmpty()) {
                return;
            }
            append(dir_name);
            });
        load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_LOAD_DIR));

        if (enable_delete_ && model()->rowCount() > 0) {
            auto* remove_all_act = action_map.addAction(qTR("Remove all"));
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

        auto* scan_select_item_replaygain_act = action_map.addAction(qTR("Scan file ReplayGain"));
        scan_select_item_replaygain_act->setIcon(qTheme.fontIcon(Glyphs::ICON_SCAN_REPLAY_GAIN));

        action_map.addSeparator();
        auto* export_flac_file_act = action_map.addAction(qTR("Export FLAC file"));
        export_flac_file_act->setIcon(qTheme.fontIcon(Glyphs::ICON_EXPORT_FILE));

        const auto select_row = selectionModel()->selectedRows();
        if (!select_row.isEmpty()) {
            auto* export_aac_file_submenu = action_map.addSubMenu(qTR("Export AAC file"));
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
                        auto entity = this->item(row.second);
                        if (entity.sample_rate != profile.sample_rate) {
                            continue;
                        }
                        emit encodeAacFile(entity, profile);
                    }
                });
            }
        }
        else {
            action_map.addAction(qTR("Export AAC file"));
        }       

        auto* export_wav_file_act = action_map.addAction(qTR("Export WAV file"));

        action_map.addSeparator();
        auto * copy_album_act = action_map.addAction(qTR("Copy album"));
        copy_album_act->setIcon(qTheme.fontIcon(Glyphs::ICON_COPY));

        auto * copy_artist_act = action_map.addAction(qTR("Copy artist"));
        auto * copy_title_act = action_map.addAction(qTR("Copy title"));

        if (model_->rowCount() == 0 || !index.isValid()) {
            try {
                action_map.exec(pt);
            }
            catch (Exception const& e) {
            }
            catch (std::exception const& e) {
            }
            return;
        }

        action_map.addSeparator();        

        auto* add_to_playlist_act = action_map.addAction(qTR("Add file to playlist"));
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

        auto* select_item_edit_track_info_act = action_map.addAction(qTR("Edit track information"));
        select_item_edit_track_info_act->setIcon(qTheme.fontIcon(Glyphs::ICON_EDIT));

        auto reload_track_info_act = action_map.addAction(qTR("Reload track information"));
        reload_track_info_act->setIcon(qTheme.fontIcon(Glyphs::ICON_RELOAD));

        auto* open_local_file_path_act = action_map.addAction(qTR("Open local file path"));
        open_local_file_path_act->setIcon(qTheme.fontIcon(Glyphs::ICON_OPEN_FILE_PATH));
        action_map.setCallback(open_local_file_path_act, [item]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(item.parent_path));
        });

        action_map.setCallback(reload_track_info_act, [this, item]() {
            onReloadEntity(item);
        });
    	
        action_map.addSeparator();
        action_map.setCallback(copy_album_act, [item]() {
            QApplication::clipboard()->setText(item.album);
        });
        action_map.setCallback(copy_artist_act, [item]() {
            QApplication::clipboard()->setText(item.artist);
        });
        action_map.setCallback(copy_title_act, [item]() {
            QApplication::clipboard()->setText(item.title);
        });

        action_map.setCallback(select_item_edit_track_info_act, [this]() {
            const auto rows = selectItemIndex();
            QList<PlayListEntity> items;
            for (const auto& row : rows) {
                items.push_front(this->item(row.second));
            }
            if (items.isEmpty()) {
                return;
            }
            emit editTags(playlistId(), items);
            Q_FOREACH(auto entity, items) {
                onReloadEntity(entity);
            }
        });

        action_map.setCallback(scan_select_item_replaygain_act, [this]() {
            const auto rows = selectItemIndex();
            QList<PlayListEntity> items;
            for (const auto& row : rows) {
                items.push_front(this->item(row.second));
            }
            emit readReplayGain(playlistId(), items);
        });

        action_map.setCallback(export_flac_file_act, [this]() {
            const auto rows = selectItemIndex();
            for (const auto& row : rows) {
                auto entity = this->item(row.second);
                emit encodeFlacFile(entity);
            }
            });

        action_map.setCallback(export_wav_file_act, [this]() {
            const auto rows = selectItemIndex();
            for (const auto& row : rows) {
                auto entity = this->item(row.second);
                emit encodeWavFile(entity);
            }
        });

        try {
            action_map.exec(pt);
        }
        catch (Exception const& e) {
        } catch (std::exception const &e){
        }
    });

    const auto *control_A_key = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    (void) QObject::connect(control_A_key, &QShortcut::activated, [this]() {
        selectAll();
    });

    // note: Fix QTableView select color issue.
    setFocusPolicy(Qt::StrongFocus);
}

void PlayListTableView::onReloadEntity(const PlayListEntity& item) {
    try {
        qMainDb.addOrUpdateMusic(TagIO::getTrackInfo(item.file_path.toStdWString()));
        reload();
        play_index_ = proxy_model_->index(play_index_.row(), play_index_.column());
    }
    catch (std::filesystem::filesystem_error& e) {
        XAMP_LOG_DEBUG("Reload track information error: {}", String::LocaleStringToUTF8(e.what()));
    }
    catch (...) {
    }
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
        case PLAYLIST_PLAYING:
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
        case PLAYLIST_HEART:
        case PLAYLIST_COVER_ID:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, kColumnCoverIdWidth);
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
    return model()->index(next_index, PLAYLIST_PLAYING);
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
    return model()->index(selected, PLAYLIST_PLAYING);
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
