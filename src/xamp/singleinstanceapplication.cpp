#include <QCoreApplication>
#include <QDir>
#include <QCryptographicHash>
#include <QString>
#include <QLocalSocket>

#include <base/logger.h>
#include <base/exception.h>
#include <widget/str_utilts.h>
#include <singleinstanceapplication.h>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

static QString GetServerName() {
    //TODO: use applicationFilePath?
    //QString serverNameSource = QCoreApplication::applicationFilePath();
    QString serverNameSource;
    serverNameSource += QDir::home().dirName();
    serverNameSource += QSysInfo::machineHostName();
    return QString(Q_TEXT(QCryptographicHash::hash(serverNameSource.toUtf8(), QCryptographicHash::Sha256)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
}

SingleInstanceApplication::SingleInstanceApplication()
    : socket_(nullptr) {
    (void)QObject::connect(&server_, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

bool SingleInstanceApplication::listen(const QString& serverName) {
    server_.removeServer(serverName);
    return server_.listen(serverName);
}

bool SingleInstanceApplication::attach(const QStringList &args) {
    const auto serverName = GetServerName();

    QLocalSocket socket;
    socket.connectToServer(serverName, QLocalSocket::ReadWrite);

    if (socket.waitForConnected())  {
        QByteArray buffer;
        foreach(QString item, args) {
            buffer.append(item + Q_TEXT("\n"));
        }
        socket.write(buffer);
        socket.waitForBytesWritten();
        XAMP_LOG_DEBUG("Another instance is already running.");
        return false;
    }

    if (!listen(serverName)) {
        return false;
    }
#if defined(Q_OS_WIN)
    const auto server_name = serverName.toStdWString();
    mutex_.reset(::CreateMutexW(nullptr, true, server_name.c_str()));
    if (!mutex_) {
        XAMP_LOG_DEBUG("CreateMutexW return failure! {}",
            xamp::base::GetPlatformErrorMessage(::GetLastError()));
        return false;
    }
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        return false;
    }
#endif
    return true;
}

void SingleInstanceApplication::readyRead() {
    qDebug() << "Args: " << socket_->readAll();
    socket_->close();
    socket_->deleteLater();
}

void SingleInstanceApplication::onNewConnection() {
    socket_ = server_.nextPendingConnection();
    (void)QObject::connect(socket_, SIGNAL(readyRead()), this, SLOT(readyRead()));
    XAMP_LOG_DEBUG("New instance detected");
    emit newInstanceDetected();
}
