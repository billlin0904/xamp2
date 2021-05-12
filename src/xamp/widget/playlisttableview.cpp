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
#include <QJsonArray>

#include <base/rng.h>
#include <metadata/metadatareader.h>
#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

#include <thememanager.h>
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
#include <widget/time_utilts.h>
#include <widget/musicentity.h>
#include <widget/playlisttableview.h>

PlayListEntity PlayListTableView::fromMetadata(const Metadata& metadata) {
    PlayListEntity item;
    item.track = metadata.track;
    item.duration = metadata.duration;
    item.samplerate = metadata.samplerate;
    item.title = QString::fromStdWString(metadata.title);
    item.file_ext = QString::fromStdWString(metadata.file_ext);
    item.album = QString::fromStdWString(metadata.album);
    item.artist = QString::fromStdWString(metadata.artist);
    item.parent_path = QString::fromStdWString(metadata.parent_path);
    item.file_path = QString::fromStdWString(metadata.file_path);
    item.bitrate = metadata.bitrate;
    item.file_name = QString::fromStdWString(metadata.file_name);
    return item;
}

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
    entity.fingerprint = getIndexValue(index, PLAYLIST_FINGER_PRINT).toString();
    entity.file_ext = getIndexValue(index, PLAYLIST_FILE_EXT).toString();
    entity.parent_path = getIndexValue(index, PLAYLIST_FILE_PARENT_PATH).toString();
    entity.lufs = getIndexValue(index, PLAYLIST_LRUS).toDouble();
    entity.true_peak = getIndexValue(index, PLAYLIST_TRUE_PEAK).toDouble();
    return entity;
}

PlayListTableView::PlayListTableView(QWidget* parent, int32_t playlist_id)
    : QTableView(parent)
    , playlist_id_(playlist_id)
    , start_delegate_(nullptr)
    , model_(this)
    , proxy_model_(this) {
    initial();
}

PlayListTableView::~PlayListTableView() {
    thread_.quit();
    thread_.wait();    
}

void PlayListTableView::refresh() {
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
    musics.fingerprint,
	musics.fileExt,
    musics.parentPath,
    musics.lufs,
	musics.true_peak
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
	playlistMusics.playlistMusicsId;
    )");
    model_.setQuery(s.arg(playlist_id_));
    proxy_model_.dataChanged(QModelIndex(), QModelIndex());
}

void PlayListTableView::setPlaylistId(const int32_t playlist_id) {
    playlist_id_ = playlist_id;

    Singleton<Database>::GetInstance().clearNowPlaying(playlist_id_);

    refresh();

    model_.setHeaderData(PLAYLIST_MUSIC_ID, Qt::Horizontal, tr("Music Id"));
    model_.setHeaderData(PLAYLIST_PLAYING, Qt::Horizontal, tr(" "));
    model_.setHeaderData(PLAYLIST_TRACK, Qt::Horizontal, tr("#"));
    model_.setHeaderData(PLAYLIST_FILEPATH, Qt::Horizontal, tr("File path"));
    model_.setHeaderData(PLAYLIST_TITLE, Qt::Horizontal, tr("Title"));
    model_.setHeaderData(PLAYLIST_FILE_NAME, Qt::Horizontal, tr("File name"));
    model_.setHeaderData(PLAYLIST_ALBUM, Qt::Horizontal, tr("Album"));
    model_.setHeaderData(PLAYLIST_ARTIST, Qt::Horizontal, tr("Artist"));
    model_.setHeaderData(PLAYLIST_DURATION, Qt::Horizontal, tr("Duration"));
    model_.setHeaderData(PLAYLIST_BITRATE, Qt::Horizontal, tr("Bit rate"));
    model_.setHeaderData(PLAYLIST_SAMPLE_RATE, Qt::Horizontal, tr("SampleRate"));
    model_.setHeaderData(PLAYLIST_RATING, Qt::Horizontal, tr("Rating"));
    model_.setHeaderData(PLAYLIST_LRUS, Qt::Horizontal, tr("LRUS"));
    model_.setHeaderData(PLAYLIST_TRUE_PEAK, Qt::Horizontal, tr("TP"));

    hideColumn(PLAYLIST_MUSIC_ID);
    hideColumn(PLAYLIST_FILEPATH);
    hideColumn(PLAYLIST_FILE_NAME);
    hideColumn(PLAYLIST_SAMPLE_RATE);

    hideColumn(PLAYLIST_ALBUM_ID);
    hideColumn(PLAYLIST_ARTIST_ID);
    hideColumn(PLAYLIST_COVER_ID);
    hideColumn(PLAYLIST_FINGER_PRINT);

    hideColumn(PLAYLIST_FILE_EXT);
    hideColumn(PLAYLIST_FILE_PARENT_PATH);

    hideColumn(PLAYLIST_BITRATE);
    hideColumn(PLAYLIST_ALBUM);
    //hideColumn(PLAYLIST_LRUS);
    //hideColumn(PLAYLIST_TRUE_PEAK);
    hideColumn(PLAYLIST_RATING);
    hideColumn(PLAYLIST_DURATION);
}

