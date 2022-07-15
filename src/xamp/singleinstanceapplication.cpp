#include <QCryptographicHash>
#include <QSharedMemory>
#include <QDir>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>
#include <widget/str_utilts.h>
#include <singleinstanceapplication.h>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

static QString getServerName() {
    //TODO: use applicationFilePath?
    //QString serverNameSource = QCoreApplication::applicationFilePath();
    QString serverNameSource;
    serverNameSource += QDir::home().dirName();
    serverNameSource += QSysInfo::machineHostName();
    return QString(Q_TEXT(QCryptographicHash::hash(serverNameSource.toUtf8(), QCryptographicHash::Sha256)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
}

SingleInstanceApplication::SingleInstanceApplication()
    : singular_(new QSharedMemory()) {
    singular_->setKey(getServerName());
}

SingleInstanceApplication::~SingleInstanceApplication() {
    delete singular_;
}

bool SingleInstanceApplication::attach(const QStringList &args) const {
    if (singular_->attach(QSharedMemory::ReadOnly)) {
        singular_->detach();
        return false;
    }

    if (singular_->create(1))
        return true;

    return false;
}

