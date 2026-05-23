#include <ui_filesystemviewpage.h>

#include <widget/filesystemviewpage.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/appsettingnames.h>
#include <widget/actionmap.h>
#include <widget/playlisttableview.h>
#include <widget/filesystemmodel.h>
#include <widget/util/ui_util.h>
#include <widget/xmainwindow.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/scanfileprogresspage.h>
#include <widget/chatgpt/spectrogramwidget.h>

#include <QLabel>
#include <QHelpEvent>
#include <QDateTime>
#include <QPointer>
#include <QThread>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QScrollBar>

class FileSystemViewPage::DisableToolTipStyledItemDelegate : public QStyledItemDelegate {
public:
    explicit DisableToolTipStyledItemDelegate(QObject* parent)
        : QStyledItemDelegate(parent) {
    }

    bool helpEvent(QHelpEvent* e, QAbstractItemView* view, const QStyleOptionViewItem& option,
        const QModelIndex& index) override;
};

bool FileSystemViewPage::DisableToolTipStyledItemDelegate::helpEvent(QHelpEvent* e,
    QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) {
    if (!e || !view) {
        return false;
    }

    if (e->type() == QEvent::ToolTip) {        
        return true;
    }
    return QStyledItemDelegate::helpEvent(e, view, option, index);
}

class FileSystemViewPage::DirFirstSortFilterProxyModel : public QSortFilterProxyModel {
public:
    explicit DirFirstSortFilterProxyModel(QObject* parent)
	    : QSortFilterProxyModel(parent) {
        setRecursiveFilteringEnabled(true);
    }

    void setSearchString(const QString& search) {
        searchString = search;
        invalidateFilter();
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
        auto* fsm = qobject_cast<QFileSystemModel*>(sourceModel());
        if (!fsm) return true;

        QModelIndex idx = fsm->index(sourceRow, 0, sourceParent);
        QString name = fsm->fileName(idx);

        //XAMP_LOG_DEBUG("=> {}", name.toStdString());

        if (searchString.isEmpty() ||
            name.contains(searchString, Qt::CaseInsensitive))
            return true;

        if (fsm->isDir(idx)) {
            for (int r = 0, rows = fsm->rowCount(idx); r < rows; ++r)
                if (filterAcceptsRow(r, idx))
                    return true;
        }
        return false;
    }

private:
    QString searchString;
};

