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
#include <widget/appsettingnames.h>

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
        const auto hover_index = view->GetHoverIndex();

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
                    opt.icon = qTheme.GetPlaylistPlayingIcon(icon_size);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignCenter;
                    opt.displayAlignment = Qt::AlignCenter;
                }
                else if (playing_state == PlayingState::PLAY_PAUSE) {
                    opt.icon = qTheme.PlaylistPauseIcon(icon_size);
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
            opt.text = FormatBytes(value.toULongLong());
            break;
        case PLAYLIST_BIT_RATE:
            opt.text = FormatBitRate(value.toUInt());
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
            		.arg(value.toDouble(), 4, 'f', 2, QLatin1Char('0'));
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
            opt.text = FormatSampleRate(value.toUInt());
            break;
        case PLAYLIST_DURATION:
            opt.text = FormatDuration(value.toDouble());            
            break;
        case PLAYLIST_LAST_UPDATE_TIME:
            opt.text = FormatTime(value.toULongLong());
            break;
        case PLAYLIST_HEART:
	        {
            auto is_heart_pressed = index.model()->data(index.model()->index(index.row(), PLAYLIST_HEART)).toInt();
        	if (is_heart_pressed > 0) {
                QVariantMap font_options;
                font_options.insert(FontIconOption::scaleFactorAttr, QVariant::fromValue(0.4));
                font_options.insert(FontIconOption::colorAttr, QColor(Qt::red));

                opt.icon = qTheme.GetFontIcon(is_heart_pressed ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART, font_options);
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
                    const QIcon icon(image_utils::RoundImage(qPixmapCache.GetOrDefault(value.toString()), kPlaylistCoverSize));
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


void PlayListTableView::Search(const QString& keyword) const {
	const QRegularExpression reg_exp(keyword, QRegularExpression::CaseInsensitiveOption);
    proxy_model_->AddFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->setFilterRegularExpression(reg_exp);
}

void PlayListTableView::Reload() {
    // NOTE: 呼叫此函數就會更新index, 會導致playing index失效    
    const QString s = qTEXT(R"(
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
	musics.duration 
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
	musics.track ASC)");

    const QSqlQuery query(s.arg(playlist_id_), qMainDb.database());
    model_->setQuery(query);
    if (model_->lastError().type() != QSqlError::NoError) {
        XAMP_LOG_DEBUG("SqlException: {}", model_->lastError().text().toStdString());
    }
    
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

void PlayListTableView::FastReload() {
    Reload();
}

void PlayListTableView::SetPlaylistId(const int32_t playlist_id, const QString &column_setting_name) {
    playlist_id_ = playlist_id;
    column_setting_name_ = column_setting_name;

    qMainDb.ClearNowPlaying(playlist_id_);

    Reload();

    model_->setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("Id"));
    model_->setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, tr("IsPlaying"));
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
    model_->setHeaderData(PLAYLIST_COVER_ID, Qt::Horizontal, tr(""));
    model_->setHeaderData(PLAYLIST_ARTIST_ID, Qt::Horizontal, tr("ArtistId"));
    model_->setHeaderData(PLAYLIST_HEART, Qt::Horizontal, tr(""));

    auto column_list = qAppSettings.ValueAsStringList(column_setting_name);

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
            PLAYLIST_HEART,
        };

        for (auto column : qAsConst(hidden_columns)) {
            hideColumn(column);
        }

        for (auto column = 0; column < horizontalHeader()->count(); ++column) {
            if (!isColumnHidden(column)) {
                qAppSettings.AddList(column_setting_name_, QString::number(column));
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

void PlayListTableView::SetHeaderViewHidden(bool enable) {
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
    auto hide_this_column_act = action_map.AddAction(tr("Hide this column"), [last_referred_logical_column, this]() {
        setColumnHidden(last_referred_logical_column, true);
    qAppSettings.RemoveList(column_setting_name_, QString::number(last_referred_logical_column));
        });
    hide_this_column_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_HIDE));

    auto select_column_show_act = action_map.AddAction(tr("Select columns to show..."), [pt, header_view, this]() {
        ActionMap<PlayListTableView> action_map(this);
    for (auto column = 0; column < header_view->count(); ++column) {
        auto header_name = model()->headerData(column, Qt::Horizontal).toString();
        action_map.AddAction(header_name, [this, column]() {
            setColumnHidden(column, false);
        qAppSettings.AddList(column_setting_name_, QString::number(column));
            }, false, !isColumnHidden(column));
    }
    action_map.exec(pt);
        });
    select_column_show_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_SHOW));
    action_map.exec(pt);
    });
}

