#include <QFileDialog>
#include <QToolTip>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/actionmap.h>
#include <widget/playlisttableview.h>
#include <widget/filesystemmodel.h>
#include <widget/filesystemviewpage.h>

FileSystemViewPage::FileSystemViewPage(QWidget* parent)
    : QWidget(parent) {
    ui.setupUi(this);
    dir_model_ = new FileSystemModel(this);
    dir_model_->setFilter(QDir::NoDotDot | QDir::AllDirs);
    dir_model_->setRootPath(AppSettings::getMyMusicFolderPath());
    ui.dirTree->setModel(dir_model_);
    ui.dirTree->setRootIndex(dir_model_->index(AppSettings::getMyMusicFolderPath()));
    ui.dirTree->setStyleSheet(Q_TEXT("background-color: transparent"));

    ui.dirTree->hideColumn(1);
    ui.dirTree->hideColumn(2);
    ui.dirTree->hideColumn(3);

    ui.dirTree->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(ui.dirTree, &QTreeView::customContextMenuRequested, [this](auto pt) {
        ActionMap<QTreeView> action_map(ui.dirTree);
        (void)action_map.addAction(tr("Load file directory"), [this]() {
            const auto dir_name = QFileDialog::getExistingDirectory(this,
                tr("Select a Directory"),
                AppSettings::getMyMusicFolderPath(), QFileDialog::ShowDirsOnly);
            if (dir_name.isEmpty()) {
                return;
            }
            AppSettings::setValue(kAppSettingMyMusicFolderPath, dir_name);
            ui.dirTree->setRootIndex(dir_model_->index(AppSettings::getMyMusicFolderPath()));
            });

        action_map.exec(pt);
        });

    (void) QObject::connect(ui.dirTree, &QTreeView::clicked, [this](const auto &index) {
        auto path = dir_model_->fileInfo(index).filePath();
        ui.playlistPage->playlist()->removeAll();
        ui.playlistPage->playlist()->append(path, false, false);
        emit ui.playlistPage->setCover(Qt::EmptyString, ui.playlistPage);
        });
}

PlaylistPage* FileSystemViewPage::playlistPage() {
    return ui.playlistPage;
}