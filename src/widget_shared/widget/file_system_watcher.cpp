#include <widget/file_system_watcher.h>
#include <widget/util/ui_utilts.h>

#include <QFileSystemWatcher>
#include <QTimer>
#include <QDirIterator>
#include <QDir>
#include <QFileInfo>

FileSystemWatcher::FileSystemWatcher(QObject* parent)
    : QObject(parent)
	, last_dir_size_(0) {
    watcher_ = new QFileSystemWatcher(this);
    timer_ = new QTimer(this);
    timer_->setSingleShot(true);

    (void)QObject::connect(timer_, &QTimer::timeout, this, &FileSystemWatcher::onTimerTimeOut);
    (void)QObject::connect(watcher_, &QFileSystemWatcher::directoryChanged, this, &FileSystemWatcher::onDirectoryChanged);
}

void FileSystemWatcher::addPath(const QString& path) {
    watcher_->addPath(path);
}

void FileSystemWatcher::addPaths(const QStringList& paths) {
    if (paths.isEmpty())
        return;

    watcher_->addPaths(paths);
}

void FileSystemWatcher::removePath(const QString& path) {
    watcher_->removePath(path);
}

void FileSystemWatcher::removePaths(const QStringList& paths) {
    if (paths.isEmpty())
        return;

    watcher_->removePaths(paths);
}

void FileSystemWatcher::onDirectoryChanged(const QString& path) {
    changed_dir_ = path;
    timer_->stop();
    timer_->start(500);
}

void FileSystemWatcher::onTimerTimeOut() {
    // Wait until the folder no longer changes
    auto size = getFileCount(changed_dir_, getTrackInfoFileNameFilter());
    if (last_dir_size_ != size) {
        last_dir_size_ = size;
        timer_->start(500);
        return;
    }

    emit directoryChanged(changed_dir_);
    last_dir_size_ = 0;
    changed_dir_.clear();
}
