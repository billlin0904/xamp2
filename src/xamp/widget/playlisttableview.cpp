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
#include <QTimer>

#include <base/logger_impl.h>

#include <widget/playlisttableproxymodel.h>
#include <widget/playlistsqlquerytablemodel.h>
#include <widget/appsettingnames.h>
#include <widget/processindicator.h>
#include <widget/widget_shared.h>
#include <version.h>

#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/http.h>
#include <widget/xmessagebox.h>
#include <widget/appsettings.h>
#include <widget/pixmapcache.h>
#include <widget/stardelegate.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/stareditor.h>
#include <widget/albumentity.h>
#include <widget/podcast_uiltis.h>
#include <widget/ui_utilts.h>
#include <widget/fonticon.h>
#include <widget/playlisttableview.h>

class StyledItemDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    explicit StyledItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (!index.isValid()) {
            return;
        }

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

        opt.decorationSize = QSize(view->columnWidth(index.column()), view->verticalHeader()->defaultSectionSize());
        opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;

        switch (index.column()) {
        case PLAYLIST_TITLE:
        case PLAYLIST_ALBUM:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignLeft;
            opt.text = value.toString();
            break;
        case PLAYLIST_ARTIST:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = value.toString();
            break;
        case PLAYLIST_TRACK:
	        {
                auto is_playing  = index.model()->data(index.model()->index(index.row(), PLAYLIST_PLAYING));
                auto playing_state = is_playing.toInt();
                if (playing_state == PlayingState::PLAY_PLAYING) {
                    opt.decorationSize = QSize(12, 12);
                    opt.icon = qTheme.iconFromFont(Glyphs::ICON_PLAY);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                    opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                }
                else if (playing_state == PlayingState::PLAY_PAUSE) {
                    opt.decorationSize = QSize(12, 12);
                    opt.icon = qTheme.iconFromFont(Glyphs::ICON_PAUSE);
                    opt.features = QStyleOptionViewItem::HasDecoration;
                    opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                    opt.displayAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
                } else {
                    opt.font.setFamily(qTEXT("MonoFont"));
                    opt.text = value.toString();
                }
	        }
            break;
        case PLAYLIST_YEAR:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = QString::number(value.toInt()).rightJustified(8);
            break;
        case PLAYLIST_FILE_SIZE:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = QString::fromStdString(String::FormatBytes(value.toULongLong()));
            break;
        case PLAYLIST_BIT_RATE:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = bitRate2String(value.toInt());
            break;
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_TRACK_PK:
        case PLAYLIST_TRACK_RG:
        case PLAYLIST_TRACK_LOUDNESS:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            switch (index.column()) {
            case PLAYLIST_ALBUM_PK:
            case PLAYLIST_TRACK_PK:
                opt.text = qSTR("%1").arg(value.toFloat(), 4, 'f', 2, QLatin1Char('0'));
                break;
            case PLAYLIST_ALBUM_RG:
            case PLAYLIST_TRACK_RG:
                opt.text = qSTR("%1 dB").arg(value.toFloat(), 4, 'f', 2, QLatin1Char('0'));
                break;
            case PLAYLIST_TRACK_LOUDNESS:
                opt.text = qSTR("%1 LUFS")
                    .arg(value.toFloat(), 4, 'f', 2, QLatin1Char('0'))
                    .rightJustified(8);
                break;
            }
            break;
        case PLAYLIST_SAMPLE_RATE:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = samplerate2String(value.toInt());
            break;
        case PLAYLIST_DURATION:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = streamTimeToString(value.toDouble());
            break;
        case PLAYLIST_LAST_UPDATE_TIME:
            opt.displayAlignment = Qt::AlignVCenter | Qt::AlignRight;
            opt.text = QDateTime::fromSecsSinceEpoch(value.toULongLong()).toString(qTEXT("yyyy-MM-dd HH:mm:ss"));
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
    model.track_loudness    = getIndexValue(index, src, PLAYLIST_TRACK_LOUDNESS).toDouble();
    model.genre             = getIndexValue(index, src, PLAYLIST_GENRE).toString();
    model.year              = getIndexValue(index, src, PLAYLIST_YEAR).toInt();
    return model;
}

