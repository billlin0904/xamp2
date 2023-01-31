#include <widget/filesystemviewpage.h>

#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/appsettingnames.h>
#include <widget/actionmap.h>
#include <widget/playlisttableview.h>
#include <widget/filesystemmodel.h>

#include <QDateTime>
#include <QFileDialog>
#include <QToolTip>
#include <QSortFilterProxyModel>

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
    ui.setupUi(this);

    setFrameStyle(QFrame::StyledPanel);

    dir_model_ = new FileSystemModel(this);
    dir_model_->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    dir_model_->setRootPath(AppSettings::GetMyMusicFolderPath());
    dir_model_->setNameFilters(GetFileNameFilter());
    dir_model_->setNameFilterDisables(false);

    dir_first_sort_filter_ = new DirFirstSortFilterProxyModel(this);
    dir_first_sort_filter_->setSourceModel(dir_model_);
    dir_first_sort_filter_->setFilterKeyColumn(0);

    ui.dirTree->setModel(dir_first_sort_filter_);
    ui.dirTree->setRootIndex(dir_first_sort_filter_->mapFromSource(dir_model_->index(AppSettings::GetMyMusicFolderPath())));
    ui.dirTree->setStyleSheet(qTEXT("background-color: transparent"));
    ui.dirTree->setSortingEnabled(true);

    ui.dirTree->header()->hide();
    ui.dirTree->hideColumn(1);
    ui.dirTree->hideColumn(2);
    ui.dirTree->hideColumn(3);    

    ui.dirTree->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(ui.dirTree, &QTreeView::customContextMenuRequested, [this](auto pt) {
        ActionMap<QTreeView, std::function<void(const QPoint&)>> action_map(ui.dirTree);

        auto add_file_to_playlist_act = action_map.AddAction(tr("Add file directory to playlist"), [this](auto pt) {
            auto index = ui.dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto src_index = dir_first_sort_filter_->mapToSource(index);
            auto path = toNativeSeparators(dir_model_->fileInfo(src_index).filePath());
            ui.playlistPage->playlist()->append(path);
        });
        add_file_to_playlist_act->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));

        auto load_dir_act = action_map.AddAction(tr("Load file directory"), [this](auto pt) {
            const auto dir_name = QFileDialog::getExistingDirectory(this,
                tr("Select a Directory"),
                AppSettings::GetMyMusicFolderPath(), QFileDialog::ShowDirsOnly);
            if (dir_name.isEmpty()) {
                return;
            }
            AppSettings::SetValue(kAppSettingMyMusicFolderPath, dir_name);
            ui.dirTree->setRootIndex(dir_first_sort_filter_->mapFromSource(dir_model_->index(AppSettings::GetMyMusicFolderPath())));
        });
        load_dir_act->setIcon(qTheme.fontIcon(Glyphs::ICON_FOLDER));

        action_map.exec(pt, pt);
        });

    setStyleSheet(qTEXT("background-color: transparent"));
    playlistPage()->playlist()->disableDelete();
    playlistPage()->playlist()->disableLoadFile();
}

PlaylistPage* FileSystemViewPage::playlistPage() {
    return ui.playlistPage;
}
