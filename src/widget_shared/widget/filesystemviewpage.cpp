#include <ui_filesystemviewpage.h>
#include <widget/filesystemviewpage.h>

#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/appsettingnames.h>
#include <widget/actionmap.h>
#include <widget/playlisttableview.h>
#include <widget/filesystemmodel.h>
#include <widget/ui_utilts.h>

#include <QDateTime>
#include <QFileDialog>
#include <QSortFilterProxyModel>

class XAMP_WIDGET_SHARED_EXPORT FileSystemViewPage::DirFirstSortFilterProxyModel : public QSortFilterProxyModel {
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
    dir_model_->setRootPath(qAppSettings.GetMyMusicFolderPath());
    dir_model_->setNameFilters(GetTrackInfoFileNameFilter());
    dir_model_->setNameFilterDisables(false);

    dir_first_sort_filter_ = new DirFirstSortFilterProxyModel(this);
    dir_first_sort_filter_->setSourceModel(dir_model_);
    dir_first_sort_filter_->setFilterKeyColumn(0);

    ui_->dirTree->setModel(dir_first_sort_filter_);
    ui_->dirTree->setRootIndex(dir_first_sort_filter_->mapFromSource(dir_model_->index(qAppSettings.GetMyMusicFolderPath())));
    ui_->dirTree->setStyleSheet(qTEXT("background-color: transparent"));
    ui_->dirTree->setSortingEnabled(true);

    ui_->dirTree->header()->hide();
    ui_->dirTree->hideColumn(1);
    ui_->dirTree->hideColumn(2);
    ui_->dirTree->hideColumn(3);    

    ui_->dirTree->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(ui_->dirTree, &QTreeView::customContextMenuRequested, [this](auto pt) {
        ActionMap<QTreeView, std::function<void(const QPoint&)>> action_map(ui_->dirTree);

        action_map.AddAction(tr("Add file directory to album"), [this](auto pt) {
            auto index = ui_->dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto src_index = dir_first_sort_filter_->mapToSource(index);
            auto path = ToNativeSeparators(dir_model_->fileInfo(src_index).filePath());
            emit ExtractFile(path, -1, false);
            });

        auto add_file_to_playlist_act = action_map.AddAction(tr("Add file directory to playlist"), [this](auto pt) {
            auto index = ui_->dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto src_index = dir_first_sort_filter_->mapToSource(index);
            auto path = ToNativeSeparators(dir_model_->fileInfo(src_index).filePath());
            ui_->playlistPage->playlist()->append(path);
            ui_->playlistPage->playlist()->Reload();
        });
        add_file_to_playlist_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST));

        auto load_dir_act = action_map.AddAction(tr("Load file directory"), [this](auto pt) {
            const auto dir_name = GetExistingDirectory(this);
            if (dir_name.isEmpty()) {
                return;
            }
            qAppSettings.SetValue(kAppSettingMyMusicFolderPath, dir_name);
            ui_->dirTree->setRootIndex(dir_first_sort_filter_->mapFromSource(dir_model_->index(qAppSettings.GetMyMusicFolderPath())));
        });
        load_dir_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_FOLDER));

        action_map.exec(pt, pt);
        });

    setStyleSheet(qTEXT("background-color: transparent"));
    playlistPage()->playlist()->DisableDelete();
    playlistPage()->playlist()->DisableLoadFile();
}

FileSystemViewPage::~FileSystemViewPage() {
    delete ui_;
}

PlaylistPage* FileSystemViewPage::playlistPage() {
    return ui_->playlistPage;
}
