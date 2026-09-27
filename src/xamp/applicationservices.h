#pragma once
#include <QThread>
#include <QScopedPointer>
class FileSystemService;
class AlbumCoverService;
class BackgroundService;

// Worker ownership and teardown are kept together, outside the main window.
class ApplicationServices final {
public:
    ApplicationServices();
    ~ApplicationServices();
    void shutdown();
    FileSystemService* files() const { return files_.get(); }
    AlbumCoverService* covers() const { return covers_.get(); }
    BackgroundService* background() const { return background_.get(); }
private:
    QThread files_thread_, covers_thread_, background_thread_;
    QScopedPointer<FileSystemService> files_;
    QScopedPointer<AlbumCoverService> covers_;
    QScopedPointer<BackgroundService> background_;
};
