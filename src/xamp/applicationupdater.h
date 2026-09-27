#pragma once
#include <QObject>
#include <QPointer>
#include <QNetworkAccessManager>
#include <QCryptographicHash>
#include <QFile>
#include <future>
class QWidget;
class QNetworkReply;
class ApplicationUpdater final : public QObject {
    Q_OBJECT
public:
    explicit ApplicationUpdater(QObject* parent = nullptr);
    void start();
    void checkNow();
    QWidget* createSettingsPage(QWidget* parent);
signals:
    void changed();
    void installing();
private:
    friend class ApplicationUpdaterTest;
    enum class State { Idle, Checking, Available, Downloading, Ready, Verifying, Failed };
    void check();
    void download();
    void cancelDownload();
    void verifyAndInstall();
    std::future<QString> verification_;
    bool cancelled_{false};
    void installDownloadedUpdate(const QString& url, const QString& filepath);
    void fail(const QString& message);
    QNetworkAccessManager network_;
    QPointer<QNetworkReply> reply_;
    QFile package_;
    QCryptographicHash hash_{QCryptographicHash::Sha256};
    State state_{State::Idle};
    QString version_, download_url_, sha256_, changelog_, error_;
    qint64 received_{0}, total_{0};
};