void PlayListTableView::initial() {
    proxy_model_->AddFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->AddFilterByColumn(PLAYLIST_ALBUM);
    proxy_model_->setSourceModel(model_);
    setModel(proxy_model_);

    auto f = font();
#ifdef Q_OS_WIN
    f.setWeight(QFont::Weight::Medium);
#else
    f.setWeight(QFont::Weight::Normal);
#endif
    f.setPointSize(qTheme.GetDefaultFontSize());
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
        PlayItem(index);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);
		
        ActionMap<PlayListTableView> action_map(this);

        auto* load_file_act = action_map.AddAction(tr("Load local file"), [this]() {
            GetOpenMusicFileName(this, [this](const auto& file_name) {
                append(file_name);
                });
            });
        load_file_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_LOAD_FILE));

        auto* load_dir_act = action_map.AddAction(tr("Load file directory"), [this]() {
            const auto dir_name = GetExistingDirectory(this);
            if (dir_name.isEmpty()) {
                return;
            }
            append(dir_name);
            });
        load_dir_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_LOAD_DIR));

        if (enable_delete_ && model()->rowCount() > 0) {
            auto* remove_all_act = action_map.AddAction(tr("Remove all"));
            remove_all_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_REMOVE_ALL));

            action_map.SetCallback(remove_all_act, [this]() {
                if (!model_->rowCount()) {
                    return;
                }

                qMainDb.ClearPendingPlaylist(GetPlaylistId());
                IGNORE_DB_EXCEPTION(qMainDb.RemovePlaylistAllMusic(GetPlaylistId()))
                    Reload();
                RemovePlaying();
            });
        }       

        action_map.AddSeparator();

        auto* read_select_item_replaygain_act = action_map.AddAction(tr("Read file ReplayGain"));
        read_select_item_replaygain_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_READ_REPLAY_GAIN));

        action_map.AddSeparator();
        auto* export_flac_file_act = action_map.AddAction(tr("Export FLAC file"));
        export_flac_file_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_EXPORT_FILE));

        const auto select_row = selectionModel()->selectedRows();
        if (!select_row.isEmpty()) {
            auto* export_aac_file_submenu = action_map.AddSubMenu(tr("Export AAC file"));
            for (const auto& profile : StreamFactory::GetAvailableEncodingProfile()) {
                if (profile.num_channels != AudioFormat::kMaxChannel
                    || profile.sample_rate < AudioFormat::k16BitPCM441Khz.GetSampleRate()
                    || profile.bitrate < kMinimalEncodingBitRate) {
                    continue;
                }

                auto profile_desc = qSTR("%0 bit, %1, %2").arg(
                    QString::number(profile.bit_per_sample),
                    FormatSampleRate(profile.sample_rate),
                    FormatBitRate(profile.bitrate));

                export_aac_file_submenu->AddAction(profile_desc, [profile, this]() {
                    const auto rows = SelectItemIndex();
                    for (const auto& row : rows) {
                        auto entity = this->item(row.second);
                        if (entity.sample_rate != profile.sample_rate) {
                            continue;
                        }
                        emit EncodeAacFile(entity, profile);
                    }
                });
            }
        }
        else {
            action_map.AddAction(tr("Export AAC file"));
        }       

        auto* export_wav_file_act = action_map.AddAction(tr("Export WAV file"));

        action_map.AddSeparator();
        auto * copy_album_act = action_map.AddAction(tr("Copy album"));
        copy_album_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_COPY));

        auto * copy_artist_act = action_map.AddAction(tr("Copy artist"));
        auto * copy_title_act = action_map.AddAction(tr("Copy title"));

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

        action_map.AddSeparator();

        auto item = GetEntity(index);

        auto* add_to_playlist_act = action_map.AddAction(tr("Add file to playlist"));
        add_to_playlist_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_FILE_CIRCLE_PLUS));
        action_map.SetCallback(add_to_playlist_act, [this]() {
            if (const auto other_playlist_id = other_playlist_id_) {
                const auto rows = SelectItemIndex();
                for (const auto& row : rows) {
	                const auto entity = this->item(row.second);
                    qMainDb.AddMusicToPlaylist(entity.music_id, other_playlist_id.value(), entity.album_id);
                }
            }
            });

        auto reload_track_info_act = action_map.AddAction(tr("Reload track information"));
        reload_track_info_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_RELOAD));

        auto* open_local_file_path_act = action_map.AddAction(tr("Open local file path"));
        open_local_file_path_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_OPEN_FILE_PATH));
        action_map.SetCallback(open_local_file_path_act, [item]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(item.parent_path));
        });

        action_map.SetCallback(reload_track_info_act, [this, item]() {
            try {
				qMainDb.AddOrUpdateMusic(GetTrackInfo(item.file_path));
				FastReload();
			}
			catch (std::filesystem::filesystem_error& e) {
				XAMP_LOG_DEBUG("Reload track information error: {}", String::LocaleStringToUTF8(e.what()));
			}
			catch (...) {
			}
        });
    	
        action_map.AddSeparator();
        action_map.SetCallback(copy_album_act, [item]() {
            QApplication::clipboard()->setText(item.album);
        });
        action_map.SetCallback(copy_artist_act, [item]() {
            QApplication::clipboard()->setText(item.artist);
        });
        action_map.SetCallback(copy_title_act, [item]() {
            QApplication::clipboard()->setText(item.title);
        });

        action_map.SetCallback(read_select_item_replaygain_act, [this]() {
            const auto rows = SelectItemIndex();
            QList<PlayListEntity> items;
            for (const auto& row : rows) {
                items.push_front(this->item(row.second));
            }
            emit ReadReplayGain(GetPlaylistId(), items);
        });

        action_map.SetCallback(export_flac_file_act, [this]() {
            const auto rows = SelectItemIndex();
            for (const auto& row : rows) {
                auto entity = this->item(row.second);
                emit EncodeFlacFile(entity);
            }
            });

        action_map.SetCallback(export_wav_file_act, [this]() {
            const auto rows = SelectItemIndex();
            for (const auto& row : rows) {
                auto entity = this->item(row.second);
                emit EncodeWavFile(entity);
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

void PlayListTableView::PauseItem(const QModelIndex& index) {
    const auto entity = item(index);
    CATCH_DB_EXCEPTION(qMainDb.SetNowPlayingState(GetPlaylistId(), entity.playlist_music_id, PlayingState::PLAY_PAUSE))
    update();
}

PlayListEntity PlayListTableView::item(const QModelIndex& index) const {
    return GetEntity(index);
}

void PlayListTableView::PlayItem(const QModelIndex& index) {
    SetNowPlaying(index);
    SetNowPlayState(PLAY_PLAYING);
    const auto play_item = item(index);
    auto [music_id, pending_playlist_id] = qMainDb.GetFirstPendingPlaylistMusic(GetPlaylistId());
    if (play_item.music_id != music_id) {
        AddPendingPlayListFromModel(qAppSettings.ValueAsEnum<PlayerOrder>(kAppSettingOrder));
    }
    emit PlayMusic(play_item);
}

void PlayListTableView::OnThemeColorChanged(QColor /*backgroundColor*/, QColor /*color*/) {
    setStyleSheet(qTEXT("border: none"));
    horizontalHeader()->setStyleSheet(qTEXT("QHeaderView { background-color: transparent; }"));
}

void PlayListTableView::UpdateReplayGain(int32_t playlistId,
    const PlayListEntity& entity,
    double track_loudness,
    double album_rg_gain,
    double album_peak,
    double track_rg_gain,
    double track_peak) {
    if (playlistId != playlist_id_) {
        return;
    }

    CATCH_DB_EXCEPTION(qMainDb.UpdateReplayGain(
        entity.music_id,
        album_rg_gain,
        album_peak, 
        track_rg_gain,
        track_peak))

    CATCH_DB_EXCEPTION(qMainDb.AddOrUpdateTrackLoudness(entity.album_id,
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

    FastReload();
}

void PlayListTableView::keyPressEvent(QKeyEvent *event) {
    QAbstractItemView::keyPressEvent(event);
}

bool PlayListTableView::eventFilter(QObject* obj, QEvent* ev) {
    const auto type = ev->type();
    if (this == obj && type == QEvent::KeyPress) {
	    const auto* event = dynamic_cast<QKeyEvent*>(ev);
        if (event->key() == Qt::Key_Delete && enable_delete_) {
            RemoveSelectItems();
            return true;
        }
    }
    return QAbstractItemView::eventFilter(obj, ev);
}

void PlayListTableView::append(const QString& file_name) {
    auto file_ext = QFileInfo(file_name).completeSuffix();

    if (!file_ext.contains(qTEXT("zip"))) {
        emit ExtractFile(file_name, GetPlaylistId());
    }
    else {
        ZipFileReader reader;
        for (auto filename : reader.OpenFile(file_name.toStdWString())) {
            emit ExtractFile(QString::fromStdWString(filename), GetPlaylistId());
        }
    }
}

void PlayListTableView::ProcessDatabase(int32_t playlist_id, const QList<PlayListEntity>& entities) {
    if (GetPlaylistId() != playlist_id) {
        return;
    }

    for (const auto& entity : entities) {
        CATCH_DB_EXCEPTION(qMainDb.AddMusicToPlaylist(entity.music_id, GetPlaylistId(), entity.album_id))
    }
    Reload();
    emit AddPlaylistItemFinished();
    DeletePendingPlaylist();
}

void PlayListTableView::ProcessTrackInfo(int32_t total_album, int32_t total_tracks) {
    Reload();
    emit AddPlaylistItemFinished();
}

void PlayListTableView::resizeEvent(QResizeEvent* event) {
    QTableView::resizeEvent(event);
    ResizeColumn();
    if (indicator_ != nullptr) {
        CenterParent(indicator_.get());
    }
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

void PlayListTableView::ResizeColumn() {
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

int32_t PlayListTableView::GetPlaylistId() const {
    return playlist_id_;
}

QModelIndex PlayListTableView::GetCurrentIndex() const {
    return play_index_;
}

QModelIndex PlayListTableView::GetFirstIndex() const {
    return model()->index(0, 0);
}

void PlayListTableView::DeletePendingPlaylist() {
    pending_playlist_.clear();
    qMainDb.ClearPendingPlaylist();
}

QList<QModelIndex> PlayListTableView::GetPendingPlayIndexes() const {
    return pending_playlist_;
}

void PlayListTableView::SetOtherPlaylist(int32_t playlist_id) {
    other_playlist_id_ = playlist_id;
}

void PlayListTableView::AddPendingPlayListFromModel(PlayerOrder order) {
    DeletePendingPlaylist();

    QModelIndex index;
    for (auto i = 0; i < proxy_model_->rowCount() && i < kMaxPendingPlayListSize; ++i) {
        switch (order) {
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
            index = GetNextIndex(i);
            break;
        case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
            index = play_index_;
            break;
        case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALL:
            index = GetShuffleIndex();
            break;
        }
        if (!index.isValid()) {
            index = GetFirstIndex();
        }
        pending_playlist_.append(index);
        const auto entity = GetEntity(index);
        try {
            qMainDb.AddPendingPlaylist(entity.playlist_music_id, GetPlaylistId());
        }
        catch (...) {
        }
    }
}

QModelIndex PlayListTableView::GetNextIndex(int forward) const {
    const auto count = proxy_model_->rowCount();
    const auto play_index = GetCurrentIndex();
    const auto next_index = (play_index.row() + forward) % count;
    return model()->index(next_index, PLAYLIST_PLAYING);
}

QModelIndex PlayListTableView::GetShuffleIndex() {
    auto current_playlist_music_id = 0;
    if (play_index_.isValid()) {
        current_playlist_music_id = GetIndexValue(play_index_, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    }
    const auto count = proxy_model_->rowCount();
    if (current_playlist_music_id != 0) {
        rng_.SetSeed(current_playlist_music_id);
    }    
    const auto selected = rng_.NextInt32(0) % count;
    return model()->index(selected, PLAYLIST_PLAYING);
}

void PlayListTableView::SetNowPlaying(const QModelIndex& index, bool is_scroll_to) {
    play_index_ = index;
    setCurrentIndex(play_index_);
    if (is_scroll_to) {
        QTableView::scrollTo(play_index_, PositionAtCenter);
    }
    const auto entity = item(play_index_);
    CATCH_DB_EXCEPTION(qMainDb.ClearNowPlaying(playlist_id_))
	CATCH_DB_EXCEPTION(qMainDb.SetNowPlayingState(playlist_id_, entity.playlist_music_id, PlayingState::PLAY_PLAYING))
    FastReload();
}

void PlayListTableView::SetNowPlayState(PlayingState playing_state) {
    if (model_->rowCount() == 0) {
        return;
    }
    if (!play_index_.isValid()) {
        return;
    }
    const auto entity = item(play_index_);
    CATCH_DB_EXCEPTION(qMainDb.SetNowPlayingState(GetPlaylistId(), entity.playlist_music_id, playing_state))
    FastReload();
    emit UpdatePlayingState(entity, playing_state);
}

void PlayListTableView::ScrollToIndex(const QModelIndex& index) {
    QTableView::scrollTo(index, PositionAtCenter);
}

std::optional<PlayListEntity> PlayListTableView::GetSelectPlayListEntity() const {
    if (const auto select_item = GetSelectItem()) {
        return item(select_item.value());
	}
    return std::nullopt;
}

std::optional<QModelIndex> PlayListTableView::GetSelectItem() const {
    auto select_row = selectionModel()->selectedRows();
    if (select_row.isEmpty()) {
        return std::nullopt;
    }
    return select_row[0];
}

OrderedMap<int32_t, QModelIndex> PlayListTableView::SelectItemIndex() const {
    OrderedMap<int32_t, QModelIndex> select_items;

    Q_FOREACH(auto index, selectionModel()->selectedRows()) {
        auto const row = index.row();
        select_items.emplace(row, index);
    }
    return select_items;
}

void PlayListTableView::SetCurrentPlayIndex(const QModelIndex& index) {
    play_index_ = index;
}

void PlayListTableView::Play(PlayerOrder order) {
    // note: 如果更新UI的話就要重取Index
    //if (pending_playlist_.isEmpty()) {
        AddPendingPlayListFromModel(order);
    //}
    auto [music_id, pending_playlist_id] = qMainDb.GetFirstPendingPlaylistMusic(GetPlaylistId());
    if (music_id == kInvalidDatabaseId || pending_playlist_id == kInvalidDatabaseId) {
        return;
    }
    const auto index = pending_playlist_.front();
    const auto entity = item(index);
    pending_playlist_.pop_front();
    XAMP_EXPECTS(entity.music_id == music_id);
    qMainDb.DeletePendingPlaylistMusic(pending_playlist_id);
    PlayIndex(index);
}

void PlayListTableView::PlayIndex(const QModelIndex& index) {
    play_index_ = index;
    SetNowPlaying(play_index_, true);
    const auto entity = item(play_index_);
    emit PlayMusic(entity);
}

void PlayListTableView::RemovePlaying() {
    CATCH_DB_EXCEPTION(qMainDb.ClearNowPlaying(playlist_id_))
    Reload();
}

void PlayListTableView::RemoveAll() {
    qMainDb.ClearPendingPlaylist(GetPlaylistId());
    IGNORE_DB_EXCEPTION(qMainDb.RemovePlaylistAllMusic(GetPlaylistId()))
    FastReload();
}

void PlayListTableView::RemoveItem(const QModelIndex& index) {
    model_->removeRows(index.row(), 1, index);
}

void PlayListTableView::RemoveSelectItems() {
    const auto rows = SelectItemIndex();

    QVector<int> remove_music_ids;

    for (const auto& row : std::ranges::reverse_view(rows)) {
        const auto it = item(row.second);
        CATCH_DB_EXCEPTION(qMainDb.ClearNowPlaying(playlist_id_, it.playlist_music_id))
        remove_music_ids.push_back(it.music_id);
    }

    const auto count = model_->rowCount();
	if (!count) {
        CATCH_DB_EXCEPTION(qMainDb.ClearNowPlaying(playlist_id_))
	}

    DeletePendingPlaylist();
    CATCH_DB_EXCEPTION(qMainDb.RemovePlaylistMusic(playlist_id_, remove_music_ids))
    Reload();
}
