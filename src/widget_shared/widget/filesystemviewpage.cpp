#include <ui_filesystemviewpage.h>
#include <widget/filesystemviewpage.h>

#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/appsettingnames.h>
#include <widget/actionmap.h>
#include <widget/playlisttableview.h>
#include <widget/filesystemmodel.h>
#include <widget/util/ui_util.h>
#include <widget/chatgpt/librosa.h>

#include <QHelpEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QToolTip>

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
        if (sourceModel()->data(left).toString() == qTEXT(".."))
            return asc;
        if (sourceModel()->data(right).toString() == qTEXT(".."))
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
    dir_model_->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    dir_model_->setRootPath(qAppSettings.myMusicFolderPath());
    dir_model_->setNameFilters(getTrackInfoFileNameFilter());
    dir_model_->setNameFilterDisables(false);

    dir_first_sort_filter_ = new DirFirstSortFilterProxyModel(this);
    dir_first_sort_filter_->setSourceModel(dir_model_);
    dir_first_sort_filter_->setFilterKeyColumn(0);

    ui_->dirTree->setModel(dir_first_sort_filter_);
    ui_->dirTree->setItemDelegate(new DisableToolTipStyledItemDelegate(this));
    ui_->dirTree->setRootIndex(dir_first_sort_filter_->mapFromSource(dir_model_->index(qAppSettings.myMusicFolderPath())));
    ui_->dirTree->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->dirTree->setSortingEnabled(true);

    ui_->dirTree->header()->hide();
    ui_->dirTree->hideColumn(1);
    ui_->dirTree->hideColumn(2);
    ui_->dirTree->hideColumn(3);

    service_.moveToThread(&thread_);
    thread_.start();
    service_.initialAsync().waitForFinished();

    ui_->dirTree->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(ui_->dirTree, &QTreeView::customContextMenuRequested, [this](auto pt) {
        ActionMap<QTreeView, std::function<void(const QPoint&)>> action_map(ui_->dirTree);

        action_map.addAction(tr("Add file directory to album"), [this](auto pt) {
            auto index = ui_->dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto src_index = dir_first_sort_filter_->mapToSource(index);
            auto path = toNativeSeparators(dir_model_->fileInfo(src_index).filePath());

            emit addPathToPlaylist(path, false);
            });

        auto add_file_to_playlist_act = action_map.addAction(tr("Add file directory to playlist"), [this](auto pt) {
            auto index = ui_->dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto src_index = dir_first_sort_filter_->mapToSource(index);
            auto path = toNativeSeparators(dir_model_->fileInfo(src_index).filePath());

            service_.loadAsync(path).waitForFinished();
            //emit addPathToPlaylist(path, true);
        });
        add_file_to_playlist_act->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));

        auto load_dir_act = action_map.addAction(tr("Load file directory"), [this](auto pt) {
            const auto dir_name = getExistingDirectory(this, tr("Select a directory"));
            if (dir_name.isEmpty()) {
                return;
            }
            qAppSettings.setValue(kAppSettingMyMusicFolderPath, dir_name);
            ui_->dirTree->setRootIndex(dir_first_sort_filter_->mapFromSource(dir_model_->index(qAppSettings.myMusicFolderPath())));
        });
        load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER));

        action_map.exec(pt, pt);
        });
    setStyleSheet(qTEXT("background-color: transparent; border: none;"));
}

FileSystemViewPage::~FileSystemViewPage() {
    delete ui_;
}
