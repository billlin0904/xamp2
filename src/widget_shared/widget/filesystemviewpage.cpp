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

#include <base/workstealingtaskqueue.h>

#include <QDirIterator>
#include <QLabel>
#include <QHelpEvent>
#include <QDateTime>
#include <QFileDialog>
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
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
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

    auto last_open_path = qAppSettings.valueAsString(
        kAppSettingFileSystemLastOpenPath);

	if (last_open_path.isEmpty()) {
		last_open_path = qAppSettings.myMusicFolderPath();
	}

    dir_model_->setRootPath(qAppSettings.myMusicFolderPath());
    dir_model_->setFilter(
        QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    dir_model_->setNameFilters(getTrackInfoFileNameFilter());
    dir_model_->setNameFilterDisables(false);

    auto last_open_index = dir_model_->index(last_open_path);
    auto proxy_index = dir_first_sort_filter_->mapFromSource(last_open_index);
    ui_->dirTree->setCurrentIndex(proxy_index);
    ui_->dirTree->expand(proxy_index);

    (void)QObject::connect(ui_->dirTree, &QTreeView::clicked,
        [this](const auto &index) {
        auto src_index = dir_first_sort_filter_->mapToSource(index);
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

		qAppSettings.setValue(kAppSettingFileSystemLastOpenPath,
            parent_dir_path);

        ui_->page->playlist()->removeAll();

		file_path_ = parent_dir_path;
        ConcurrentQueue<TrackInfo> track_queue;

        QDirIterator itr(parent_dir_path, getTrackInfoFileNameFilter(),
            QDir::NoDotAndDotDot | QDir::Files);

        std::vector<std::wstring> file_paths;
        file_paths.reserve(20);

    	while (itr.hasNext()) {
            const auto next_path = toNativeSeparators(itr.next());
            const auto path_str = next_path.toStdWString();
			file_paths.push_back(path_str);
        }

        Executor::ParallelFor(getMainWindow()->threadPool(),
            file_paths, 
            [&track_queue](const auto& path) {
            const Path file_path(path);
            if (file_path.extension() == kCueFileExtension) {
                return;
            }
            try {
                auto reader = MakeMetadataReader();
                reader->Open(file_path);
                track_queue.enqueue(reader->Extract());
            }
            catch (const Exception& e) {
                XAMP_LOG_DEBUG("Failed to extract file: {}", e.GetStackTrace());
            }
            catch (...) {
            }
            });

        ForwardList<TrackInfo> track_infos;

        TrackInfo temp;
    	while (track_queue.try_dequeue(temp)) {
            track_infos.push_front(temp);
        }

        track_infos.sort([](const auto& first, const auto& last) {
            return first.track < last.track;
            });

        DatabaseFacade facade;
        facade.insertTrackInfo(track_infos,
            kFileSystemPlaylistId, 
            StoreType::LOCAL_STORE);

        ui_->page->playlist()->reload();
        });
    (void)QObject::connect(ui_->dirTree,
        &QTreeView::customContextMenuRequested, [this](auto pt) {
        ActionMap<QTreeView, std::function<void(const QPoint&)>> action_map(ui_->dirTree);

        action_map.addAction(tr("Add file directory to album"),
            [this](auto pt) {
            auto index = ui_->dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto src_index = dir_first_sort_filter_->mapToSource(index);
            auto path = toNativeSeparators(
                dir_model_->fileInfo(src_index).filePath());

            emit addPathToPlaylist(path, false);
            });

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
}

PlaylistPage* FileSystemViewPage::playlistPage() {
    return ui_->page;
}

WaveformWidget* FileSystemViewPage::waveformWidget() {
    return ui_->frame_2;
}

FileSystemViewPage::~FileSystemViewPage() {
    delete ui_;
}
