#include <widget/playlisttableview.h>

#include <QHeaderView>
#include <QDesktopServices>
#include <QFileDialog>
#include <QClipboard>
#include <QScrollBar>
#include <QShortcut>
#include <QFormLayout>
#include <QTimeEdit>
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
#include <widget/http.h>
#include <widget/xmessagebox.h>
#include <widget/appsettings.h>
#include <widget/imagecache.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/albumentity.h>
#include <widget/podcast_uiltis.h>
#include <widget/ui_utilts.h>
#include <widget/fonticon.h>

class PlayListStyledItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    explicit PlayListStyledItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {
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

#ifdef Q_OS_WIN
        QFont::Weight weight = QFont::Weight::DemiBold;
        switch (qTheme.GetThemeColor()) {
        case ThemeColor::LIGHT_THEME:
            weight = QFont::Weight::Medium;
            break;
        case ThemeColor::DARK_THEME:
            weight = QFont::Weight::DemiBold;
            break;
        }
        opt.font.setWeight(weight);
#endif
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
                    opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                    opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                }
                else if (playing_state == PlayingState::PLAY_PAUSE) {
                    opt.icon = qTheme.PlaylistPauseIcon(icon_size);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                    opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                } else {
                    opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
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
            static constexpr auto kPlaylistHeartSize = QSize(16, 16);
            auto is_heart_pressed = index.model()->data(index.model()->index(index.row(), PLAYLIST_HEART)).toInt();
        	if (is_heart_pressed) {
                QVariantMap font_options;
                font_options.insert(FontIconOption::scaleFactorAttr, QVariant::fromValue(0.4));
                font_options.insert(FontIconOption::colorAttr, QColor(Qt::red));
                opt.icon = qTheme.GetFontIcon(is_heart_pressed ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART, font_options);
                opt.features = QStyleOptionViewItem::HasDecoration;
                opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
        	}
	        }
            break;
        case PLAYLIST_COVER_ID:
	        {
				static constexpr auto kPlaylistCoverSize = QSize(32, 32);
                opt.icon = QIcon(image_utils::RoundImage(qPixmapCache.GetOrDefault(value.toString()), kPlaylistCoverSize));
				opt.features = QStyleOptionViewItem::HasDecoration;
				opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
				opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
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

static PlayListEntity GetEntity(const QModelIndex& index) {
    PlayListEntity entity;
    entity.music_id          = GetIndexValue(index, PLAYLIST_MUSIC_ID).toInt();
    entity.playing           = GetIndexValue(index, PLAYLIST_PLAYING).toInt();
    entity.track             = GetIndexValue(index, PLAYLIST_TRACK).toUInt();
    entity.file_path         = GetIndexValue(index, PLAYLIST_FILE_PATH).toString();
    entity.file_size         = GetIndexValue(index, PLAYLIST_FILE_SIZE).toULongLong();
    entity.title             = GetIndexValue(index, PLAYLIST_TITLE).toString();
    entity.file_name         = GetIndexValue(index, PLAYLIST_FILE_NAME).toString();
    entity.artist            = GetIndexValue(index, PLAYLIST_ARTIST).toString();
    entity.album             = GetIndexValue(index, PLAYLIST_ALBUM).toString();
    entity.duration          = GetIndexValue(index, PLAYLIST_DURATION).toDouble();
    entity.bit_rate          = GetIndexValue(index, PLAYLIST_BIT_RATE).toUInt();
    entity.sample_rate       = GetIndexValue(index, PLAYLIST_SAMPLE_RATE).toUInt();
    entity.rating            = GetIndexValue(index, PLAYLIST_RATING).toUInt();
    entity.album_id          = GetIndexValue(index, PLAYLIST_ALBUM_ID).toInt();
    entity.artist_id         = GetIndexValue(index, PLAYLIST_ARTIST_ID).toInt();
    entity.cover_id          = GetIndexValue(index, PLAYLIST_COVER_ID).toString();
    entity.file_extension    = GetIndexValue(index, PLAYLIST_FILE_EXT).toString();
    entity.parent_path       = GetIndexValue(index, PLAYLIST_FILE_PARENT_PATH).toString();
    entity.timestamp         = GetIndexValue(index, PLAYLIST_LAST_UPDATE_TIME).toULongLong();
    entity.playlist_music_id = GetIndexValue(index, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    entity.album_replay_gain = GetIndexValue(index, PLAYLIST_ALBUM_RG).toDouble();
    entity.album_peak        = GetIndexValue(index, PLAYLIST_ALBUM_PK).toDouble();
    entity.track_replay_gain = GetIndexValue(index, PLAYLIST_TRACK_RG).toDouble();
    entity.track_peak        = GetIndexValue(index, PLAYLIST_TRACK_PK).toDouble();
    entity.track_loudness    = GetIndexValue(index, PLAYLIST_TRACK_LOUDNESS).toDouble();
    entity.genre             = GetIndexValue(index, PLAYLIST_GENRE).toString();
    entity.heart             = GetIndexValue(index, PLAYLIST_HEART).toUInt();
    return entity;
}

void PlayListTableView::Reload() {
    // 呼叫此函數就會更新index, 會導致playing index失效    
    QString s = qTEXT(R"(
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
    musics.bit_rate,
    musics.sample_rate,
    musics.rating,
    albumMusic.albumId,
    albumMusic.artistId,    
	musics.fileExt,
    musics.parentPath,
    musics.dateTime,
	playlistMusics.playlistMusicsId,
    musics.album_replay_gain,
    musics.album_peak,	
    musics.track_replay_gain,
	musics.track_peak,
	musicLoudness.track_loudness,
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
    playlistMusics.playlistId = %1)");

    if (!IsPodcastMode()) {       
        s += qTEXT(R"(
        GROUP BY
        musics.parentPath, musics.track
        ORDER BY
        musics.parentPath ASC, musics.track ASC;
        )");
    }
    else {
        s += qTEXT(R"(
        ORDER BY
        musics.track DESC;
        )");
    }

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
    , model_(new PlayListSqlQueryTableModel(this)) {
    initial();
}

PlayListTableView::~PlayListTableView() = default;

void PlayListTableView::DownloadPodcast() {
    emit FetchPodcast(GetPlaylistId());
}

void PlayListTableView::FastReload() {
    //model_->query().exec();
    //model_->dataChanged(QModelIndex(), QModelIndex());
    //update();
    Reload();
}

void PlayListTableView::SetPlaylistId(const int32_t playlist_id, const QString &column_setting_name) {
    playlist_id_ = playlist_id;
    column_setting_name_ = column_setting_name;

    qMainDb.ClearNowPlaying(playlist_id_);

    Reload();

    model_->setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("Id"));
    model_->setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, tr("IsPlaying"));
    model_->setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, tr("#"));
    model_->setHeaderData(PLAYLIST_FILE_PATH, Qt::Horizontal, tr("FilePath"));
    model_->setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, tr("Title"));
    model_->setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, tr("FileName"));
    model_->setHeaderData(PLAYLIST_FILE_SIZE, Qt::Horizontal, tr("FileSize"));
    model_->setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, tr("Album"));
    model_->setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, tr("Artist"));
    model_->setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, tr("Duraion"));
    model_->setHeaderData(PLAYLIST_BIT_RATE, Qt::Horizontal, tr("Bitrate"));
    model_->setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, tr("Samplerate"));
    model_->setHeaderData(PLAYLIST_RATING, Qt::Horizontal, tr("Rating"));
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
    model_->setHeaderData(PLAYLIST_HEART, Qt::Horizontal, tr("Heart"));

    auto column_list = AppSettings::ValueAsStringList(column_setting_name);

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
            PLAYLIST_RATING,
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
                AppSettings::AddList(column_setting_name_, QString::number(column));
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
    AppSettings::RemoveList(column_setting_name_, QString::number(last_referred_logical_column));
        });
    hide_this_column_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_HIDE));

    auto select_column_show_act = action_map.AddAction(tr("Select columns to show..."), [pt, header_view, this]() {
        ActionMap<PlayListTableView> action_map(this);
    for (auto column = 0; column < header_view->count(); ++column) {
        auto header_name = model()->headerData(column, Qt::Horizontal).toString();
        action_map.AddAction(header_name, [this, column]() {
            setColumnHidden(column, false);
        AppSettings::AddList(column_setting_name_, QString::number(column));
            }, false, !isColumnHidden(column));
    }
    action_map.exec(pt);
        });
    select_column_show_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_SHOW));
    action_map.exec(pt);
    });
}

