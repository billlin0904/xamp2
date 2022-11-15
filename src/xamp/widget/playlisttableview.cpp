#include <QHeaderView>
#include <QDesktopServices>
#include <QFileDialog>
#include <QClipboard>
#include <QScrollBar>
#include <QShortcut>
#include <QFormLayout>
#include <QTimeEdit>
#include <QLineEdit>
#include <QSqlQuery>

#include <widget/playlisttableproxymodel.h>
#include <widget/playlistsqlquerytablemodel.h>
#include <widget/appsettingnames.h>
#include <widget/widget_shared.h>
#include <base/logger_impl.h>

#include <rapidxml.hpp>

#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/http.h>
#include <widget/toast.h>
#include <widget/appsettings.h>
#include <widget/pixmapcache.h>
#include <widget/stardelegate.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/stareditor.h>
#include <widget/albumentity.h>
#include <widget/podcast_uiltis.h>
#include <widget/processindicator.h>
#include <widget/fonticon.h>
#include <widget/playlisttableview.h>

class StyledItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    explicit StyledItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->setRenderHints(QPainter::Antialiasing, true);
        painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

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

        switch (index.column()) {
        case PLAYLIST_DURATION:
        case PLAYLIST_FILE_SIZE:
        case PLAYLIST_BIT_RATE:
        case PLAYLIST_SAMPLE_RATE:
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_TRACK_PK:
        case PLAYLIST_TRACK_RG:
        case PLAYLIST_LAST_UPDATE_TIME:
        case PLAYLIST_YEAR:
            opt.font.setFamily(Q_TEXT("MonoFont"));
            break;
        }

        opt.decorationSize = QSize(view->columnWidth(index.column()), view->verticalHeader()->defaultSectionSize());

        switch (index.column()) {
        case PLAYLIST_TRACK:
	        {
        		opt.rect.setX(8);
                auto is_playing  = index.model()->data(index.model()->index(index.row(), PLAYLIST_PLAYING));
                auto playing_state = is_playing.toInt();
                if (playing_state == PlayingState::PLAY_PLAYING) {
                    opt.decorationSize = QSize(12, 12);
                    opt.icon = qTheme.iconFromFont(IconCode::ICON_Play);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                }
                else if (playing_state == PlayingState::PLAY_PAUSE) {
                    opt.decorationSize = QSize(12, 12);
                    opt.icon = qTheme.iconFromFont(IconCode::ICON_Pause);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                } else {
                    opt.font.setFamily(Q_TEXT("MonoFont"));
                    opt.text = value.toString();
                }                
	        }
            break;
        case PLAYLIST_YEAR:
            opt.text = QString::number(value.toInt()).rightJustified(8);
            break;
        case PLAYLIST_FILE_SIZE:
            opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = QString::fromStdString(String::FormatBytes(value.toULongLong()));
            break;
        case PLAYLIST_BIT_RATE:
            opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = bitRate2String(value.toInt());
            break;
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_TRACK_PK:
        case PLAYLIST_TRACK_RG:
            opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = QString::number(value.toFloat(), 'f', 2);
            break;
        case PLAYLIST_SAMPLE_RATE:
            opt.text = samplerate2String(value.toInt());
            break;
        case PLAYLIST_DURATION:
            opt.text = streamTimeToString(value.toDouble());
            break;
        case PLAYLIST_LAST_UPDATE_TIME:
            opt.text = QDateTime::fromSecsSinceEpoch(value.toULongLong()).toString(Q_TEXT("yyyy-MM-dd HH:mm:ss"));
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

static PlayListEntity getEntity(const QModelIndex& index, const QModelIndex& src) {
    PlayListEntity model;
    model.music_id          = getIndexValue(index, src, PLAYLIST_MUSIC_ID).toInt();
    model.playing           = getIndexValue(index, src, PLAYLIST_PLAYING).toInt();
    model.track             = getIndexValue(index, src, PLAYLIST_TRACK).toInt();
    model.file_path         = getIndexValue(index, src, PLAYLIST_FILE_PATH).toString();
    model.file_size         = getIndexValue(index, src, PLAYLIST_FILE_SIZE).toInt();
    model.title             = getIndexValue(index, src, PLAYLIST_TITLE).toString();
    model.file_name         = getIndexValue(index, src, PLAYLIST_FILE_NAME).toString();
    model.artist            = getIndexValue(index, src, PLAYLIST_ARTIST).toString();
    model.album             = getIndexValue(index, src, PLAYLIST_ALBUM).toString();
    model.duration          = getIndexValue(index, src, PLAYLIST_DURATION).toDouble();
    model.bitrate           = getIndexValue(index, src, PLAYLIST_BIT_RATE).toInt();
    model.samplerate        = getIndexValue(index, src, PLAYLIST_SAMPLE_RATE).toInt();
    model.rating            = getIndexValue(index, src, PLAYLIST_RATING).toInt();
    model.album_id          = getIndexValue(index, src, PLAYLIST_ALBUM_ID).toInt();
    model.artist_id         = getIndexValue(index, src, PLAYLIST_ARTIST_ID).toInt();
    model.cover_id          = getIndexValue(index, src, PLAYLIST_COVER_ID).toString();
    model.file_ext          = getIndexValue(index, src, PLAYLIST_FILE_EXT).toString();
    model.parent_path       = getIndexValue(index, src, PLAYLIST_FILE_PARENT_PATH).toString();
    model.timestamp         = getIndexValue(index, src, PLAYLIST_LAST_UPDATE_TIME).toInt();
    model.playlist_music_id = getIndexValue(index, src, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    model.album_replay_gain = getIndexValue(index, src, PLAYLIST_ALBUM_RG).toDouble();
    model.album_peak        = getIndexValue(index, src, PLAYLIST_ALBUM_PK).toDouble();
    model.track_replay_gain = getIndexValue(index, src, PLAYLIST_TRACK_RG).toDouble();
    model.track_peak        = getIndexValue(index, src, PLAYLIST_TRACK_PK).toDouble();
    model.genre             = getIndexValue(index, src, PLAYLIST_GENRE).toString();
    model.year              = getIndexValue(index, src, PLAYLIST_YEAR).toInt();
    return model;
}

void PlayListTableView::excuteQuery() {
    // 呼叫此函數就會更新index, 會導致playing index失效
    const QString s = Q_TEXT(R"(
    SELECT
    musics.musicId,
    playlistMusics.playing,
    musics.track,
    musics.path,
	musics.fileSize,
    musics.title,
    musics.fileName,
    artists.artist,
    albums.album,
    musics.duration,
    musics.bitrate,
    musics.samplerate,
    musics.rating,
    albumMusic.albumId,
    albumMusic.artistId,
    albums.coverId,
	musics.fileExt,
    musics.parentPath,
    musics.dateTime,
	playlistMusics.playlistMusicsId,
    musics.album_replay_gain,
    musics.album_peak,	
    musics.track_replay_gain,
	musics.track_peak,
	musics.genre,
	musics.year
    FROM
    playlistMusics
    JOIN playlist ON playlist.playlistId = playlistMusics.playlistId
    JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId
    JOIN musics ON playlistMusics.musicId = musics.musicId
    JOIN albums ON albumMusic.albumId = albums.albumId
    JOIN artists ON albumMusic.artistId = artists.artistId
    WHERE
    playlistMusics.playlistId = %1
	ORDER BY
	playlistMusics.playlistMusicsId, musics.path;
    )");
    QSqlQuery query(s.arg(playlist_id_));
    model_->setQuery(query);
    while (model_->canFetchMore()) {
        model_->fetchMore();
    }
    proxy_model_->dataChanged(QModelIndex(), QModelIndex());
}

PlayListTableView::PlayListTableView(QWidget* parent, int32_t playlist_id)
    : QTableView(parent)
    , podcast_mode_(false)
    , playlist_id_(playlist_id)
    , start_delegate_(nullptr)
    , model_(new PlayListSqlQueryTableModel(this))
    , proxy_model_(new PlayListTableFilterProxyModel(this)) {
    initial();
}

PlayListTableView::~PlayListTableView() = default;

void PlayListTableView::reload() {
    model_->query().exec();
}

void PlayListTableView::setPlaylistId(const int32_t playlist_id) {
    playlist_id_ = playlist_id;

    qDatabase.clearNowPlaying(playlist_id_);

    excuteQuery();

    model_->setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("ID"));
    model_->setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, tr("IS.PLAYING"));
    model_->setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, tr("TRACK#"));
    model_->setHeaderData(PLAYLIST_FILE_PATH, Qt::Horizontal, tr("FILE PATH"));
    model_->setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, tr("TITLE"));
    model_->setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, tr("FILE NAME"));
    model_->setHeaderData(PLAYLIST_FILE_SIZE, Qt::Horizontal, tr("FILE SIZE"));
    model_->setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, tr("ALBUM"));
    model_->setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, tr("ARTIST"));
    model_->setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, tr("DURATION"));
    model_->setHeaderData(PLAYLIST_BIT_RATE, Qt::Horizontal, tr("BIT.RATE"));
    model_->setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, tr("SAMPLE.RATE"));
    model_->setHeaderData(PLAYLIST_RATING, Qt::Horizontal, tr("RATING"));
    model_->setHeaderData(PLAYLIST_ALBUM_RG, Qt::Horizontal, tr("ALBUM.RG"));
    model_->setHeaderData(PLAYLIST_ALBUM_PK, Qt::Horizontal, tr("ALBUM.PK"));
    model_->setHeaderData(PLAYLIST_LAST_UPDATE_TIME, Qt::Horizontal, tr("LAST UPDATE TIME"));
    model_->setHeaderData(PLAYLIST_TRACK_RG, Qt::Horizontal, tr("TRACK.RG"));
    model_->setHeaderData(PLAYLIST_TRACK_PK, Qt::Horizontal, tr("TRACK.PK"));
    model_->setHeaderData(PLAYLIST_GENRE, Qt::Horizontal, tr("GENRE"));
    model_->setHeaderData(PLAYLIST_YEAR, Qt::Horizontal, tr("YEAR"));

    hideColumn(PLAYLIST_PLAYING);
    hideColumn(PLAYLIST_MUSIC_ID);
    hideColumn(PLAYLIST_FILE_PATH);
    hideColumn(PLAYLIST_FILE_NAME);
    hideColumn(PLAYLIST_FILE_SIZE);
    hideColumn(PLAYLIST_ALBUM_ID);
    hideColumn(PLAYLIST_ARTIST_ID);
    hideColumn(PLAYLIST_COVER_ID);
    hideColumn(PLAYLIST_FILE_EXT);
    hideColumn(PLAYLIST_FILE_PARENT_PATH);
    hideColumn(PLAYLIST_BIT_RATE);
    hideColumn(PLAYLIST_ALBUM);
    hideColumn(PLAYLIST_LAST_UPDATE_TIME);
    hideColumn(PLAYLIST_RATING);
    hideColumn(PLAYLIST_PLAYLIST_MUSIC_ID);
    hideColumn(PLAYLIST_ALBUM_RG);
    hideColumn(PLAYLIST_ALBUM_PK);
    hideColumn(PLAYLIST_TRACK_RG);
    hideColumn(PLAYLIST_TRACK_PK);
    hideColumn(PLAYLIST_GENRE);
    hideColumn(PLAYLIST_YEAR);

    if (isPodcastMode()) {
        return;
    }

    auto column_list = AppSettings::getList(columnAppSettingName());

    if (column_list.empty()) {
        for (auto column = 0; column < horizontalHeader()->count(); ++column) {
            if (!isColumnHidden(column)) {
                AppSettings::addList(columnAppSettingName(), QString::number(column));
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

void PlayListTableView::initial() {
    setStyleSheet(Q_TEXT("border : none;"));

    notshow_column_names_.insert(Q_TEXT("albumId"));
    notshow_column_names_.insert(Q_TEXT("artistId"));
    notshow_column_names_.insert(Q_TEXT("coverId"));
    notshow_column_names_.insert(Q_TEXT("fileExt"));
    notshow_column_names_.insert(Q_TEXT("parentPath"));
    notshow_column_names_.insert(Q_TEXT("playlistMusicsId"));

    proxy_model_->setSourceModel(model_);
    proxy_model_->setFilterByColumn(PLAYLIST_RATING);
    proxy_model_->setFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->setDynamicSortFilter(true);
    setModel(proxy_model_);

    auto f = font();
    f.setWeight(QFont::Weight::Normal);
    f.setPointSize(qTheme.fontSize());
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
    verticalHeader()->setDefaultSectionSize(40);

    horizontalScrollBar()->setDisabled(true);

    horizontalHeader()->setVisible(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    setItemDelegate(new StyledItemDelegate(this));

    start_delegate_ = new StarDelegate(this);
    setItemDelegateForColumn(PLAYLIST_RATING, start_delegate_);
    //setItemDelegateForColumn(PLAYLIST_TRACK, new AlignCenterStyledItemDelegate(this));

    (void)QObject::connect(start_delegate_, &StarDelegate::commitData, [this](auto editor) {
        auto start_editor = qobject_cast<StarEditor*>(editor);
        if (!start_editor) {
            return;
        }
        auto index = model()->index(start_editor->row(), PLAYLIST_RATING);

        const auto src_index = proxy_model_->mapToSource(index);
        auto item = getEntity(index, src_index);
        item.rating = start_editor->starRating().starCount();
        qDatabase.updateMusicRating(item.music_id, item.rating);
        reload();
    });

    (void)QObject::connect(this, &QTableView::entered, [this](auto index) {
        if (index.column() == PLAYLIST_TRACK) {
            setCursor(Qt::PointingHandCursor);
        }
        else {
            setCursor(Qt::ArrowCursor);
        }
        });

    setEditTriggers(DoubleClicked | SelectedClicked);
    verticalHeader()->setSectionsMovable(false);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    (void)QObject::connect(horizontalHeader(), &QHeaderView::sectionClicked, [this](int logicalIndex) {
        sortByColumn(logicalIndex, Qt::AscendingOrder);
    });

    (void)QObject::connect(this, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        playItem(index);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);

        ActionMap<PlayListTableView> action_map(this);

        if (!podcast_mode_) {
            auto* load_file_act = action_map.addAction(tr("Load local file"), [this]() {
                auto reader = MakeMetadataReader();
                QString exts(Q_TEXT("("));
                for (const auto& file_ext : reader->GetSupportFileExtensions()) {
                    exts += Q_TEXT("*") + QString::fromStdString(file_ext);
                    exts += Q_TEXT(" ");
                }
                exts += Q_TEXT(")");
                const auto file_name = QFileDialog::getOpenFileName(this,
                    tr("Open file"),
                    AppSettings::getMyMusicFolderPath(),
                    tr("Music Files ") + exts,
                    nullptr);
                if (file_name.isEmpty()) {
                    return;
                }
                append(file_name);
                });
            load_file_act->setIcon(qTheme.iconFromFont(IconCode::ICON_LoadFile));

            auto* load_dir_act = action_map.addAction(tr("Load file directory"), [this]() {
                const auto dir_name = QFileDialog::getExistingDirectory(this,
                    tr("Select a Directory"),
                    AppSettings::getMyMusicFolderPath(), QFileDialog::ShowDirsOnly);
                if (dir_name.isEmpty()) {
                    return;
                }
                append(dir_name);
                });
            load_dir_act->setIcon(qTheme.iconFromFont(IconCode::ICON_LoadDir));

            action_map.addSeparator();
        }

        if (podcast_mode_) {
            auto* import_podcast_act = action_map.addAction(tr("Download latest podcast"));
            import_podcast_act->setIcon(qTheme.iconFromFont(IconCode::ICON_Download));
            action_map.setCallback(import_podcast_act, [this]() {
                importPodcast();
                });
        }

        auto* reload_metadata_act = action_map.addAction(tr("Reload metadata"));
        reload_metadata_act->setIcon(qTheme.iconFromFont(IconCode::ICON_Reload));

        auto* remove_all_act = action_map.addAction(tr("Remove all"));
        remove_all_act->setIcon(qTheme.iconFromFont(IconCode::ICON_RemoveAll));

        auto* open_local_file_path_act = action_map.addAction(tr("Open local file path"));
        open_local_file_path_act->setIcon(qTheme.iconFromFont(IconCode::ICON_OpenFilePath));

        auto* read_select_item_replaygain_act = action_map.addAction(tr("Read replay gain"));
        read_select_item_replaygain_act->setIcon(qTheme.iconFromFont(IconCode::ICON_ReadReplayGain));

        action_map.addSeparator();
        auto* export_flac_file_act = action_map.addAction(tr("Export FLAC file"));
        export_flac_file_act->setIcon(qTheme.iconFromFont(IconCode::ICON_ExportFile));

        const auto select_row = selectionModel()->selectedRows();
        if (!select_row.isEmpty()) {
            auto* export_aac_file_submenu = action_map.addSubMenu(tr("Export AAC file"));

            for (const auto& profile : DspComponentFactory::GetAvailableEncodingProfile()) {
                auto profile_desc = Q_STR("%0 bit, %1, %2")
                    .arg(profile.bit_per_sample).rightJustified(2)
                    .arg(samplerate2String(profile.sample_rate))
                    .arg(bitRate2String(profile.bitrate));

                export_aac_file_submenu->addAction(profile_desc, [profile, this]() {
                    const auto rows = selectItemIndex();
                    for (const auto& row : rows) {
                        auto entity = this->item(row.second);
                        if (entity.samplerate > AudioFormat::k16BitPCM441Khz.GetSampleRate()) {
                            continue;
                        }
                        emit encodeAACFile(entity, profile);
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
        copy_album_act->setIcon(qTheme.iconFromFont(IconCode::ICON_Copy));

        auto * copy_artist_act = action_map.addAction(tr("Copy artist"));
        auto * copy_title_act = action_map.addAction(tr("Copy title"));

        if (model_->rowCount() == 0 || !index.isValid()) {
            try {
                action_map.exec(pt);
            }
            catch (std::exception const& e) {
                Toast::showTip(QString::fromStdString(e.what()), this);
            }
            return;
        }

        auto item = getEntity(index, proxy_model_->mapToSource(index));

        action_map.setCallback(reload_metadata_act, [this, item]() {
            qDatabase.addOrUpdateMusic(getMetadata(item.file_path));
            reload();
        });

        action_map.setCallback(remove_all_act, [this]() {
            qDatabase.removePlaylistAllMusic(playlistId());
            excuteQuery();
            removePlaying();
        });

        action_map.setCallback(open_local_file_path_act, [item]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(item.parent_path));
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

        action_map.setCallback(read_select_item_replaygain_act, [this]() {
            const auto rows = selectItemIndex();
            Vector<PlayListEntity> items;
            for (const auto& row : rows) {
                items.push_back(this->item(row.second));
            }
            emit addPlaylistReplayGain(true, items);
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
        } catch (std::exception const &e){
            Toast::showTip(QString::fromStdString(e.what()), this);
        }
    });

    const auto *control_A_key = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    (void) QObject::connect(control_A_key, &QShortcut::activated, [this]() {
        selectAll();
    });

    installEventFilter(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalScrollBar()->setStyleSheet(Q_TEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));
}

void PlayListTableView::pauseItem(const QModelIndex& index) {
    const auto entity = item(index);
    qDatabase.setNowPlayingState(playlistId(), entity.playlist_music_id, PlayingState::PLAY_PAUSE);
    //reload();
    update();
}

PlayListEntity PlayListTableView::item(const QModelIndex& index) {
    const auto src_index = proxy_model_->mapToSource(index);
    return getEntity(index, src_index);
}

void PlayListTableView::playItem(const QModelIndex& index) {
    setNowPlaying(index);
    const auto play_item = item(index);
    emit playMusic(play_item);
}

bool PlayListTableView::isPodcastMode() const {
    return podcast_mode_;
}

ConstLatin1String PlayListTableView::columnAppSettingName() const {
	const auto setting_name = isPodcastMode() ? kAppSettingPodcastPlaylistColumnName : kAppSettingPlaylistColumnName;
    return setting_name;
}

void PlayListTableView::setPodcastMode(bool enable) {
    podcast_mode_ = enable;

	if (podcast_mode_) {
        hideColumn(PLAYLIST_FILE_SIZE);
        hideColumn(PLAYLIST_TRACK_RG);
        hideColumn(PLAYLIST_TRACK_PK);
        hideColumn(PLAYLIST_ARTIST);
        hideColumn(PLAYLIST_ALBUM_RG);
        hideColumn(PLAYLIST_ALBUM_PK);
        hideColumn(PLAYLIST_DURATION);
        hideColumn(PLAYLIST_SAMPLE_RATE);
        hideColumn(PLAYLIST_GENRE);
        hideColumn(PLAYLIST_YEAR);
        hideColumn(PLAYLIST_BIT_RATE);
        hideColumn(PLAYLIST_LAST_UPDATE_TIME);
    }

    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, [this](auto pt) {
        
        ActionMap<PlayListTableView> action_map(this);

        auto* header_view = horizontalHeader();

        auto last_referred_logical_column = header_view->logicalIndexAt(pt);
        auto hide_this_column_act = action_map.addAction(tr("Hide this column"), [last_referred_logical_column, this]() {
            setColumnHidden(last_referred_logical_column, true);
            AppSettings::removeList(columnAppSettingName(), QString::number(last_referred_logical_column));
            });
        hide_this_column_act->setIcon(qTheme.iconFromFont(IconCode::ICON_Hide));

        auto select_column_show_act = action_map.addAction(tr("Select columns to show..."), [pt, header_view, this]() {
            ActionMap<PlayListTableView> action_map(this);
            for (auto column = 0; column < header_view->count(); ++column) {
                auto header_name = model()->headerData(column, Qt::Horizontal).toString();
                if (notshow_column_names_.contains(header_name)) {
                    continue;
                }
                action_map.addAction(header_name, [this, column]() {
                    setColumnHidden(column, false);
                    AppSettings::addList(columnAppSettingName(), QString::number(column));
                    }, false, !isColumnHidden(column));
            }
            action_map.exec(pt);
            });
        select_column_show_act->setIcon(qTheme.iconFromFont(IconCode::ICON_Show));

        action_map.exec(pt);
        });
}

void PlayListTableView::onThemeColorChanged(QColor backgroundColor, QColor color) {
    //setStyleSheet(backgroundColorToString(backgroundColor));
}

void PlayListTableView::updateReplayGain(int music_id,
    double album_rg_gain,
    double album_peak,
    double track_rg_gain,
    double track_peak) {
    qDatabase.updateReplayGain(
        music_id, album_rg_gain, album_peak, track_rg_gain, track_peak);
    XAMP_LOG_DEBUG("Update DB music id : {}, album_rg_gain: {:.2f} album_peak: {:.2f} track_rg_gain: {:.2f} track_peak: {:.2f}",
        music_id, album_rg_gain, album_peak, track_rg_gain, track_peak);
    reload();
}

void PlayListTableView::keyPressEvent(QKeyEvent *pEvent) {
    if (pEvent->key() == Qt::Key_Return) {
        // we captured the Enter key press, now we need to move to the next row
        auto n_next_row = currentIndex().row() + 1;
        if (n_next_row + 1 > model()->rowCount(currentIndex())) {
            // we are all the way down, we can't go any further
            n_next_row = n_next_row - 1;
        }

        if (state() == QAbstractItemView::EditingState) {
            // if we are editing, confirm and move to the row below
            const auto o_next_index = model()->index(n_next_row, currentIndex().column());
            setCurrentIndex(o_next_index);
            selectionModel()->select(o_next_index, QItemSelectionModel::ClearAndSelect);
        } else {
            // if we're not editing, start editing
            edit(currentIndex());
        }
    } else {
        // any other key was pressed, inform base class
        QAbstractItemView::keyPressEvent(pEvent);
    }
}

bool PlayListTableView::eventFilter(QObject* obj, QEvent* ev) {
    const auto type = ev->type();
    if (this == obj && type == QEvent::KeyPress) {
	    const auto* event = dynamic_cast<QKeyEvent*>(ev);
        if (event->key() == Qt::Key_Delete) {
            removeSelectItems();
            return true;
        }
    }
    return QAbstractItemView::eventFilter(obj, ev);
}

void PlayListTableView::append(const QString& file_name, bool show_progress_dialog, bool is_recursive) {
	const auto adapter = QSharedPointer<::MetadataExtractAdapter>(new ::MetadataExtractAdapter());

    (void) QObject::connect(adapter.get(),
                            &::MetadataExtractAdapter::readCompleted,
                            this,
                            &PlayListTableView::processMeatadata);

   ::MetadataExtractAdapter::readFileMetadata(adapter, file_name, show_progress_dialog, is_recursive);
}

void PlayListTableView::processMeatadata(int64_t dir_last_write_time, const ForwardList<Metadata>& medata) {
    ::MetadataExtractAdapter::processMetadata(medata, this, dir_last_write_time);
    resizeColumn();
    excuteQuery();
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

void PlayListTableView::importPodcast() {
    QSharedPointer<ProcessIndicator> indicator(new ProcessIndicator(this), &QObject::deleteLater);
    XAMP_LOG_DEBUG("Start download podcast.xml");

    indicator->startAnimation();

    http::HttpClient(Q_TEXT("https://suisei.moe/podcast.xml"))
	.error([indicator](const QString& msg) {
        XAMP_LOG_DEBUG("Download podcast error! {}", msg.toStdString());
	})
	.success([indicator, this](const QString& json) {
        XAMP_LOG_DEBUG("Download podcast.xml success!");

		Stopwatch sw;
        auto const podcast_info = parsePodcastXML(json);
        XAMP_LOG_DEBUG("Parse podcast.xml success! {}sec", sw.ElapsedSeconds());

        sw.Reset();
        ::MetadataExtractAdapter::processMetadata(
            podcast_info.second,
            this,
            podcast_mode_);
        XAMP_LOG_DEBUG("Insert database! {}sec", sw.ElapsedSeconds());

        XAMP_LOG_DEBUG("Start download podcast image file");        
        http::HttpClient(QString::fromStdString(podcast_info.first))
		.error([indicator](const QString& msg) {
            XAMP_LOG_DEBUG("Download podcast error! {}", msg.toStdString());
        })
    	.download([indicator, this](const QByteArray& data) {
            if (!model()->rowCount()) {
                return;
            }
            XAMP_LOG_DEBUG("Download podcast image file success!");
            const auto cover_id = qPixmapCache.addOrUpdate(data);
            const auto index = this->model()->index(0, 0);
            const auto play_item = item(index);
            qDatabase.setAlbumCover(play_item.album_id, play_item.album, cover_id);
            emit updateAlbumCover(cover_id);
            });
        }).get();
}

void PlayListTableView::resizeColumn() {
    constexpr auto kMaxStretchedSize = 900;
	auto* header = horizontalHeader();
    auto not_hide_column = 0;

    for (auto column = 0; column < header->count(); ++column) {
        if (!isColumnHidden(column)) {
            ++not_hide_column;
        }

	    switch (column) {
        case PLAYLIST_PLAYING:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            header->resizeSection(column, 25);
            break;
        case PLAYLIST_TRACK:
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_TRACK_RG:
        case PLAYLIST_TRACK_PK:
        case PLAYLIST_DURATION:
        case PLAYLIST_BIT_RATE:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 40);
            break;
        case PLAYLIST_TITLE:
            header->resizeSection(column,
                (std::max)(sizeHintForColumn(column), kMaxStretchedSize));
        	break;        
        case PLAYLIST_ARTIST:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 300);
            break;
        case PLAYLIST_ALBUM:
        default:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        }
    }

    if (not_hide_column == 3) {
        header->setSectionResizeMode(PLAYLIST_DURATION, QHeaderView::Fixed);
        header->resizeSection(PLAYLIST_DURATION, 30);
    }
}

void PlayListTableView::search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax) {
    const QRegExp reg_exp(sort_str, case_sensitivity, pattern_syntax);

    proxy_model_->setFilterRegExp(reg_exp);
    proxy_model_->invalidate();
}

int32_t PlayListTableView::playlistId() const {
    return playlist_id_;
}

QModelIndex PlayListTableView::currentIndex() const {
    return play_index_;
}

QModelIndex PlayListTableView::nextIndex(int forward) const {
    const auto count = proxy_model_->rowCount();
    const auto play_index = currentIndex();
    const auto next_index = (play_index.row() + forward) % count;
    return model()->index(next_index, PLAYLIST_PLAYING);
}

QModelIndex PlayListTableView::shuffeIndex() {    
    const auto count = proxy_model_->rowCount();
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
    qDatabase.clearNowPlaying(playlist_id_);
    qDatabase.setNowPlayingState(playlist_id_, entity.playlist_music_id, PlayingState::PLAY_PLAYING);
    reload();
    update();
}

void PlayListTableView::setNowPlayState(PlayingState playing_state) {
    if (!play_index_.isValid()) {
        return;
    }
    const auto entity = item(play_index_);
    qDatabase.setNowPlayingState(playlistId(), entity.playlist_music_id, playing_state);
    reload();
    update();
}

void PlayListTableView::scrollToIndex(const QModelIndex& index) {
    QTableView::scrollTo(index, PositionAtCenter);
}

std::optional<QModelIndex> PlayListTableView::selectItem() const {
    auto select_row = selectionModel()->selectedRows();
    if (select_row.isEmpty()) {
        return std::nullopt;
    }
    return select_row[0];
}

std::map<int32_t, QModelIndex> PlayListTableView::selectItemIndex() const {
    std::map<int32_t, QModelIndex> select_items;

    Q_FOREACH(auto index, selectionModel()->selectedRows()) {
        auto const row = index.row();
        select_items.emplace(row, index);
    }
    return select_items;
}


void PlayListTableView::setCurrentPlayIndex(const QModelIndex& index) {
    play_index_ = index;
}

void PlayListTableView::play(const QModelIndex& index) {
    play_index_ = index;
    setNowPlaying(play_index_, true);
    const auto entity = item(play_index_);
    emit playMusic(entity);
}

void PlayListTableView::removePlaying() {
    qDatabase.clearNowPlaying(playlist_id_);
    excuteQuery();
}

void PlayListTableView::removeAll() {
    qDatabase.removePlaylistAllMusic(playlistId());
    update();
}

void PlayListTableView::removeItem(const QModelIndex& index) {
    proxy_model_->removeRows(index.row(), 1, index);
}

void PlayListTableView::removeSelectItems() {
    const auto rows = selectItemIndex();

    QVector<int> remove_music_ids;

    for (auto itr = rows.rbegin(); itr != rows.rend(); ++itr) {
        const auto it = item((*itr).second);
        qDatabase.clearNowPlaying(playlist_id_, it.playlist_music_id);
        remove_music_ids.push_back(it.playlist_music_id);
    }

    const auto count = proxy_model_->rowCount();
	if (!count) {
        qDatabase.clearNowPlaying(playlist_id_);       
	}

    qDatabase.removePlaylistMusic(playlist_id_, remove_music_ids);
    excuteQuery();
}
