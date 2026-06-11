#include <widget/logview.h>
#include <thememanager.h>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QFileInfo>
#include <QSignalBlocker>

#include <ui_logview.h>

LogView::LogView(QWidget * parent)
    : QWidget(parent)
    , file_watcher_(new QFileSystemWatcher(this))
    , timer_(new QTimer(this))
    , lastFileSize_(0) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_ = new Ui::LogView();
    ui_->setupUi(this);

    ui_->logViewerEdit->setFont(qTheme.debugFont());
    
    (void)QObject::connect(ui_->searchButton, &QPushButton::clicked, this, &LogView::findNext);
    (void)QObject::connect(timer_, &QTimer::timeout, this, &LogView::checkFileUpdate);
    (void)QObject::connect(file_watcher_, &QFileSystemWatcher::fileChanged, this, [this](const QString& path) {
        if (path != logFilePath_) {
            return;
        }
        checkFileUpdate();
        watchLogFile();
        });
}

LogView::~LogView() {
    delete ui_;
}

// 載入 log 檔案並顯示於 logViewer_
bool LogView::loadLogFile(const QString& filePath) {
    logFilePath_ = filePath;

    QFile file_(filePath);
    if (!file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lastFileSize_ = 0;
        watchLogFile();
        timer_->start(250);
        return false;
    }

    QTextStream in(&file_);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file_.close();

    ui_->logViewerEdit->clear();
    ui_->logViewerEdit->appendPlainText(content);

    QFileInfo info(filePath);
    lastFileSize_ = info.size();
    watchLogFile();

    // 將游標移動到最後，確保視窗焦點也在最後一行
    ui_->logViewerEdit->moveCursor(QTextCursor::End);
    // 確保光標可見（也就是自動捲動到底部）
    ui_->logViewerEdit->ensureCursorVisible();

    timer_->start(250);

    return true;
}

void LogView::onAppendLog(const QString& logText) {
    ui_->logViewerEdit->appendPlainText(logText);

    ui_->logViewerEdit->moveCursor(QTextCursor::End);
    ui_->logViewerEdit->ensureCursorVisible();
}

void LogView::appendNewLogs(qint64 startPos, qint64 endPos) {
    QFile file_(logFilePath_);
    if (!file_.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    // 將檔案指標移到上次讀取結束的位置
    if (!file_.seek(startPos)) {
        return;
    }

    // 讀取新內容
    qint64 bytesToRead = endPos - startPos;
    QByteArray data = file_.read(bytesToRead);
    file_.close();

    // 假設是 UTF-8 編碼
    QString newContent = QString::fromUtf8(data);

    // 追加到 logViewer_
    ui_->logViewerEdit->moveCursor(QTextCursor::End);
    ui_->logViewerEdit->insertPlainText(newContent);

    // 自動捲動到底部
    ui_->logViewerEdit->ensureCursorVisible();
}

void LogView::watchLogFile() {
    if (logFilePath_.isEmpty()) {
        return;
    }

    const QSignalBlocker blocker(file_watcher_);
    const auto watched_files = file_watcher_->files();
    if (!watched_files.isEmpty()) {
        file_watcher_->removePaths(watched_files);
    }

    if (QFileInfo::exists(logFilePath_)) {
        file_watcher_->addPath(logFilePath_);
    }
}

void LogView::findNext() {
    QString searchText = ui_->lineEdit->text().trimmed();
    if (searchText.isEmpty()) {
        return;
    }

    QTextDocument::FindFlags options = QTextDocument::FindCaseSensitively;

    bool found = ui_->logViewerEdit->find(searchText, options);
    if (!found) {
        QTextCursor cursor = ui_->logViewerEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        ui_->logViewerEdit->setTextCursor(cursor);
        found = ui_->logViewerEdit->find(searchText, options);
    }
}

void LogView::checkFileUpdate() {
    if (logFilePath_.isEmpty()) {
        return;
    }

    QFileInfo info(logFilePath_);
    if (!info.exists()) {
        return;
    }

    qint64 currentSize = info.size();
    if (currentSize < lastFileSize_) {
        loadLogFile(logFilePath_);
        return;
    }

    if (currentSize > lastFileSize_) {
        appendNewLogs(lastFileSize_, currentSize);
        lastFileSize_ = currentSize;
    }
}