void PlayListTableView::initial() {
    proxy_model_.setSourceModel(&model_);
    proxy_model_.setFilterByColumn(PLAYLIST_RATING);
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
        Singleton<Database>::GetInstance().updateMusicRating(item.music_id, item.rating);
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
        refresh();
        auto play_item = getEntity(current_index);
    	if (play_item.lufs == 0.0) {
            readLUFS(play_item);
    	}        
        emit playMusic(current_index, play_item);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);

        ActionMap<PlayListTableView, std::function<void()>> action_map(this);

        (void)action_map.addAction(tr("Load local file"), [this]() {
            xamp::metadata::TaglibMetadataReader reader;
            QString exts(Q_UTF8("("));
            for (const auto file_ext : reader.GetSupportFileExtensions()) {
                exts += Q_UTF8("*") + QString::fromStdString(file_ext);
                exts += Q_UTF8(" ");
            }
            exts += Q_UTF8(")");
            const auto file_name = QFileDialog::getOpenFileName(this,
                tr("Open file"),
                AppSettings::getMyMusicFolderPath(),
                tr("Music Files ") + exts);
            if (file_name.isEmpty()) {
                return;
            }
            append(file_name);
            });

        (void)action_map.addAction(tr("Load file directory"), [this]() {
	        const auto dir_name = QFileDialog::getExistingDirectory(this,
	                                                                tr("Select a Directory"),
	                                                                AppSettings::getMyMusicFolderPath(),
	                                                                QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly);
        	if (dir_name.isEmpty()) {
                return;
        	}
            append(dir_name);
            });

        action_map.addSeparator();

        auto * remove_all_act = action_map.addAction(tr("Remove all"));
        auto * open_local_file_path_act = action_map.addAction(tr("Open local file path"));
        auto * reload_file_meta_act = action_map.addAction(tr("Reload file meta"));
        auto * reload_file_fingerprint_act = action_map.addAction(tr("Read file fingerprint"));
        auto * read_file_lufs_act = action_map.addAction(tr("Read file LUFS"));
        auto * export_wave_file_act = action_map.addAction(tr("Export wave file"));
        auto * copy_album_act = action_map.addAction(tr("Copy album"));
        auto * copy_artist_act = action_map.addAction(tr("Copy artist"));
        auto * copy_title_act = action_map.addAction(tr("Copy title"));
        auto * set_cover_art_act = action_map.addAction(tr("Set cover art"));
        auto * export_cover_act = action_map.addAction(tr("Export music cover"));

        if (model_.rowCount() == 0 || !index.isValid()) {
            action_map.exec(pt);
            return;
        }

        auto item = getEntity(proxy_model_.mapToSource(index));

        action_map.setCallback(remove_all_act, [this, item]() {
            Singleton<Database>::GetInstance().removePlaylistAllMusic(playlistId());
            refresh();
            removePlaying();
            });

        action_map.setCallback(open_local_file_path_act, [item]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(item.parent_path));
            });

        action_map.setCallback(reload_file_meta_act, [this]() {
            reloadSelectMetadata();
            });

        action_map.setCallback(reload_file_fingerprint_act, [this]() {
            const auto rows = selectItemIndex();
            for (const auto& select_item : rows) {
                auto entity = this->item(select_item.second);
                emit readFingerprint(select_item.second, entity);
            }
            });
        action_map.setCallback(read_file_lufs_act, [this]() {
            const auto rows = selectItemIndex();
            for (const auto& select_item : rows) {
                auto entity = this->item(select_item.second);
                emit readFileLUFS(select_item.second, entity);
            }
            refresh();
            });

        action_map.setCallback(export_wave_file_act, [this]() {
            const auto rows = selectItemIndex();
            for (const auto& select_item : rows) {
                auto entity = this->item(select_item.second);
                emit exportWaveFile(select_item.second, entity);
            }
            refresh();
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

        action_map.setCallback(export_cover_act, [item, this]() {
            xamp::metadata::TaglibMetadataReader reader;
            auto buffer = reader.ExtractEmbeddedCover(item.file_path.toStdWString());
            if (buffer.empty()) {
                return;
            }
            const auto file_name = QFileDialog::getSaveFileName(this,
                                                                tr("Save cover image"),
                                                                Qt::EmptyString,
                                                                tr("Images file (*.png);;All Files (*)"));
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
            const auto file_name = QFileDialog::getOpenFileName(this, tr("Open Cover Art Image"),
                                                               tr("C:\\"), tr("Image Files (*.png *.jpeg *.jpg)"));
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
                xamp::metadata::TaglibMetadataWriter writer;
                try {
                    writer.WriteEmbeddedCover(entity.file_path.toStdWString(), image_data);
                }
                catch (std::exception& e) {
                    Toast::showTip(QString::fromStdString(e.what()), this);
                }
            }
        });

        action_map.exec(pt);
    });

    const auto *control_A_key = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    (void) QObject::connect(control_A_key, &QShortcut::activated, [this]() {
        selectAll();
    });

    installEventFilter(this);

    read_worker_.moveToThread(&thread_);
    QObject::connect(this,
        &PlayListTableView::readLUFS,
        &read_worker_,
        &ReadLufsWorker::addEntity);
    QObject::connect(&read_worker_, 
        &ReadLufsWorker::readCompleted,
        this,
        &PlayListTableView::onReadCompleted);
    thread_.start();
}

