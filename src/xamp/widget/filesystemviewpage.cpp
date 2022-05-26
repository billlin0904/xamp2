#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/playlisttableview.h>
#include <widget/filesystemmodel.h>
#include <widget/filesystemviewpage.h>

FileSystemViewPage::FileSystemViewPage(QWidget* parent)
    : QWidget(parent) {
    ui.setupUi(this);
    dir_model_ = new FileSystemModel(this);
    dir_model_->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);    
    dir_model_->setRootPath(AppSettings::getMyMusicFolderPath());
    ui.dirTree->setModel(dir_model_);
    ui.dirTree->setRootIndex(dir_model_->index(AppSettings::getMyMusicFolderPath()));
    ui.dirTree->setStyleSheet(Q_TEXT("background-color: transparent"));
    ui.playlistPage->playlist()->setPlaylistId(kDefaultFileExplorerPlaylistId);
    
    ui.dirTree->hideColumn(1);
    ui.dirTree->hideColumn(2);
    ui.dirTree->hideColumn(3);

    (void) QObject::connect(ui.dirTree, &QTreeView::clicked, [this](const auto &index) {
        auto path = dir_model_->fileInfo(index).filePath();
        ui.playlistPage->playlist()->removeAll();
        ui.playlistPage->playlist()->append(path, false, false);
        });
}

PlaylistPage* FileSystemViewPage::playlistPage() {
    return ui.playlistPage;
}