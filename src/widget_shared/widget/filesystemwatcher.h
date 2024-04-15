//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QStringList>

#include <widget/widget_shared_global.h>

class QFileSystemWatcher;
class QTimer;

class XAMP_WIDGET_SHARED_EXPORT FileSystemWatcher : public QObject {
    Q_OBJECT
public:
    explicit FileSystemWatcher(QObject* parent = nullptr);

    void addPath(const QString& path);

    void addPaths(const QStringList& paths);

    void removePath(const QString& path);

    void removePaths(const QStringList& paths);

signals:
    void directoryChanged(const QString&);

private slots:
    void onDirectoryChanged(const QString& path);

    void onTimerTimeOut();

private:
    size_t last_dir_size_;
    QFileSystemWatcher* watcher_;
    QTimer* timer_;
    QString changed_dir_;
};

