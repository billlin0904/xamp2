#include <QHeaderView>
#include <QMenu>
#include <QFileInfo>
#include <QDesktopServices>
#include <QFileDialog>
#include <QtWidgets/QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QShortcut>
#include <QtConcurrent>
#include <QFutureWatcher>

#include <base/rng.h>
#include <metadata/metadatareader.h>
#include <metadata/taglibmetareader.h>

#include <widget/appsettings.h>
#include <widget/pixmapcache.h>
#include <widget/stardelegate.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/stareditor.h>
#include <widget/playlisttableview.h>

PlayListEntity PlayListTableView::fromMetadata(const xamp::base::Metadata& metadata) {
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

PlayListTableView::PlayListTableView(QWidget* parent, int32_t playlist_id)
    : QTableView(parent)
    , playlist_id_(playlist_id)
    , start_delegate_(nullptr)
    , model_(this)
    , proxy_model_(this) {
    initial();
}

PlayListTableView::~PlayListTableView() = default;

void PlayListTableView::initial() {
    proxy_model_.setSourceModel(&model_);
    proxy_model_.setFilterByColumn(PLAYLIST_ALBUM);
    proxy_model_.setFilterByColumn(PLAYLIST_ARTIST);
    proxy_model_.setFilterByColumn(PLAYLIST_TITLE);
    proxy_model_.setFilterByColumn(PLAYLIST_RATING);
    proxy_model_.setDynamicSortFilter(true);
    setModel(&proxy_model_);

    auto f = font();
#ifdef Q_OS_WIN
    f.setPointSize(9);
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

    hideColumn(PLAYLIST_MUSIC_ID);
    hideColumn(PLAYLIST_FILEPATH);
    hideColumn(PLAYLIST_FILE_NAME);
    hideColumn(PLAYLIST_BITRATE);
    hideColumn(PLAYLIST_SAMPLE_RATE);

    setHorizontalScrollMode(ScrollPerPixel);
    setVerticalScrollMode(ScrollPerPixel);
    setSelectionMode(ExtendedSelection);
    setSelectionBehavior(SelectRows);

    verticalHeader()->setVisible(false);
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setDefaultSectionSize(40);

    horizontalScrollBar()->setDisabled(true);
    horizontalHeader()->setHighlightSections(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    start_delegate_ = new StarDelegate(this);
    setItemDelegateForColumn(PLAYLIST_RATING, start_delegate_);
    QObject::connect(start_delegate_, &StarDelegate::commitData, [this](auto editor) {
        auto start_editor = qobject_cast<StarEditor*>(editor);
        if (!start_editor) {
            return;
        }
        auto& item = model_.item(start_editor->row());
        item.rating = start_editor->starRating().starCount();
        Database::instance().updateMusicRating(item.music_id, item.rating);
        });

    setEditTriggers(DoubleClicked | SelectedClicked);
    verticalHeader()->setSectionsMovable(false);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    (void)QObject::connect(horizontalHeader(), &QHeaderView::sectionClicked, [this](int logicalIndex) {
        sortByColumn(logicalIndex, Qt::AscendingOrder);
        });

    (void)QObject::connect(this, &QTableView::doubleClicked, [this](const QModelIndex& index) {
        emit playMusic(index, model_.item(proxy_model_.mapToSource(index)));
        setNowPlaying(index);
        });

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &QTableView::customContextMenuRequested, [this](auto pt) {
        auto index = indexAt(pt);

        ActionMap<PlayListTableView, std::function<void()>> action_map(this);

        (void)action_map.addAction(tr("Load local file"), [this]() {
            xamp::metadata::TaglibMetadataReader reader;
            QString exts(Q_UTF8("("));
            for (auto file_ext : reader.GetSupportFileExtensions()) {
                exts += Q_UTF8("*") + QString::fromStdString(file_ext);
                exts += Q_UTF8(" ");
            }
            exts += Q_UTF8(")");
            auto file_name = QFileDialog::getOpenFileName(this,
                tr("Open file"),
                AppSettings::getMyMusicFolderPath(),
                tr("Music Files ") + exts);
            if (file_name.isEmpty()) {
                return;
            }
            append(file_name);
            });

        (void)action_map.addAction(tr("Load file directory"), [this]() {
            auto dir_name = QFileDialog::getExistingDirectory(this,
                tr("Select a Directory"),
                AppSettings::getMyMusicFolderPath());
            append(dir_name);
            });
    	
        action_map.addSeparator();

        auto open_local_file_path_act = action_map.addAction(tr("Open local file path"));
        auto reload_file_meta_act = action_map.addAction(tr("Reload file meta"));
        auto reload_file_fingerprint_act = action_map.addAction(tr("Read file fingerprint"));
        auto copy_album_act = action_map.addAction(tr("Copy album"));
        auto copy_artist_act = action_map.addAction(tr("Copy artist"));
        auto copy_title_act = action_map.addAction(tr("Copy title"));

        if (!model_.isEmpty() && index.isValid()) {
            auto item = model_.item(proxy_model_.mapToSource(index));

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
        }

        action_map.exec(pt);
        });

    auto control_A_key = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_A), this);
    (void) QObject::connect(control_A_key, &QShortcut::activated, [this]() {
        selectAll();
    });

    installEventFilter(this);
}