void PlayListTableView::executeQuery() {
    // 呼叫此函數就會更新index, 會導致playing index失效
    const QString s = qTEXT(R"(
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
	musicLoudness.track_loudness,
	musics.genre,
	musics.year
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
	playlistMusics.playlistMusicsId, musics.path, albums.album;
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
    proxy_model_->dataChanged(QModelIndex(), QModelIndex());
    update();
}

void PlayListTableView::setPlaylistId(const int32_t playlist_id, const QString &column_setting_name) {
    playlist_id_ = playlist_id;
    column_setting_name_ = column_setting_name;

    qDatabase.clearNowPlaying(playlist_id_);

    executeQuery();

    model_->setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("ID"));
    model_->setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, tr("IS.PLAYING"));
    model_->setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, tr("TRACK#"));
    model_->setHeaderData(PLAYLIST_FILE_PATH, Qt::Horizontal, tr("FILE.PATH"));
    model_->setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, tr("TITLE"));
    model_->setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, tr("FILE.NAME"));
    model_->setHeaderData(PLAYLIST_FILE_SIZE, Qt::Horizontal, tr("FILE.SIZE"));
    model_->setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, tr("ALBUM"));
    model_->setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, tr("ARTIST"));
    model_->setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, tr("DURATION"));
    model_->setHeaderData(PLAYLIST_BIT_RATE, Qt::Horizontal, tr("BIT.RATE"));
    model_->setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, tr("SAMPLE.RATE"));
    model_->setHeaderData(PLAYLIST_RATING, Qt::Horizontal, tr("RATING"));
    model_->setHeaderData(PLAYLIST_ALBUM_RG, Qt::Horizontal, tr("ALBUM.RG"));
    model_->setHeaderData(PLAYLIST_ALBUM_PK, Qt::Horizontal, tr("ALBUM.PK"));
    model_->setHeaderData(PLAYLIST_LAST_UPDATE_TIME, Qt::Horizontal, tr("LAST.UPDATE.TIME"));
    model_->setHeaderData(PLAYLIST_TRACK_RG, Qt::Horizontal, tr("TRACK.RG"));
    model_->setHeaderData(PLAYLIST_TRACK_PK, Qt::Horizontal, tr("TRACK.PK"));
    model_->setHeaderData(PLAYLIST_TRACK_LOUDNESS, Qt::Horizontal, tr("LOUDNESS"));
    model_->setHeaderData(PLAYLIST_GENRE, Qt::Horizontal, tr("GENRE"));
    model_->setHeaderData(PLAYLIST_YEAR, Qt::Horizontal, tr("YEAR"));
    model_->setHeaderData(PLAYLIST_ALBUM_ID, Qt::Horizontal, tr("ALBUM.ID"));
    model_->setHeaderData(PLAYLIST_PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("PLAYLIST.ID"));
    model_->setHeaderData(PLAYLIST_FILE_EXT, Qt::Horizontal, tr("FILE.EXT"));
    model_->setHeaderData(PLAYLIST_FILE_PARENT_PATH, Qt::Horizontal, tr("PARENT.PATH"));
    model_->setHeaderData(PLAYLIST_COVER_ID, Qt::Horizontal, tr("COVER.ID"));
    model_->setHeaderData(PLAYLIST_ARTIST_ID, Qt::Horizontal, tr("ARTIST.ID"));

    auto column_list = AppSettings::getList(column_setting_name);

    if (column_list.empty()) {
        const QList<int> hideColumns{
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
            PLAYLIST_YEAR,
            PLAYLIST_ALBUM_ID,
            PLAYLIST_ARTIST_ID,
            PLAYLIST_COVER_ID,
            PLAYLIST_FILE_EXT,
            PLAYLIST_FILE_PARENT_PATH,
            PLAYLIST_PLAYLIST_MUSIC_ID
        };

        for (auto column : qAsConst(hideColumns)) {
            hideColumn(column);
        }

        for (auto column = 0; column < horizontalHeader()->count(); ++column) {
            if (!isColumnHidden(column)) {
                AppSettings::addList(column_setting_name_, QString::number(column));
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
    setStyleSheet(qTEXT("border : none;"));

    proxy_model_->setSourceModel(model_);
    proxy_model_->setFilterByColumn(PLAYLIST_RATING);
    proxy_model_->setFilterByColumn(PLAYLIST_TITLE);
    proxy_model_->setFilterByColumn(PLAYLIST_DURATION);
    proxy_model_->setFilterByColumn(PLAYLIST_ALBUM);
    proxy_model_->setDynamicSortFilter(true);
    setModel(proxy_model_);

    auto f = font();
    f.setWeight(QFont::Weight::Medium);
    f.setPointSize(qTheme.fontSize());
    setFont(f);

    setUpdatesEnabled(true);
    setAcceptDrops(true);
    setDragEnabled(true);
    setShowGrid(false);
    setMouseTracking(true);
    //setAlternatingRowColors(true);

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

    setEditTriggers(DoubleClicked | SelectedClicked);
    verticalHeader()->setSectionsMovable(false);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    (void)QObject::connect(horizontalHeader(), &QHeaderView::sectionClicked, [this](int logicalIndex) {
        sortByColumn(logicalIndex, Qt::AscendingOrder);
    });

    (void)QObject::connect(this, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        playItem(index);
    });

    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlayListTableView> action_map(this);

        auto* header_view = horizontalHeader();

        auto last_referred_logical_column = header_view->logicalIndexAt(pt);
        auto hide_this_column_act = action_map.addAction(tr("Hide this column"), [last_referred_logical_column, this]() {
            setColumnHidden(last_referred_logical_column, true);
        AppSettings::removeList(column_setting_name_, QString::number(last_referred_logical_column));
            });
        hide_this_column_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_HIDE));

        auto select_column_show_act = action_map.addAction(tr("Select columns to show..."), [pt, header_view, this]() {
            ActionMap<PlayListTableView> action_map(this);
        for (auto column = 0; column < header_view->count(); ++column) {
            auto header_name = model()->headerData(column, Qt::Horizontal).toString();
            action_map.addAction(header_name, [this, column]() {
                setColumnHidden(column, false);
            AppSettings::addList(column_setting_name_, QString::number(column));
                }, false, !isColumnHidden(column));
        }
        action_map.exec(pt);
            });
        select_column_show_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_SHOW));

        action_map.exec(pt);
        });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);

        ActionMap<PlayListTableView> action_map(this);

        if (!podcast_mode_) {
            if (enable_load_file_) {
                auto* load_file_act = action_map.addAction(tr("Load local file"), [this]() {
                    auto reader = MakeMetadataReader();
                QString exts(qTEXT("("));
                for (const auto& file_ext : reader->GetSupportFileExtensions()) {
                    exts += qTEXT("*") + QString::fromStdString(file_ext);
                    exts += qTEXT(" ");
                }
                exts += qTEXT(")");
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
                load_file_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_LOAD_FILE));

                auto* load_dir_act = action_map.addAction(tr("Load file directory"), [this]() {
                    const auto dir_name = QFileDialog::getExistingDirectory(this,
                    tr("Select a Directory"),
                    AppSettings::getMyMusicFolderPath(), QFileDialog::ShowDirsOnly);
                if (dir_name.isEmpty()) {
                    return;
                }
                append(dir_name);
                    });
                load_dir_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_LOAD_DIR));
            }
        }

        if (podcast_mode_) {
            auto* import_podcast_act = action_map.addAction(tr("Download latest podcast"));
            import_podcast_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_DOWNLOAD));
            action_map.setCallback(import_podcast_act, [this]() {
                indicator_ = makeProcessIndicator(this);
				indicator_->startAnimation();
                emit downloadPodcast();
                });
        }

        if (enable_delete_) {
            auto* remove_all_act = action_map.addAction(tr("Remove all"));
            remove_all_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_REMOVE_ALL));

            action_map.setCallback(remove_all_act, [this]() {
                if (!model_->rowCount()) {
                    return;
                }

            const auto button = XMessageBox::showWarning(tr("Are you sure remove all?"),
                kApplicationTitle,
                QDialogButtonBox::No | QDialogButtonBox::Yes,
                QDialogButtonBox::No);
            if (button == QDialogButtonBox::Yes) {
                qDatabase.removePlaylistAllMusic(playlistId());
                executeQuery();
                removePlaying();
            }
            });
        }       

        action_map.addSeparator();

        QAction* reload_metadata_act = nullptr;
        if (!podcast_mode_) {
            reload_metadata_act = action_map.addAction(tr("Reload metadata"));
            reload_metadata_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_RELOAD));
        }

        auto* open_local_file_path_act = action_map.addAction(tr("Open local file path"));
        open_local_file_path_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_OPEN_FILE_PATH));

        auto* read_select_item_replaygain_act = action_map.addAction(tr("Read file ReplayGain"));
        read_select_item_replaygain_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_READ_REPLAY_GAIN));

        action_map.addSeparator();
        auto* export_flac_file_act = action_map.addAction(tr("Export FLAC file"));
        export_flac_file_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_EXPORT_FILE));

        const auto select_row = selectionModel()->selectedRows();
        if (!select_row.isEmpty()) {
            auto* export_aac_file_submenu = action_map.addSubMenu(tr("Export AAC file"));

            for (const auto& profile : StreamFactory::GetAvailableEncodingProfile()) {
                if (profile.num_channels != AudioFormat::kMaxChannel) {
                    continue;
                }
                auto profile_desc = qSTR("%0 bit, %1, %2")
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
        copy_album_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_COPY));

        auto * copy_artist_act = action_map.addAction(tr("Copy artist"));
        auto * copy_title_act = action_map.addAction(tr("Copy title"));

        if (model_->rowCount() == 0 || !index.isValid()) {
            try {
                action_map.exec(pt);
            }
            catch (std::exception const& e) {
                XMessageBox::showError(QString::fromStdString(e.what()));
            }
            return;
        }

        auto item = getEntity(index, proxy_model_->mapToSource(index));

        if (reload_metadata_act != nullptr) {
            action_map.setCallback(reload_metadata_act, [this, item]() {
                try {
                qDatabase.addOrUpdateMusic(getMetadata(item.file_path));
                reload();
            }
            catch (std::filesystem::filesystem_error& e) {
                XAMP_LOG_DEBUG("Reload metadata error: {}", String::LocaleStringToUTF8(e.what()));
            }
            catch (...) {
            }
            });
        }        

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
			ForwardList<PlayListEntity> items;
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
        } catch (std::exception const &e){
            XMessageBox::showError(QString::fromStdString(e.what()));
        }
    });

    const auto *control_A_key = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    (void) QObject::connect(control_A_key, &QShortcut::activated, [this]() {
        selectAll();
    });

    installEventFilter(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    verticalScrollBar()->setStyleSheet(qTEXT(
        "QScrollBar:vertical { width: 6px; }"
    ));
}