bool FileSystemViewPage::DirFirstSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    if (sortColumn() == 0) {
        auto* fsm = qobject_cast<QFileSystemModel*>(sourceModel());
        bool asc = sortOrder() == Qt::AscendingOrder ? true : false;

        const auto left_file_info = fsm->fileInfo(left);
        const auto right_file_info = fsm->fileInfo(right);

        // If DotAndDot move in the beginning
        if (sourceModel()->data(left).toString() == ".."_str)
            return asc;
        if (sourceModel()->data(right).toString() == ".."_str)
            return !asc;

        // Move dirs upper
        if (!left_file_info.isDir() && right_file_info.isDir()) {
            return !asc;
        }
        if (left_file_info.isDir() && !right_file_info.isDir()) {
            return asc;
        }
        if (left_file_info.lastModified() > right_file_info.lastModified()) {
            return asc;
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

FileSystemViewPage::FileSystemViewPage(QWidget* parent)
    : QFrame(parent) {
    ui_ = new Ui::FileSystemViewPage();
    ui_->setupUi(this);

    progress_page_ = new ScanFileProgressPage(this);
    progress_page_->hide();
    progress_page_->move(0, height() - 80);

    setFrameStyle(QFrame::StyledPanel);    

    dir_model_ = new FileSystemModel(this);   

    dir_first_sort_filter_ = new DirFirstSortFilterProxyModel(this);
    dir_first_sort_filter_->setSourceModel(dir_model_);
    dir_first_sort_filter_->setFilterKeyColumn(0);

    ui_->dirTree->setModel(dir_first_sort_filter_);
    ui_->dirTree->setItemDelegate(
        new DisableToolTipStyledItemDelegate(this));
    ui_->dirTree->setStyleSheet("background-color: transparent"_str);
    ui_->dirTree->setSortingEnabled(true);
    ui_->dirTree->setFixedWidth(300);

    auto f = ui_->dirTree->font();
    f.setPointSize(8);
	ui_->dirTree->setFont(f);

    ui_->dirTree->header()->hide();
    ui_->dirTree->hideColumn(1);
    ui_->dirTree->hideColumn(2);
    ui_->dirTree->hideColumn(3);

    ui_->dirTree->setContextMenuPolicy(Qt::CustomContextMenu);
	ui_->page->searchLineEdit()->hide();
    ui_->page->pageTitle()->hide();
    ui_->page->hidePlaybackInformation(true);
    ui_->page->playlist()->setPlaylistId(kFileSystemPlaylistId,
        kAppSettingPlaylistColumnName);
    ui_->page->playlist()->setHeaderViewHidden(false);
    (void)QObject::connect(ui_->page->playlist(),
        &PlaylistTableView::playMusic,
        this,
        [this](int32_t, const PlayListEntity& item, bool) {
            spectrogramWidget()->loadFile(item);
        });

    auto last_open_path = qAppSettings.valueAsString(
        kAppSettingFileSystemLastOpenPath);

	if (last_open_path.isEmpty()) {
		last_open_path = qAppSettings.myMusicFolderPath();
	}

    dir_model_->setRootPath(qAppSettings.myMusicFolderPath());
    dir_model_->setFilter(
        QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);

    auto filter = getTrackInfoFileNameFilter();
    dir_model_->setNameFilters(filter);
    dir_model_->setNameFilterDisables(false);

    auto last_open_index = dir_model_->index(last_open_path);
    auto proxy_index = dir_first_sort_filter_->mapFromSource(last_open_index);
    ui_->dirTree->setCurrentIndex(proxy_index);
    ui_->dirTree->expand(proxy_index);

    f = font();
    f.setBold(true);
    f.setPointSize(12);
    ui_->page->pageTitle()->setFont(f);
    ui_->page->pageTitle()->show();
    ui_->page->pageTitle()->setText(kEmptyString);

    (void)QObject::connect(ui_->dirTree, &QTreeView::clicked,
        [this](const auto &index) {      
        auto src_index = dir_first_sort_filter_->mapToSource(index);
        ui_->dirTree->expand(index);

		auto path = dir_model_->fileInfo(src_index).filePath();
        QFileInfo file_info(path);
        QString parent_dir_path;

        if (!file_info.isDir()) {
            parent_dir_path = file_info.absolutePath();
        }
        else {
            parent_dir_path = path;
        }

		if (file_path_ == parent_dir_path) {
			return;
		}

		qAppSettings.setValue(kAppSettingFileSystemLastOpenPath, parent_dir_path);

        ui_->page->playlist()->removeAll();
		file_path_ = parent_dir_path;

        auto dir_name = file_info.fileName();
        if (!file_info.isDir()) {
            dir_name = QFileInfo(parent_dir_path).fileName();
        }

		QFontMetrics fm(ui_->page->pageTitle()->font());
        auto title = fm.elidedText(qFormat("Scanning \"%1\"...")
			.arg(dir_name),
            Qt::ElideRight,
            800);
        ui_->page->pageTitle()->setText(title);

        if (scanner_stop_source_) {
            scanner_stop_source_->request_stop();
        }
        scanner_stop_source_ = std::make_shared<std::stop_source>();
        const auto scan_generation = ++scanner_generation_;

        progress_page_->show();
        progress_page_->setOnlyShowProgress();
        const auto list_view_rect = this->rect();
        progress_page_->setFixedSize(QSize(list_view_rect.size().width() - 2, 80));
        progress_page_->move(0, height() - 80);
		progress_page_->setFileCount(0);
        progress_page_->setFileProgress(0);

        const auto native_path = toNativeSeparators(parent_dir_path);
        const auto display_dir_name = dir_name;
        QPointer<FileSystemViewPage> receiver(this);
        auto stop_source = scanner_stop_source_;
        auto scanner = metadata_scanner_;

        auto* scanner_thread = QThread::create([receiver,
            scanner,
            stop_source,
            native_path,
            display_dir_name,
            scan_generation]() {
            MetadataScanOptions options;
            options.recursive = false;
            MetadataScanCallbacks callbacks;
            callbacks.on_found_file_count = [receiver, display_dir_name, scan_generation](size_t total_file_count) {
                if (receiver.isNull()) {
                    return;
                }
                QMetaObject::invokeMethod(receiver.data(), [receiver, display_dir_name, scan_generation, total_file_count]() {
                    if (receiver.isNull()
                        || scan_generation != receiver->scanner_generation_) {
                        return;
                    }
                    receiver->progress_page_->setFileCount(static_cast<int32_t>(total_file_count));
                    QFontMetrics fm(receiver->ui_->page->pageTitle()->font());
                    const auto title = fm.elidedText(qFormat("Files in \"%1\" (%2 songs)")
                        .arg(display_dir_name)
                        .arg(QString::number(total_file_count)),
                        Qt::ElideRight,
                        800);
                    receiver->ui_->page->pageTitle()->setText(title);
                    }, Qt::QueuedConnection);
                };
            callbacks.on_progress = [receiver, scan_generation](MetadataScanProgress progress) {
                if (receiver.isNull()) {
                    return;
                }
                QMetaObject::invokeMethod(receiver.data(), [receiver, scan_generation, progress]() {
                    if (receiver.isNull()
                        || scan_generation != receiver->scanner_generation_
                        || progress.total_work == 0) {
                        return;
                    }
                    receiver->progress_page_->setFileProgress(
                        static_cast<int32_t>((progress.completed_work * 100) / progress.total_work));
                    }, Qt::QueuedConnection);
                };
            callbacks.on_batch_tracks = [receiver, scan_generation](auto tracks) {
                if (receiver.isNull()) {
                    return;
                }
                QMetaObject::invokeMethod(receiver.data(), [receiver, scan_generation, tracks = std::move(tracks)]() mutable {
                    if (receiver.isNull()
                        || scan_generation != receiver->scanner_generation_) {
                        return;
                    }
                    qDatabaseFacade.insertMultipleTrackInfo(tracks,
                        kFileSystemPlaylistId,
                        StoreType::LOCAL_STORE,
                        QString(),
                        DatabaseFacade::kSkipFetchCover);
                    receiver->ui_->page->playlist()->reload();
                    }, Qt::QueuedConnection);
                };
            callbacks.on_tracks = [receiver, scan_generation](auto tracks) {
                if (receiver.isNull()) {
                    return;
                }
                QMetaObject::invokeMethod(receiver.data(), [receiver, scan_generation, tracks = std::move(tracks)]() mutable {
                    if (receiver.isNull()
                        || scan_generation != receiver->scanner_generation_) {
                        return;
                    }
                    qDatabaseFacade.insertTrackInfo(tracks,
                        kFileSystemPlaylistId,
                        StoreType::LOCAL_STORE,
                        QString(),
                        DatabaseFacade::kSkipFetchCover);
                    receiver->ui_->page->playlist()->reload();
                    }, Qt::QueuedConnection);
                };

            try {
                scanner->Scan(Path(native_path.toStdWString()),
                    stop_source->get_token(),
                    callbacks,
                    options);
            }
            catch (const std::exception& e) {
                XAMP_LOG_ERROR("Metadata scan failed path:{} error:{}",
                    native_path.toStdString(),
                    e.what());
            }

            if (receiver.isNull()) {
                return;
            }
            QMetaObject::invokeMethod(receiver.data(), [receiver, scan_generation]() {
                if (receiver.isNull()
                    || scan_generation != receiver->scanner_generation_) {
                    return;
                }
                receiver->progress_page_->setFileProgress(100);
                receiver->progress_page_->hide();
                receiver->ui_->page->playlist()->reload();
                }, Qt::QueuedConnection);
            });
        (void)QObject::connect(scanner_thread,
            &QThread::finished,
            scanner_thread,
            &QObject::deleteLater);
        scanner_thread->start(QThread::LowestPriority);
        
        });

    (void)QObject::connect(ui_->dirTree,
        &QTreeView::customContextMenuRequested, [this](auto pt) {
        ActionMap<QTreeView, std::function<void(const QPoint&)>> action_map(ui_->dirTree);

        auto add_file_to_playlist_act = action_map.addAction(
            tr("Add file directory to playlist"),
            [this](auto pt) {
            auto index = ui_->dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto src_index = dir_first_sort_filter_->mapToSource(index);
            auto path = toNativeSeparators(
                dir_model_->fileInfo(src_index).filePath());
            emit addPathToPlaylist(path, true);
        });
        add_file_to_playlist_act->setIcon(
            qTheme.fontIcon(Glyphs::ICON_PLAYLIST));

        auto load_dir_act = action_map.addAction(
            tr("Load file directory"), [this](auto pt) {
            const auto dir_name = 
                getExistingDirectory(this, tr("Select a directory"));
            if (dir_name.isEmpty()) {
                return;
            }
            qAppSettings.setValue(
                kAppSettingMyMusicFolderPath, dir_name);
            ui_->dirTree->setRootIndex(
                dir_first_sort_filter_->mapFromSource(
                    dir_model_->index(qAppSettings.myMusicFolderPath())));
        });
        load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER));

        action_map.exec(pt, pt);
        });
    setStyleSheet("background-color: transparent; border: none;"_str);

    ui_->dirTree->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical { width: 6px; }"_str);

    qTheme.setLineEditStyle(ui_->searchLineEdit, "searchLineEdit"_str);
    auto search_line_action = ui_->searchLineEdit->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::TrailingPosition);
    ui_->searchLineEdit->setPlaceholderText(tr("Search directory name/file name"));

    (void)QObject::connect(ui_->searchLineEdit, &QLineEdit::returnPressed, [this]() {
        const auto text = ui_->searchLineEdit->text();
        dir_first_sort_filter_->setSearchString(text);
        });

}

void FileSystemViewPage::setScannerThreadPool(std::shared_ptr<IThreadPoolExecutor> scanner_thread_pool) {
    XAMP_ASSERT(scanner_thread_pool);
    metadata_scanner_ = std::make_shared<MetadataLibraryScanner>(std::move(scanner_thread_pool));
}

PlaylistPage* FileSystemViewPage::playlistPage() {
    return ui_->page;
}

SpectrogramWidget* FileSystemViewPage::spectrogramWidget() {
    return ui_->spectrogramWidget;
}

FileSystemViewPage::~FileSystemViewPage() {
    ++scanner_generation_;
    if (scanner_stop_source_) {
        scanner_stop_source_->request_stop();
    }
    delete ui_;
}