void PlayListTableView::onTextColorChanged(QColor backgroundColor, QColor color) {
    setStyleSheet(backgroundColorToString(backgroundColor));
}

void PlayListTableView::keyPressEvent(QKeyEvent *pEvent) {
    if (pEvent->key() == Qt::Key_Return) {
        // we captured the Enter key press, now we need to move to the next row
        qint32 nNextRow = currentIndex().row() + 1;
        if (nNextRow + 1 > model()->rowCount(currentIndex())) {
            // we are all the way down, we can't go any further
            nNextRow = nNextRow - 1;
        }

        if (state() == QAbstractItemView::EditingState) {
            // if we are editing, confirm and move to the row below
            QModelIndex oNextIndex = model()->index(nNextRow, currentIndex().column());
            setCurrentIndex(oNextIndex);
            selectionModel()->select(oNextIndex, QItemSelectionModel::ClearAndSelect);
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
    auto type = ev->type();
    if (this == obj && type == QEvent::KeyPress) {
        auto event = static_cast<QKeyEvent*>(ev);
        if (event->key() == Qt::Key_Delete) {
            removeSelectItems();
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void PlayListTableView::append(const QString& file_name) {
    auto adapter = new MetadataExtractAdapter();

    (void) QObject::connect(adapter,
                            &MetadataExtractAdapter::readCompleted,
                            this,
                            &PlayListTableView::processMeatadata);

    MetadataExtractAdapter::readMetadataAsync(adapter, file_name);
}

void PlayListTableView::processMeatadata(const std::vector<xamp::base::Metadata>& medata) {
    MetadataExtractAdapter::processMetadata(medata, this);
    resizeColumn();
}

void PlayListTableView::resizeEvent(QResizeEvent* event) {
    QTableView::resizeEvent(event);
    resizeColumn();
}

void PlayListTableView::resizeColumn() const {
    auto header = horizontalHeader();

    for (auto column = 0; column < header->count(); ++column) {
        switch (column) {
        case PLAYLIST_TRACK:
        case PLAYLIST_PLAYING:
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            header->resizeSection(column, 12);
            break;
        case PLAYLIST_DURATION:
        case PLAYLIST_BITRATE:
            header->setSectionResizeMode(column, QHeaderView::Fixed);
            header->resizeSection(column, 90);
            break;
        case PLAYLIST_TITLE:
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            header->resizeSection(column, 450);
            break;
        case PLAYLIST_ALBUM:
            header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
            break;
        case PLAYLIST_ARTIST:
        default:
            header->setSectionResizeMode(column, QHeaderView::Stretch);
            break;
        }
    }
}

void PlayListTableView::appendItem(const xamp::base::Metadata& metadata) {
    appendItem(fromMetadata(metadata));
}

void PlayListTableView::appendItem(const PlayListEntity& item) {
    model_.append(item);
}

void PlayListTableView::search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax) {
    const QRegExp reg_exp(sort_str, case_sensitivity, pattern_syntax);

    proxy_model_.setFilterRegExp(reg_exp);
    proxy_model_.invalidate();
}

void PlayListTableView::setPlaylistId(const int32_t playlist_id) {
    playlist_id_ = playlist_id;
}

int32_t PlayListTableView::playlistId() const {
    return playlist_id_;
}

QModelIndex PlayListTableView::currentIndex() const {
    return play_index_;
}

QModelIndex PlayListTableView::shuffeIndex() {
    std::vector<int32_t> indexes;
    indexes.reserve(static_cast<size_t>(proxy_model_.rowCount()));

    for (int n = 0; n < proxy_model_.rowCount(); n++) {
        if (model_.nowPlaying() != n) {
            indexes.push_back(n);
        }
    }

    xamp::base::RNG::Instance().Shuffle(indexes);
    auto selected = xamp::base::RNG::Instance()(size_t(0), indexes.size() - 1);
    return model()->index(indexes[selected], PLAYLIST_PLAYING);
}

void PlayListTableView::setNowPlaying(const QModelIndex& index, bool is_scroll_to) {
    play_index_ = index;
    model_.setNowPlaying(proxy_model_.mapToSource(index).row());
    setCurrentIndex(index);
    if (is_scroll_to) {
        QTableView::scrollTo(index, PositionAtCenter);
    }
    proxy_model_.dataChanged(QModelIndex(), QModelIndex());
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

PlayListEntity& PlayListTableView::item(const QModelIndex& index) {
    return model_.item(proxy_model_.mapToSource(index));
}

void PlayListTableView::play(const QModelIndex& index) {
    play_index_ = index;
    emit playMusic(index, item(index));
}

void PlayListTableView::removePlaying() {
    model_.setNowPlaying(-1);
    proxy_model_.dataChanged(QModelIndex(), QModelIndex());
}

void PlayListTableView::removeItem(const QModelIndex& index) {
    proxy_model_.removeRows(index.row(), 1, index);
}

void PlayListTableView::reloadSelectMetadata() {
    xamp::metadata::TaglibMetadataReader reader;
    const auto rows = selectItemIndex();

    for (const auto &select_item : rows) {
        auto &entity = item(select_item.second);
        auto album_id = entity.album_id;
        auto music_id = entity.music_id;
        auto cover_id = entity.cover_id;
        auto artist_id = entity.artist_id;

        const xamp::metadata::Path path(entity.file_path.toStdWString());
        auto metadata = reader.Extract(path);

        entity = fromMetadata(metadata);
        entity.album_id = album_id;
        entity.music_id = music_id;
        entity.cover_id = cover_id;
        entity.artist_id = artist_id;

        Database::instance().addOrUpdateMusic(metadata, -1);
    }

    proxy_model_.dataChanged(QModelIndex(), QModelIndex());
}

void PlayListTableView::removeSelectItems() {
    const auto rows = selectItemIndex();

    QVector<int> remove_music_ids;

    auto index = 0;
    for (auto itr = rows.rbegin(); itr != rows.rend(); ++itr) {
        auto const music_id = model()->data((*itr).second);
        remove_music_ids.push_back(music_id.toInt());
        proxy_model_.removeRows((*itr).first, 1);
        if (model_.nowPlaying() == index) {
            removePlaying();
        }
        ++index;
    }

    if (!remove_music_ids.empty()) {
        emit removeItems(playlist_id_, remove_music_ids);
    }

    if (model_.isEmpty()) {
        removePlaying();
    }
}