void PlayListTableView::initial() {
    setStyleSheet(qTEXT("border : none;"));
    setModel(model_);

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
    setAlternatingRowColors(false);

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

        if (!podcast_mode_) {
            if (enable_load_file_) {
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
            }
        }

        if (podcast_mode_) {
            auto* import_podcast_act = action_map.AddAction(tr("Download latest podcast"));
            import_podcast_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_DOWNLOAD));
            action_map.SetCallback(import_podcast_act, [this]() {
                if (model_->rowCount() > 0) {
                    const auto button = XMessageBox::ShowYesOrNo(tr("Download latest podcast before must be remove all,\r\nRemove all items?"));
                    if (button == QDialogButtonBox::Yes) {
                        qMainDb.ClearPendingPlaylist(GetPlaylistId());
                        qMainDb.RemovePlaylistAllMusic(GetPlaylistId());
                        Reload();
                        RemovePlaying();
                    }
                    else {
                        return;
                    }
                }                
                indicator_ = MakeProcessIndicator(this);
				indicator_->StartAnimation();
                DownloadPodcast();
             });
        }

        if (enable_delete_ && model()->rowCount() > 0) {
            auto* remove_all_act = action_map.AddAction(tr("Remove all"));
            remove_all_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_REMOVE_ALL));

            action_map.SetCallback(remove_all_act, [this]() {
                if (!model_->rowCount()) {
                    return;
                }

                const auto button = XMessageBox::ShowYesOrNo(tr("Remove all items?"));
                if (button == QDialogButtonBox::Yes) {
                    qMainDb.ClearPendingPlaylist(GetPlaylistId());
                    IGNORE_DB_EXCEPTION(qMainDb.RemovePlaylistAllMusic(GetPlaylistId()))
                    Reload();
                    RemovePlaying();
                }
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
                XMessageBox::ShowBug(e);
            }
            catch (std::exception const& e) {
                XMessageBox::ShowError(QString::fromStdString(e.what()));
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
        	XMessageBox::ShowBug(e);
        } catch (std::exception const &e){
            XMessageBox::ShowError(QString::fromStdString(e.what()));
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

PlayListEntity PlayListTableView::item(const QModelIndex& index) {
    return GetEntity(index);
}

void PlayListTableView::PlayItem(const QModelIndex& index) {
    SetNowPlaying(index);
    SetNowPlayState(PLAY_PLAYING);
    const auto play_item = item(index);
    auto [music_id, pending_playlist_id] = qMainDb.GetFirstPendingPlaylistMusic(GetPlaylistId());
    if (play_item.music_id != music_id) {
        AddPendingPlayListFromModel(AppSettings::ValueAsEnum<PlayerOrder>(kAppSettingOrder));
    }
    emit PlayMusic(play_item);
}

bool PlayListTableView::IsPodcastMode() const {
    return podcast_mode_;
}

void PlayListTableView::SetPodcastMode(bool enable) {
    podcast_mode_ = enable;
    if (podcast_mode_) {
        setColumnHidden(PLAYLIST_DURATION, true);
        horizontalHeader()->setContextMenuPolicy(Qt::NoContextMenu);
    }
}

void PlayListTableView::OnThemeColorChanged(QColor /*backgroundColor*/, QColor /*color*/) {
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
    if (event->key() == Qt::Key_Return) {
        // we captured the Enter key press, now we need to move to the next row
        auto n_next_row = GetCurrentIndex().row() + 1;
        if (n_next_row + 1 > model()->rowCount(GetCurrentIndex())) {
            // we are all the way down, we can't go any further
            n_next_row = n_next_row - 1;
        }

        if (state() == QAbstractItemView::EditingState) {
            // if we are editing, confirm and move to the row below
            const auto o_next_index = model()->index(n_next_row, GetCurrentIndex().column());
            setCurrentIndex(o_next_index);
            selectionModel()->select(o_next_index, QItemSelectionModel::ClearAndSelect);
        } else {
            // if we're not editing, start editing
            edit(GetCurrentIndex());
        }
    } else {
        // any other key was pressed, inform base class XAMP_WIDGET_SHARED_EXPORT
        QAbstractItemView::keyPressEvent(event);
    }
}

bool PlayListTableView::eventFilter(QObject* obj, QEvent* ev) {
    const auto type = ev->type();
    if (this == obj && type == QEvent::KeyPress) {
	    const auto* event = dynamic_cast<QKeyEvent*>(ev);
        if (event->key() == Qt::Key_Delete) {
            RemoveSelectItems();
            return true;
        }
    }
    return QAbstractItemView::eventFilter(obj, ev);
}

void PlayListTableView::append(const QString& file_name) {
    emit ExtractFile(file_name, GetPlaylistId(), IsPodcastMode());
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

void PlayListTableView::OnFetchPodcastError(const QString& msg) {
    XAMP_LOG_DEBUG("Download podcast error! {}", msg.toStdString());
    XMessageBox::ShowError(qTEXT("Download podcast error!"));
    OnFetchPodcastCompleted({}, {});
}

void PlayListTableView::OnFetchPodcastCompleted(const Vector<TrackInfo>& track_infos, const QByteArray& cover_image_data) {
    XAMP_LOG_DEBUG("Download podcast completed!");   
    indicator_.reset();

    if (track_infos.empty()) {
        return;
    }

    DatabaseFacade facade;
    facade.InsertTrackInfo(track_infos, GetPlaylistId(), true);
    Reload();

    if (!model()->rowCount()) {
        XAMP_LOG_DEBUG("Fail to add the playlist!");
        return;
    }

    QPixmap cover;
    if (cover.loadFromData(cover_image_data)) {
        const auto cover_id = qPixmapCache.AddImage(cover);
        const auto index = this->model()->index(0, 0);
        const auto entity = item(index);
        CATCH_DB_EXCEPTION(qMainDb.SetAlbumCover(entity.album_id, entity.album, cover_id))
        emit UpdateAlbumCover(cover_id);
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
    Reload();
    DeletePendingPlaylist();

    QModelIndex index;
    for (auto i = 0; i < model_->rowCount(); ++i) {
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
        auto entity = GetEntity(index);
        try {
            qMainDb.AddPendingPlaylist(entity.playlist_music_id, GetPlaylistId());
        }
        catch (...) {
        }
    }
}

QModelIndex PlayListTableView::GetNextIndex(int forward) const {
    const auto count = model_->rowCount();
    const auto play_index = GetCurrentIndex();
    const auto next_index = (play_index.row() + forward) % count;
    return model()->index(next_index, PLAYLIST_PLAYING);
}

QModelIndex PlayListTableView::GetShuffleIndex() {
    auto current_playlist_music_id = 0;
    if (play_index_.isValid()) {
        current_playlist_music_id = GetIndexValue(play_index_, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    }
    const auto count = model_->rowCount();
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
    if (pending_playlist_.isEmpty()) {
        AddPendingPlayListFromModel(order);
    }
    auto [music_id, pending_playlist_id] = qMainDb.GetFirstPendingPlaylistMusic(GetPlaylistId());
    if (music_id == kInvalidDatabaseId || pending_playlist_id == kInvalidDatabaseId) {
        return;
    }
    auto index = pending_playlist_.front();
    auto entity = item(index);
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