void PlayListTableView::pauseItem(const QModelIndex& index) {
    const auto entity = item(index);
    qDatabase.setNowPlayingState(playlistId(), entity.playlist_music_id, PlayingState::PLAY_PAUSE);
    update();
}

PlayListEntity PlayListTableView::item(const QModelIndex& index) {
    return getEntity(index, proxy_model_->mapToSource(index));
}

void PlayListTableView::playItem(const QModelIndex& index) {
    setNowPlaying(index);
    const auto play_item = item(index);
    emit playMusic(play_item);
}

bool PlayListTableView::isPodcastMode() const {
    return podcast_mode_;
}

void PlayListTableView::setPodcastMode(bool enable) {
    podcast_mode_ = enable;
}

void PlayListTableView::onThemeColorChanged(QColor /*backgroundColor*/, QColor /*color*/) {
}

void PlayListTableView::updateReplayGain(int32_t playlistId,
    const PlayListEntity& entity,
    double track_loudness,
    double album_rg_gain,
    double album_peak,
    double track_rg_gain,
    double track_peak) {
    if (playlistId != playlist_id_) {
        return;
    }

    qDatabase.updateReplayGain(
        entity.music_id,
        album_rg_gain,
        album_peak, 
        track_rg_gain,
        track_peak);

    qDatabase.addOrUpdateTrackLoudness(entity.album_id,
        entity.artist_id,
        entity.music_id,
        track_loudness);

    XAMP_LOG_DEBUG("Update DB music id: {} artist id: {} album id id: {}, track_loudness: {:.2f} LUFS album_rg_gain: {:.2f} dB album_peak: {:.2f} track_rg_gain: {:.2f} dB track_peak: {:.2f}",
        entity.music_id,
        entity.artist_id,
        entity.album_id,
        track_loudness,
        album_rg_gain,
        album_peak, 
        track_rg_gain,
        track_peak);

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

    (void)QObject::connect(adapter.get(),
                            &::MetadataExtractAdapter::readCompleted,
                            this,
                            &PlayListTableView::processMeatadata);

    (void)QObject::connect(adapter.get(),
        &::MetadataExtractAdapter::fromDatabase,
        this,
        &PlayListTableView::processDatabase);

   ::MetadataExtractAdapter::readFileMetadata(adapter, file_name, show_progress_dialog, is_recursive);
}

