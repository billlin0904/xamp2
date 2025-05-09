#include <widget/logview.h>
#include <thememanager.h>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QFileInfo>

#include <ui_logview.h>

LogView::LogView(QWidget * parent)
    : QWidget(parent)
    , timer_(new QTimer(this))
    , lastFileSize_(0) {
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    ui_ = new Ui::LogView();
    ui_->setupUi(this);

    ui_->logViewerEdit->setFont(qTheme.debugFont());
    
    (void)QObject::connect(ui_->searchButton, &QPushButton::clicked, this, &LogView::findNext);
    (void)QObject::connect(timer_, &QTimer::timeout, this, &LogView::checkFileUpdate);
}

LogView::~LogView() {
    delete ui_;
}

// ���J log �ɮר���ܩ� logViewer_
bool LogView::loadLogFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {        
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    ui_->logViewerEdit->clear();
    ui_->logViewerEdit->appendPlainText(content);

    QFileInfo info(filePath);
    lastFileSize_ = info.size();
    logFilePath_ = filePath;

    // �N��в��ʨ�̫�A�T�O�����J�I�]�b�̫�@��
    ui_->logViewerEdit->moveCursor(QTextCursor::End);
    // �T�O���Хi���]�]�N�O�۰ʱ��ʨ쩳���^
    ui_->logViewerEdit->ensureCursorVisible();

    timer_->start(1000);

    return true;
}

void LogView::onAppendLog(const QString& logText) {
    ui_->logViewerEdit->appendPlainText(logText);

    ui_->logViewerEdit->moveCursor(QTextCursor::End);
    ui_->logViewerEdit->ensureCursorVisible();
}

void LogView::appendNewLogs(qint64 startPos, qint64 endPos) {
    QFile file(logFilePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    // �N�ɮ׫��в���W��Ū����������m
    if (!file.seek(startPos)) {
        return;
    }

    // Ū���s���e
    qint64 bytesToRead = endPos - startPos;
    QByteArray data = file.read(bytesToRead);
    file.close();

    // ���]�O UTF-8 �s�X
    QString newContent = QString::fromUtf8(data);

    // �l�[�� logViewer_
    ui_->logViewerEdit->moveCursor(QTextCursor::End);
    ui_->logViewerEdit->insertPlainText(newContent);

    // �۰ʱ��ʨ쩳��
    ui_->logViewerEdit->ensureCursorVisible();
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
    if (currentSize > lastFileSize_) {
        appendNewLogs(lastFileSize_, currentSize);
        lastFileSize_ = currentSize;
    }
}