#include <QHeaderView>
#include <QDesktopServices>
#include <QFileDialog>
#include <QClipboard>
#include <QScrollBar>
#include <QShortcut>
#include <QFormLayout>
#include <QTimeEdit>
#include <QLineEdit>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QXmlStreamReader>
#include <QStorageInfo>

#include <base/rng.h>
#include <base/str_utilts.h>
#include <stream/icddevice.h>
#include <player/api.h>

#include <metadata/api.h>
#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <rapidxml.hpp>

#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/http.h>
#include <widget/toast.h>
#include <widget/image_utiltis.h>
#include <widget/appsettings.h>
#include <widget/pixmapcache.h>
#include <widget/stardelegate.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/stareditor.h>
#include <widget/albumentity.h>
#include <widget/podcast_uiltis.h>
#include <widget/playlisttableview.h>

using namespace xamp::metadata;

static PlayListEntity getEntity(const QModelIndex& index) {
    PlayListEntity entity;
    entity.music_id = getIndexValue(index, PLAYLIST_MUSIC_ID).toInt();
    entity.track = getIndexValue(index, PLAYLIST_TRACK).toUInt();
    entity.playing = getIndexValue(index, PLAYLIST_PLAYING).toBool();
    entity.file_path = getIndexValue(index, PLAYLIST_FILEPATH).toString();
    entity.title = getIndexValue(index, PLAYLIST_TITLE).toString();
    entity.file_name = getIndexValue(index, PLAYLIST_FILE_NAME).toString();
    entity.artist = getIndexValue(index, PLAYLIST_ARTIST).toString();
    entity.album = getIndexValue(index, PLAYLIST_ALBUM).toString();
    entity.duration = getIndexValue(index, PLAYLIST_DURATION).toDouble();
    entity.bitrate = getIndexValue(index, PLAYLIST_BITRATE).toUInt();
    entity.samplerate = getIndexValue(index, PLAYLIST_SAMPLE_RATE).toUInt();
    entity.rating = getIndexValue(index, PLAYLIST_RATING).toUInt();
    entity.album_id = getIndexValue(index, PLAYLIST_ALBUM_ID).toInt();
    entity.artist_id = getIndexValue(index, PLAYLIST_ARTIST_ID).toInt();
    entity.cover_id = getIndexValue(index, PLAYLIST_COVER_ID).toString();
    entity.file_ext = getIndexValue(index, PLAYLIST_FILE_EXT).toString();
    entity.parent_path = getIndexValue(index, PLAYLIST_FILE_PARENT_PATH).toString();
    entity.timestamp = getIndexValue(index, PLAYLIST_LAST_UPDATE_TIME).toULongLong();
    entity.playlist_music_id = getIndexValue(index, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    entity.album_replay_gain = getIndexValue(index, PLAYLIST_ALBUM_RG).toDouble();
    entity.album_peak = getIndexValue(index, PLAYLIST_ALBUM_PK).toDouble();
    entity.track_replay_gain = getIndexValue(index, PLAYLIST_TRACK_RG).toDouble();
    entity.track_peak = getIndexValue(index, PLAYLIST_TRACK_PK).toDouble();
    return entity;
}

PlayListTableView::PlayListTableView(QWidget* parent, int32_t playlist_id)
    : QTableView(parent)
    , podcast_mode_(false)
    , playlist_id_(playlist_id)
    , start_delegate_(nullptr)
    , model_(this)
    , proxy_model_(this) {
    initial();
}

PlayListTableView::~PlayListTableView() = default;

void PlayListTableView::refresh() {
    model_.query().exec();
}

void PlayListTableView::updateData() {
    const QString s = Q_UTF8(R"(
    SELECT
    musics.musicId,
    playlistMusics.playing,
    musics.track,
    musics.path,
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
    model_.setQuery(query);
    while (model_.canFetchMore()) {
        model_.fetchMore();
    }
    proxy_model_.dataChanged(QModelIndex(), QModelIndex());
}

void PlayListTableView::setPlaylistId(const int32_t playlist_id) {
    playlist_id_ = playlist_id;

    SharedSingleton<Database>::GetInstance().clearNowPlaying(playlist_id_);

    updateData();

    model_.setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("ID"));
    model_.setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, tr(" "));
    model_.setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, tr("TRACK#"));
    model_.setHeaderData(PLAYLIST_FILEPATH, Qt::Horizontal, tr("FILE PATH"));
    model_.setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, tr("TITLE"));
    model_.setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, tr("FILE NAME"));
    model_.setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, tr("ALBUM"));
    model_.setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, tr("ARTIST"));
    model_.setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, tr("DURATION"));
    model_.setHeaderData(PLAYLIST_BITRATE, Qt::Horizontal, tr("BIT.RATE"));
    model_.setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, tr("SAMPLE.RATE"));
    model_.setHeaderData(PLAYLIST_RATING, Qt::Horizontal, tr("RATING"));
    model_.setHeaderData(PLAYLIST_ALBUM_RG, Qt::Horizontal, tr("ALBUM.RG"));
    model_.setHeaderData(PLAYLIST_ALBUM_PK, Qt::Horizontal, tr("ALBUM.PK"));
    model_.setHeaderData(PLAYLIST_LAST_UPDATE_TIME, Qt::Horizontal, tr("LAST UPDATE TIME"));
    model_.setHeaderData(PLAYLIST_TRACK_RG, Qt::Horizontal, tr("TRACK.RG"));
    model_.setHeaderData(PLAYLIST_TRACK_PK, Qt::Horizontal, tr("TRACK.PK"));
    model_.setHeaderData(PLAYLIST_GENRE, Qt::Horizontal, tr("GENRE"));
    model_.setHeaderData(PLAYLIST_YEAR, Qt::Horizontal, tr("YEAR"));

    hideColumn(PLAYLIST_MUSIC_ID);
    hideColumn(PLAYLIST_FILEPATH);
    hideColumn(PLAYLIST_FILE_NAME);
    hideColumn(PLAYLIST_ALBUM_ID);
    hideColumn(PLAYLIST_ARTIST_ID);
    hideColumn(PLAYLIST_COVER_ID);
    hideColumn(PLAYLIST_FILE_EXT);
    hideColumn(PLAYLIST_FILE_PARENT_PATH);
    hideColumn(PLAYLIST_BITRATE);
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

    auto column_list = AppSettings::getList(kAppSettingColumnName);

    if (column_list.empty()) {
        for (auto column = 0; column < horizontalHeader()->count(); ++column) {
            if (!isColumnHidden(column)) {
                AppSettings::addList(kAppSettingColumnName, QString::number(column));
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
    notshow_column_names_.insert(Q_UTF8("albumId"));
    notshow_column_names_.insert(Q_UTF8("artistId"));
    notshow_column_names_.insert(Q_UTF8("coverId"));
    notshow_column_names_.insert(Q_UTF8("fileExt"));
    notshow_column_names_.insert(Q_UTF8("parentPath"));
    notshow_column_names_.insert(Q_UTF8("playlistMusicsId"));

    proxy_model_.setSourceModel(&model_);
    proxy_model_.setFilterByColumn(PLAYLIST_RATING);
    proxy_model_.setFilterByColumn(PLAYLIST_TITLE);
    proxy_model_.setDynamicSortFilter(true);
    setModel(&proxy_model_);

    auto f = font();
#ifdef Q_OS_WIN
    f.setPointSize(10);
#else
    f.setPointSize(14);
#endif
    setFont(f);

    setUpdatesEnabled(true);
    setAcceptDrops(true);
    setDragEnabled(true);
    setShowGrid(false);

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

    start_delegate_ = new StarDelegate(this);
    setItemDelegateForColumn(PLAYLIST_RATING, start_delegate_);

    (void)QObject::connect(start_delegate_, &StarDelegate::commitData, [this](auto editor) {
        auto start_editor = qobject_cast<StarEditor*>(editor);
        if (!start_editor) {
            return;
        }
        auto index = model()->index(start_editor->row(), PLAYLIST_RATING);
        auto item = nomapItem(index);        
        item.rating = start_editor->starRating().starCount();
        SharedSingleton<Database>::GetInstance().updateMusicRating(item.music_id, item.rating);
        refresh();
    });

    setEditTriggers(DoubleClicked | SelectedClicked);
    verticalHeader()->setSectionsMovable(false);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    (void)QObject::connect(horizontalHeader(), &QHeaderView::sectionClicked, [this](int logicalIndex) {
        sortByColumn(logicalIndex, Qt::AscendingOrder);
    });

    (void)QObject::connect(this, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        const auto current_index = proxy_model_.mapToSource(index);
        setNowPlaying(current_index);
        const auto play_item = getEntity(current_index);
        emit playMusic(play_item);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);

        ActionMap<PlayListTableView> action_map(this);

    	if (!podcast_mode_) {
            (void)action_map.addAction(tr("Load local file"), [this]() {
                auto reader = MakeMetadataReader();
                QString exts(Q_UTF8("("));
                for (const auto& file_ext : reader->GetSupportFileExtensions()) {
                    exts += Q_UTF8("*") + QString::fromStdString(file_ext);
                    exts += Q_UTF8(" ");
                }
                exts += Q_UTF8(")");
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

            (void)action_map.addAction(tr("Load file directory"), [this]() {
                const auto dir_name = QFileDialog::getExistingDirectory(this,
                    tr("Select a Directory"),
                    AppSettings::getMyMusicFolderPath(), QFileDialog::ShowDirsOnly);
                if (dir_name.isEmpty()) {
                    return;
                }
                append(dir_name);
                });
#ifdef XAMP_OS_WIN
            auto *open_cd_submenu = action_map.addSubMenu(tr("Open CD device"));
            const QList<std::string> kCDFileSystemType = {
            	"CDFS",
                "UDF",
                "ISO-9660",
            	"ISO9660"
            };
            foreach(auto& storage, QStorageInfo::mountedVolumes()) {
                if (storage.isValid() && storage.isReady()) {
                    auto display_name = storage.displayName() + Q_UTF8("(") + storage.rootPath() + Q_UTF8(")");
                    auto driver_letter = storage.rootPath().left(1).toStdString()[0];

                    if (kCDFileSystemType.contains(storage.fileSystemType().toUpper().toStdString())) {                       
                        auto device = OpenCD(driver_letter);
                        auto device_info = device->GetCDDeviceInfo();
                        display_name += QString::fromStdWString(L" " + device_info.product);
                        open_cd_submenu->addAction(display_name, [driver_letter, this]() {
                            auto cd = OpenCD(driver_letter);
                            cd->SetMaxSpeed();
                            auto track_id = 0;
                            std::vector<Metadata> metadatas;
                            for (auto const & track : cd->GetTotalTracks()) {
                                auto metadata = ::getMetadata(QString::fromStdWString(track));
                                metadata.duration = cd->GetDuration(track_id++);
                                metadata.samplerate = 44100;
                                metadatas.push_back(metadata);
                            }
                            processMeatadata(QDateTime::currentSecsSinceEpoch(), metadatas);
                        });
                    }
                }
            }
#endif
            action_map.addSeparator();
    	}

        if (podcast_mode_) {
            auto* import_podcast_act = action_map.addAction(tr("Import podcast"));
            action_map.setCallback(import_podcast_act, [this]() {
                importPodcast();
            });
        }

        auto * reload_metadata_act = action_map.addAction(tr("Reload metadata"));
        auto * remove_all_act = action_map.addAction(tr("Remove all"));
        auto * open_local_file_path_act = action_map.addAction(tr("Open local file path"));
        auto * read_select_item_replaygain_act = action_map.addAction(tr("Read replay gain"));
        action_map.addSeparator();
        auto * export_flac_file_act = action_map.addAction(tr("Export flac file"));
        action_map.addSeparator();
        auto * copy_album_act = action_map.addAction(tr("Copy album"));
        auto * copy_artist_act = action_map.addAction(tr("Copy artist"));
        auto * copy_title_act = action_map.addAction(tr("Copy title"));
        action_map.addSeparator();
        auto * set_cover_art_act = action_map.addAction(tr("Set cover art"));
        auto * export_cover_act = action_map.addAction(tr("Export music cover"));

        if (model_.rowCount() == 0 || !index.isValid()) {
            try {
                action_map.exec(pt);
            }
            catch (std::exception const& e) {
                Toast::showTip(QString::fromStdString(e.what()), this);
            }
            return;
        }

        auto item = getEntity(proxy_model_.mapToSource(index));

        action_map.setCallback(reload_metadata_act, [this, item]() {
            auto reader = MakeMetadataReader();
            auto metadata = reader->Extract(item.file_path.toStdWString());
            SharedSingleton<Database>::GetInstance().addOrUpdateMusic(
                metadata, -1);
            refresh();
        });

        action_map.setCallback(remove_all_act, [this]() {
            SharedSingleton<Database>::GetInstance().removePlaylistAllMusic(playlistId());
            updateData();
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
            std::vector<PlayListEntity> items;
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

        action_map.setCallback(export_cover_act, [item, this]() {
	        const auto reader = MakeMetadataReader();
            auto& buffer = reader->GetEmbeddedCover(item.file_path.toStdWString());
            if (buffer.empty()) {
                return;
            }
            const auto file_name = QFileDialog::getSaveFileName(this,
                                                                tr("Save cover image"),
                                                                Qt::EmptyString,
                                                                tr("Images file (*.png);;All Files (*)"),
                nullptr);
            if (file_name.isEmpty()) {
                return;
            }
            QFile file(file_name);
            QPixmap pixmap;
            pixmap.loadFromData(buffer.data(),
                static_cast<uint32_t>(buffer.size()));
            pixmap.save(&file, "png", 100);
            });

        action_map.setCallback(set_cover_art_act, [item, this]() {
            const auto file_name = QFileDialog::getOpenFileName(this, 
                tr("Open Cover Art Image"),
                tr("C:\\"), 
                tr("Image Files (*.png *.jpeg *.jpg)"),
                nullptr);
            if (file_name.isEmpty()) {
                return;
            }

            const QPixmap image(file_name);
            if (image.isNull()) {
                Toast::showTip(tr("Can't read image file."), this);
                return;
            }

            const QSize kMaxCoverArtSize(500, 500);
            const auto resize_cover = Pixmap::resizeImage(image, kMaxCoverArtSize, true);

            const auto image_data = Pixmap::getImageDate(resize_cover);
            const auto rows = selectItemIndex();
            for (const auto& select_item : rows) {
                auto entity = this->item(select_item.second);
                auto writer = MakeMetadataWriter();
                writer->WriteEmbeddedCover(entity.file_path.toStdWString(), image_data);
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
}

bool PlayListTableView::isPodcastMode() const {
    return podcast_mode_;
}

void PlayListTableView::setPodcastMode(bool enable) {
    podcast_mode_ = enable;
	if (podcast_mode_) {
        hideColumn(PLAYLIST_TRACK_RG);
        hideColumn(PLAYLIST_TRACK_PK);
        hideColumn(PLAYLIST_ARTIST);
        hideColumn(PLAYLIST_ALBUM_RG);
        hideColumn(PLAYLIST_ALBUM_PK);
        hideColumn(PLAYLIST_DURATION);
        hideColumn(PLAYLIST_SAMPLE_RATE);
        hideColumn(PLAYLIST_GENRE);
        hideColumn(PLAYLIST_YEAR);
        setColumnHidden(PLAYLIST_LAST_UPDATE_TIME, false);
    } else {
        horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
        (void)QObject::connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, [this](auto pt) {
            ActionMap<PlayListTableView> action_map(this);

            auto* header_view = horizontalHeader();

            auto last_referred_logical_column = header_view->logicalIndexAt(pt);
            action_map.addAction(tr("Hide this column"), [last_referred_logical_column, this]() {
                setColumnHidden(last_referred_logical_column, true);
                AppSettings::removeList(kAppSettingColumnName, QString::number(last_referred_logical_column));
                });
            
            action_map.addAction(tr("Select columns to show..."), [pt, header_view, this]() {
                ActionMap<PlayListTableView> action_map(this);
                for (auto column = 0; column < header_view->count(); ++column) {
                    auto header_name = model()->headerData(column, Qt::Horizontal).toString();
                    if (notshow_column_names_.contains(header_name)) {
                        continue;
                    }
                    action_map.addAction(header_name, [this, column]() {
                        setColumnHidden(column, false);
                        AppSettings::addList(kAppSettingColumnName, QString::number(column));
                        }, false, !isColumnHidden(column));
                }
                action_map.exec(pt);
                });
            action_map.exec(pt);
            });
    }
}

void PlayListTableView::onThemeColorChanged(QColor backgroundColor, QColor color) {
    //setStyleSheet(backgroundColorToString(backgroundColor));
}

void PlayListTableView::updateReplayGain(int music_id,
    double album_rg_gain,
    double album_peak,
    double track_rg_gain,
    double track_peak) {
    SharedSingleton<Database>::GetInstance().updateReplayGain(
        music_id, album_rg_gain, album_peak, track_rg_gain, track_peak);
    XAMP_LOG_DEBUG("Update DB music id : {}, album_rg_gain: {:.2f} album_peak: {:.2f} track_rg_gain: {:.2f} track_peak: {:.2f}",
        music_id, album_rg_gain, album_peak, track_rg_gain, track_peak);
    refresh();
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

void PlayListTableView::append(const QString& file_name, bool show_progress_dialog) {
	const auto adapter = QSharedPointer<::MetadataExtractAdapter>(new ::MetadataExtractAdapter());

    (void) QObject::connect(adapter.get(),
                            &::MetadataExtractAdapter::readCompleted,
                            this,
                            &PlayListTableView::processMeatadata);

   ::MetadataExtractAdapter::readFileMetadata(adapter, file_name, show_progress_dialog);
}

void PlayListTableView::processMeatadata(int64_t dir_last_write_time, const std::vector<Metadata>& medata) {
    ::MetadataExtractAdapter::processMetadata(dir_last_write_time, medata, this, podcast_mode_);
    resizeColumn();
    updateData();
}

void PlayListTableView::resizeEvent(QResizeEvent* event) {
    QTableView::resizeEvent(event);
    resizeColumn();
}

void PlayListTableView::importPodcast() {
    /*QDialog dialog(this);
    dialog.setWindowTitle(tr("Import file from meta.json"));
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.resize(600, 20);

    QFormLayout form(&dialog);
    auto* url_edit = new QLineEdit(&dialog);
    url_edit->setText(Q_UTF8("https://suisei.moe/podcast.xml"));
    form.addRow(tr("URL:"), url_edit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    (void)QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    (void)QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }*/

    http::HttpClient(/*url_edit->text()*/Q_UTF8("https://suisei.moe/podcast.xml"))
	.success([this](const QString& json) {
        auto const podcast_info = parsePodcastXML(json);
        ::MetadataExtractAdapter::processMetadata(QDateTime::currentSecsSinceEpoch(), podcast_info.second, this, podcast_mode_);
        http::HttpClient(QString::fromStdString(podcast_info.first))
    	.download([=](auto data) {
            auto cover_id = SharedSingleton<PixmapCache>::GetInstance().addOrUpdate(data);
    		if (!model()->rowCount()) {
                return;
    		}
            auto play_item = getEntity(this->model()->index(0, 0));
            SharedSingleton<Database>::GetInstance().setAlbumCover(play_item.album_id, play_item.album, cover_id);
            });    	
        }).get();
}

void PlayListTableView::resizeColumn() {
    constexpr auto kStretchedSize = 500;
    auto* header = horizontalHeader();

    if (header->count() == 3) {
        QList<int> not_hide_column;
        for (auto column = 0; column < header->count(); ++column) {
            if (!isColumnHidden(column)) {
                not_hide_column.append(column);
            }
        }
        header->setSectionResizeMode(not_hide_column.back(), QHeaderView::Fixed);
        header->resizeSection(not_hide_column.back(), 300);
        return;
    }

    for (auto column = 0; column < header->count(); ++column) {
        switch (column) {
        case PLAYLIST_PLAYING:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            header->resizeSection(column, 15);
            break;
        case PLAYLIST_TRACK:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 50);
            break;
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_TRACK_RG:
        case PLAYLIST_TRACK_PK:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        case PLAYLIST_DURATION:
        case PLAYLIST_BITRATE:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 90);
            break;
        case PLAYLIST_TITLE:
            header->resizeSection(column,
                (std::max)(sizeHintForColumn(column), kStretchedSize));
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
}

void PlayListTableView::search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax) {
    const QRegExp reg_exp(sort_str, case_sensitivity, pattern_syntax);

    proxy_model_.setFilterRegExp(reg_exp);
    proxy_model_.invalidate();
}

int32_t PlayListTableView::playlistId() const {
    return playlist_id_;
}

QModelIndex PlayListTableView::currentIndex() const {
    return play_index_;
}

QModelIndex PlayListTableView::nextIndex(int forward) const {
    const auto count = proxy_model_.rowCount();
    const auto play_index = currentIndex();
    const auto next_index = (play_index.row() + forward) % count;
    return model()->index(next_index, PLAYLIST_PLAYING);
}

QModelIndex PlayListTableView::shuffeIndex() {    
    const auto count = proxy_model_.rowCount();
    const auto selected = rng_.NextInt64(0) % count;
    return model()->index(selected, PLAYLIST_PLAYING);
}

void PlayListTableView::setNowPlaying(const QModelIndex& index, bool is_scroll_to) {
    auto count = proxy_model_.rowCount();
    play_index_ = index;
    setCurrentIndex(play_index_);
    if (is_scroll_to) {
        QTableView::scrollTo(play_index_, PositionAtCenter);
    }
    const auto entity = getEntity(play_index_);
    SharedSingleton<Database>::GetInstance().setNowPlaying(playlist_id_, entity.playlist_music_id);
    refresh();
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

PlayListEntity PlayListTableView::nomapItem(const QModelIndex& index) {
    return getEntity(index);
}

PlayListEntity PlayListTableView::item(const QModelIndex& index) {
     return getEntity(proxy_model_.mapToSource(index));
}

void PlayListTableView::setCurrentPlayIndex(const QModelIndex& index) {
    play_index_ = index;
}

void PlayListTableView::play(const QModelIndex& index) {
    play_index_ = index;
    setNowPlaying(play_index_, true);
    const auto play_item = nomapItem(play_index_);
    emit playMusic(play_item);
}

void PlayListTableView::removePlaying() {
    SharedSingleton<Database>::GetInstance().clearNowPlaying(playlist_id_);
    updateData();
}

void PlayListTableView::removeItem(const QModelIndex& index) {
    proxy_model_.removeRows(index.row(), 1, index);
}

void PlayListTableView::removeSelectItems() {
    const auto rows = selectItemIndex();

    QVector<int> remove_music_ids;

    for (auto itr = rows.rbegin(); itr != rows.rend(); ++itr) {
        const auto it = item((*itr).second);
        SharedSingleton<Database>::GetInstance().clearNowPlaying(playlist_id_, it.playlist_music_id);
        remove_music_ids.push_back(it.playlist_music_id);
    }

    const auto count = proxy_model_.rowCount();
	if (!count) {
        SharedSingleton<Database>::GetInstance().clearNowPlaying(playlist_id_);       
	}

    SharedSingleton<Database>::GetInstance().removePlaylistMusic(playlist_id_, remove_music_ids);
    updateData();
}
