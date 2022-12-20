#include <QFileDialog>
#include <QToolTip>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/appsettingnames.h>
#include <widget/actionmap.h>
#include <widget/playlisttableview.h>
#include <widget/filesystemmodel.h>
#include <widget/filesystemviewpage.h>

FileSystemViewPage::FileSystemViewPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);

    setFrameStyle(QFrame::StyledPanel);
    dir_model_ = new FileSystemModel(this);
    dir_model_->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    dir_model_->setRootPath(AppSettings::getMyMusicFolderPath());
    ui.dirTree->setModel(dir_model_);
    ui.dirTree->setRootIndex(dir_model_->index(AppSettings::getMyMusicFolderPath()));
    ui.dirTree->setStyleSheet(qTEXT("background-color: transparent"));

    ui.dirTree->header()->hide();
    ui.dirTree->hideColumn(1);
    ui.dirTree->hideColumn(2);
    ui.dirTree->hideColumn(3);

    QStringList filter;
    for (auto& file_ext : GetSupportFileExtensions()) {
        filter << qSTR("*%1").arg(QString::fromStdString(file_ext));
    }
    dir_model_->setNameFilters(filter);
    dir_model_->setNameFilterDisables(false);

    ui.dirTree->setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(ui.dirTree, &QTreeView::customContextMenuRequested, [this](auto pt) {
        ActionMap<QTreeView, std::function<void(const QPoint&)>> action_map(ui.dirTree);

        auto add_file_to_playlist_act = action_map.addAction(tr("Add file directory to playlist"), [this](auto pt) {
            auto index = ui.dirTree->indexAt(pt);
            if (!index.isValid()) {
                return;
            }
            auto path = fromQStringPath(dir_model_->fileInfo(index).filePath());
            ui.playlistPage->playlist()->append(path, false, false);
            });
        add_file_to_playlist_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_PLAYLIST));

        auto load_dir_act = action_map.addAction(tr("Load file directory"), [this](auto pt) {
            const auto dir_name = QFileDialog::getExistingDirectory(this,
                tr("Select a Directory"),
                AppSettings::getMyMusicFolderPath(), QFileDialog::ShowDirsOnly);
            if (dir_name.isEmpty()) {
                return;
            }
            AppSettings::setValue(kAppSettingMyMusicFolderPath, dir_name);
            ui.dirTree->setRootIndex(dir_model_->index(AppSettings::getMyMusicFolderPath()));
            });
        load_dir_act->setIcon(qTheme.iconFromFont(Glyphs::ICON_FOLDER));

        action_map.exec(pt, pt);
        });

    setStyleSheet(qTEXT("background-color: transparent"));
    ui.playlistPage->playlist()->setFocusPolicy(Qt::StrongFocus);
    playlistPage()->playlist()->disableDelete();
    playlistPage()->playlist()->disableLoadFile();
}

PlaylistPage* FileSystemViewPage::playlistPage() {
    return ui.playlistPage;
}