void PlayListTableView::processDatabase(const ForwardList<PlayListEntity>& entities) {
    for (const auto& entity : entities) {
        qDatabase.addMusicToPlaylist(entity.music_id, playlistId(), entity.album_id);
    }
    executeQuery();
    emit addPlaylistItemFinished();
}

void PlayListTableView::processMeatadata(int64_t dir_last_write_time, const ForwardList<TrackInfo>& medata) {
    ::MetadataExtractAdapter::processMetadata(medata, this, dir_last_write_time);
    executeQuery();
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

void PlayListTableView::onDownloadPodcastCompleted(const ForwardList<TrackInfo>& track_infos, const QByteArray& cover_image_data) {
    Stopwatch sw;
    ::MetadataExtractAdapter::processMetadata(track_infos,
        this,
        podcast_mode_);
    XAMP_LOG_DEBUG("Insert database! {}sec", sw.ElapsedSeconds());
    sw.Reset();

    if (!model()->rowCount()) {
        return;
    }

    const auto cover_id = qPixmapCache.addOrUpdate(cover_image_data);
    const auto index = this->model()->index(0, 0);
    const auto play_item = item(index);
    qDatabase.setAlbumCover(play_item.album_id, play_item.album, cover_id);
    emit updateAlbumCover(cover_id);
    XAMP_LOG_DEBUG("Add album cover cache! {}sec", sw.ElapsedSeconds());

    indicator_.reset();
}

void PlayListTableView::resizeColumn() {
    constexpr auto kMaxStretchedSize = 500;
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
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 65);
            break;
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_TRACK_RG:
        case PLAYLIST_TRACK_PK:
        case PLAYLIST_TRACK_LOUDNESS:
        case PLAYLIST_DURATION:
        case PLAYLIST_BIT_RATE:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 80);
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
}

void PlayListTableView::setNowPlayState(PlayingState playing_state) {
    if (!play_index_.isValid()) {
        return;
    }
    const auto entity = item(play_index_);
    qDatabase.setNowPlayingState(playlistId(), entity.playlist_music_id, playing_state);
    reload();
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
    executeQuery();
}

void PlayListTableView::removeAll() {
    qDatabase.removePlaylistAllMusic(playlistId());
    reload();
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
    executeQuery();
}