void PlayListTableView::onReadCompleted(int32_t music_id, double lrus, double trure_peak) {
    XAMP_LOG_DEBUG("onReadCompleted {} {}, {}", music_id, lrus, trure_peak);
    Singleton<Database>::GetInstance().updateLUFS(music_id, lrus, trure_peak);
    refresh();
}

void PlayListTableView::onThemeColorChanged(QColor backgroundColor, QColor color) {
    setStyleSheet(backgroundColorToString(backgroundColor));
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
        auto* event = dynamic_cast<QKeyEvent*>(ev);
        if (event->key() == Qt::Key_Delete) {
            removeSelectItems();
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void PlayListTableView::append(const QString& file_name) {
	const auto adapter = QSharedPointer<MetadataExtractAdapter>(new MetadataExtractAdapter());

    (void) QObject::connect(adapter.get(),
                            &MetadataExtractAdapter::readCompleted,
                            this,
                            &PlayListTableView::processMeatadata);

    MetadataExtractAdapter::readFileMetadata(adapter, file_name);    
}

void PlayListTableView::processMeatadata(const std::vector<Metadata>& medata) {    
    MetadataExtractAdapter::processMetadata(medata, this);
    resizeColumn();
    refresh();
}

void PlayListTableView::resizeEvent(QResizeEvent* event) {
    QTableView::resizeEvent(event);
    resizeColumn();
}

void PlayListTableView::resizeColumn() const {
    constexpr auto kStretchedSize = 500;
    auto* header = horizontalHeader();
	
    for (auto column = 0; column < header->count(); ++column) {
        switch (column) {
        case PLAYLIST_PLAYING:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 3);
            break;
        case PLAYLIST_TRACK:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 40);
            break;
        case PLAYLIST_LRUS:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 30);
            break;
        case PLAYLIST_TRUE_PEAK:
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
    const auto selected = RNG::GetInstance()(0, count - 1);
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
    Singleton<Database>::GetInstance().setNowPlaying(playlist_id_, entity.music_id);
    refresh();
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

    foreach(auto index, selectionModel()->selectedRows()) {
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
    const auto play_item = nomapItem(play_index_);
	if (play_item.lufs == 0.0) {
        readLUFS(play_item);
	}
    emit playMusic(play_index_, play_item);
}

void PlayListTableView::removePlaying() {
    Singleton<Database>::GetInstance().clearNowPlaying(playlist_id_);
    refresh();
}

void PlayListTableView::removeItem(const QModelIndex& index) {
    proxy_model_.removeRows(index.row(), 1, index);
}

void PlayListTableView::reloadSelectMetadata() {
    xamp::metadata::TaglibMetadataReader reader;
    const auto rows = selectItemIndex();

    for (const auto &select_item : rows) {
        auto entity = item(select_item.second);
        const auto album_id = entity.album_id;
        const auto music_id = entity.music_id;
        const auto cover_id = entity.cover_id;
        const auto artist_id = entity.artist_id;

        const Path path(entity.file_path.toStdWString());
        auto metadata = reader.Extract(path);

        entity = fromMetadata(metadata);
        entity.album_id = album_id;
        entity.music_id = music_id;
        entity.cover_id = cover_id;
        entity.artist_id = artist_id;

        Singleton<Database>::GetInstance().addOrUpdateMusic(metadata, -1);
    }

    refresh();
}

void PlayListTableView::removeSelectItems() {
    const auto rows = selectItemIndex();

    QVector<int> remove_music_ids;

    for (auto itr = rows.rbegin(); itr != rows.rend(); ++itr) {
        auto const music_id = model()->data((*itr).second).toInt();
        Singleton<Database>::GetInstance().clearNowPlaying(playlist_id_, music_id);
        remove_music_ids.push_back(music_id);
    }

    const auto count = proxy_model_.rowCount();
	if (!count) {
        Singleton<Database>::GetInstance().clearNowPlaying(playlist_id_);       
	}

    Singleton<Database>::GetInstance().removePlaylistMusic(playlist_id_, remove_music_ids);
    refresh();    
}
