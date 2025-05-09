//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextCursor>
#include <QFont>
#include <QLabel>
#include <QPlainTextEdit>
#include <QLineEdit>

#include <widget/widget_shared_global.h>

namespace Ui {
    class LogView;
}

class XAMP_WIDGET_SHARED_EXPORT LogView : public QWidget {
    Q_OBJECT
public:
    explicit LogView(QWidget* parent = nullptr);

    virtual ~LogView() override;
    
    bool loadLogFile(const QString& filePath);

public slots:
    void onAppendLog(const QString& logText);

    void findNext();
	
    void checkFileUpdate();
private:
    void appendNewLogs(qint64 startPos, qint64 endPos);    

    QTimer* timer_;
    QString  logFilePath_;
    qint64   lastFileSize_;
    Ui::LogView* ui_;
};
