#include "applicationupdater.h"
#include <version.h>
#include <widget/widget_shared.h>
#include <widget/xmessagebox.h>
#include <QApplication>
#include <widget/appsettings.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
#define XAMP_ENABLE_UPDATER 1

#endif
namespace {
    const auto kUpdateDefinitionsUrl = "https://raw.githubusercontent.com/billlin0904/xamp2/master/src/versions/updates.json"_str;
    constexpr auto kUpdateDefinitionsUrlEnvironmentName = "XAMP_UPDATE_DEFINITIONS_URL";
    QString updateDefinitionsUrl() {
        const auto override_url = qEnvironmentVariable(kUpdateDefinitionsUrlEnvironmentName).trimmed();
        return override_url.isEmpty() ? kUpdateDefinitionsUrl : override_url;
    }

    QString updatePlatformKey() {
#ifdef Q_OS_WIN
        return "windows"_str;
#elif defined(Q_OS_LINUX)
        return "linux"_str;
#else
        return {};
#endif
    }

    QString fileSha256(const QString& filepath) {
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);
        while (!file.atEnd()) {
            const auto bytes = file.read(1024 * 1024);
            if (bytes.isEmpty() && file.error() != QFileDevice::NoError) return {};
            hash.addData(bytes);
        }
        return QString::fromLatin1(hash.result().toHex());
    }

    bool equalsSha256(const QString& actual, const QString& expected) {
        return !actual.isEmpty()
            && !expected.isEmpty()
            && actual.compare(expected.trimmed(), Qt::CaseInsensitive) == 0;
    }

#ifdef Q_OS_LINUX
    QString appImagePath() {
        return qEnvironmentVariable("APPIMAGE").trimmed();
    }

    bool writeLinuxAppImageUpdateScript(const QString& script_path) {
        QFile source(":/xamp/appimage-update.sh"_str);
        if (!source.open(QIODevice::ReadOnly)) {
            return false;
        }

        QFile script(script_path);
        if (!script.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return false;
        }

        if (script.write(source.readAll()) < 0) {
            return false;
        }

        script.close();
        return QFile::setPermissions(script_path,
            QFileDevice::ReadOwner
            | QFileDevice::WriteOwner
            | QFileDevice::ExeOwner
            | QFileDevice::ReadGroup
            | QFileDevice::ExeGroup
            | QFileDevice::ReadOther
            | QFileDevice::ExeOther);
    }
#endif

}
void ApplicationUpdater::installDownloadedUpdate(const QString& url, const QString& filepath) {
#ifdef XAMP_ENABLE_UPDATER
    const auto update_url = updateDefinitionsUrl();
    if (url != update_url || filepath.isEmpty()) {
        XAMP_LOG_WARN("Ignore downloaded update. signal_url:{} expected_url:{} filepath:{}",
            url.toStdString(),
            update_url.toStdString(),
            filepath.toStdString());
        return;
    }

#ifdef Q_OS_WIN
    const auto installer_log = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
        .filePath("xamp2-update-install.log"_str);
    const QStringList args{
        "/SILENT"_str,
        "/SUPPRESSMSGBOXES"_str,
        "/NORESTART"_str,
        "/CLOSEAPPLICATIONS"_str,
        "/UPDATE"_str,
        "/DIR="_str + QCoreApplication::applicationDirPath(),
        "/LOG="_str + installer_log,
    };

    qint64 installer_pid = 0;
    qAppSettings.save();
    if (QProcess::startDetached(filepath, args, QFileInfo(filepath).absolutePath(), &installer_pid)) {
        XAMP_LOG_INFO("Started update installer. filepath:{} pid:{} log:{}",
            filepath.toStdString(),
            installer_pid,
            installer_log.toStdString());
        emit installing();
        qApp->quit();
    }
    else {
        XAMP_LOG_ERROR("Failed to start update installer. filepath:{}",
            filepath.toStdString());
        XMessageBox::showError(QCoreApplication::translate("Xamp", "Failed to start the update installer."));
    }
#elif defined(Q_OS_LINUX)
    const auto target_appimage = appImagePath();
    if (target_appimage.isEmpty()) {
        XAMP_LOG_ERROR("Cannot install AppImage update because APPIMAGE is not set.");
        XMessageBox::showError(QCoreApplication::translate("Xamp", "Automatic Linux updates are only supported when running from an AppImage."));
        return;
    }

    const auto temp_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const auto script_path = QDir(temp_dir).filePath("xamp2-appimage-update.sh"_str);
    const auto update_log = QDir(temp_dir).filePath("xamp2-appimage-update.log"_str);

    if (!QFile::setPermissions(filepath,
        QFileDevice::ReadOwner
        | QFileDevice::WriteOwner
        | QFileDevice::ExeOwner
        | QFileDevice::ReadGroup
        | QFileDevice::ExeGroup
        | QFileDevice::ReadOther
        | QFileDevice::ExeOther)) {
        XAMP_LOG_ERROR("Failed to mark downloaded AppImage executable. filepath:{}",
            filepath.toStdString());
        XMessageBox::showError(QCoreApplication::translate("Xamp", "Failed to prepare the downloaded AppImage."));
        return;
    }

    if (!writeLinuxAppImageUpdateScript(script_path)) {
        XAMP_LOG_ERROR("Failed to write AppImage update helper. script:{}",
            script_path.toStdString());
        XMessageBox::showError(QCoreApplication::translate("Xamp", "Failed to prepare the AppImage update helper."));
        return;
    }

    const QStringList args{
        script_path,
        QString::number(QCoreApplication::applicationPid()),
        filepath,
        target_appimage,
        update_log,
    };

    qint64 updater_pid = 0;
    if (QProcess::startDetached("/bin/sh"_str, args, temp_dir, &updater_pid)) {
        XAMP_LOG_INFO("Started AppImage update helper. script:{} pid:{} target:{} log:{}",
            script_path.toStdString(),
            updater_pid,
            target_appimage.toStdString(),
            update_log.toStdString());
        emit installing();
        qApp->quit();
    }
    else {
        XAMP_LOG_ERROR("Failed to start AppImage update helper. script:{}",
            script_path.toStdString());
        XMessageBox::showError(QCoreApplication::translate("Xamp", "Failed to start the AppImage update helper."));
    }
#endif
#else
    (void)url;
    (void)filepath;
#endif
}


#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QRegularExpression>
#include <QDateTime>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTextDocument>
#include <QFrame>
#include <widget/appsettings.h>

ApplicationUpdater::ApplicationUpdater(QObject* parent) : QObject(parent), network_(this) {}

void ApplicationUpdater::fail(const QString& message) {
    error_ = message;
    state_ = State::Failed;
    emit changed();
}

void ApplicationUpdater::start() {
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        if (!qAppSettings.contains("updateAutoCheck"_str) || qAppSettings.valueAsBool("updateAutoCheck"_str)) check();
    });
    timer->start(std::chrono::hours(24));
    QTimer::singleShot(5000, this, [this] { if (!qAppSettings.contains("updateAutoCheck"_str) || qAppSettings.valueAsBool("updateAutoCheck"_str)) check(); });
}

void ApplicationUpdater::checkNow() { check(); }

void ApplicationUpdater::check() {
    if (reply_ || state_ == State::Ready || state_ == State::Verifying) return;
    state_ = State::Checking;
    error_.clear();
    emit changed();
    QNetworkRequest request{QUrl(updateDefinitionsUrl())};
    request.setTransferTimeout(30000);
    auto* reply = network_.get(request);
    reply_ = reply;
    connect(reply, &QNetworkReply::readyRead, this, [reply] { if (reply->bytesAvailable() > 1024 * 1024) reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply_ = nullptr;
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) { fail(tr("Unable to check for updates. Please try again.")); return; }
        const auto document = QJsonDocument::fromJson(reply->readAll());
        const auto entry = document.object().value("updates"_str).toObject().value(updatePlatformKey()).toObject();
        version_ = entry.value("latest-version"_str).toString();
        if (QVersionNumber::fromString(version_).isNull()) { fail(tr("Invalid update information.")); return; }
        qAppSettings.setValue("updateLastCheck"_str, QDateTime::currentDateTime().toString(Qt::ISODate));
        qAppSettings.save();
        if (QVersionNumber::fromString(version_) <= kApplicationVersionValue) { state_ = State::Idle; emit changed(); return; }
        download_url_ = entry.value("download-url"_str).toString();
        sha256_ = entry.value("sha256"_str).toString().trimmed();
        changelog_ = entry.value("changelog"_str).toString();
        if (QUrl(download_url_).scheme() != "https"_str || !QRegularExpression("^[a-fA-F0-9]{64}$"_str).match(sha256_).hasMatch()) {
            fail(tr("Invalid update information.")); return;
        }
        state_ = State::Available;
        emit changed();
        if (qAppSettings.valueAsBool("updateAutoDownload"_str)) download();
    });
}

void ApplicationUpdater::download() {
    if (reply_ || state_ != State::Available) return;
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/updates"_str;
    if (!QDir().mkpath(dir)) { fail(tr("Unable to write the update package.")); return; }
#ifdef Q_OS_WIN
    package_.setFileName(dir + "/xamp-update.exe"_str);
#else
    package_.setFileName(dir + "/xamp-update.AppImage"_str);
#endif
    if (!package_.open(QIODevice::WriteOnly | QIODevice::Truncate)) { fail(tr("Unable to write the update package.")); return; }
    error_.clear();
    cancelled_ = false;
    hash_.reset(); received_ = total_ = 0;
    state_ = State::Downloading;
    emit changed();
    QNetworkRequest request{QUrl(download_url_)};
    request.setTransferTimeout(60000);
    auto* reply = network_.get(request);
    reply_ = reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        const auto bytes = reply->readAll();
        if (package_.write(bytes) != bytes.size()) { error_ = tr("Unable to write the update package."); reply->abort(); return; }
        hash_.addData(bytes);
    });
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        received_ = received; total_ = total; emit changed();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply_ = nullptr;
        reply->deleteLater();
        const bool flushed = package_.flush();
        package_.close();
        if (cancelled_) { package_.remove(); state_ = State::Available; emit changed(); return; }
        if (reply->error() != QNetworkReply::NoError || !flushed) {
            package_.remove();
            if (reply->error() == QNetworkReply::OperationCanceledError && error_.isEmpty()) { state_ = State::Available; emit changed(); return; }
            fail(error_.isEmpty() ? tr("Download failed. Please try again.") : error_); return;
        }
        if (!equalsSha256(QString::fromLatin1(hash_.result().toHex()), sha256_)) {
            package_.remove(); fail(tr("The update package failed SHA256 verification.")); return;
        }
        state_ = State::Ready;
        emit changed();
    });
}

QWidget* ApplicationUpdater::createSettingsPage(QWidget* parent) {
    auto* page = new QFrame(parent);
    page->setProperty("settingRow", true);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);
    auto* heading = new QLabel(page);
    heading->setProperty("settingTitle", true);
    auto* status = new QLabel(page);
    status->setWordWrap(true);
    status->setProperty("settingHint", true);
    auto* notes = new QLabel(page);
    notes->setWordWrap(true);
    notes->setTextFormat(Qt::PlainText);
    auto* automatic = new QCheckBox(page);
    automatic->setChecked(!qAppSettings.contains("updateAutoCheck"_str) || qAppSettings.valueAsBool("updateAutoCheck"_str));
    auto* autoDownload = new QCheckBox(page);
    autoDownload->setChecked(qAppSettings.valueAsBool("updateAutoDownload"_str));
    auto* progress = new QProgressBar(page);
    auto* action = new QPushButton(page);
    auto* cancel = new QPushButton(page);
    action->setMaximumWidth(240);
    cancel->setMaximumWidth(240);
    for (QWidget* widget : QList<QWidget*>{heading, status, notes, automatic, autoDownload, progress, action, cancel}) layout->addWidget(widget);
    const auto render = [=, this] {
        heading->setText(tr("Software updates") + QStringLiteral(" \u00b7 ") + kApplicationVersion);
        automatic->setText(tr("Automatically check for updates"));
        autoDownload->setText(tr("Automatically download updates"));
        cancel->setText(tr("Cancel download"));
        cancel->setVisible(state_ == State::Downloading);
        progress->setVisible(state_ == State::Downloading);
        progress->setRange(0, total_ > 0 ? 100 : 0);
        progress->setValue(total_ > 0 ? int(100.0 * received_ / total_) : 0);
        action->setEnabled(state_ != State::Checking && state_ != State::Downloading && state_ != State::Verifying);
        action->setText(state_ == State::Ready ? tr("Restart and update") : state_ == State::Available ? tr("Download update") : tr("Check for updates"));
        QString message;
        switch (state_) {
        case State::Verifying: message = tr("Verifying update…"); break;
        case State::Checking: message = tr("Checking for updates…"); break;
        case State::Available: message = tr("New version available: %1").arg(version_); break;
        case State::Downloading: message = tr("Downloading update…"); break;
        case State::Ready: message = tr("Update ready. Restart when convenient; playback will not be interrupted automatically."); break;
        case State::Failed: message = error_; break;
        default: message = version_.isEmpty() ? tr("Check for updates") : tr("You are up to date.");
            message += "\n"_str + tr("Last checked: %1").arg(qAppSettings.valueAsString("updateLastCheck"_str)); break;
        }
        status->setText(message);
        QTextDocument description;
        description.setHtml(changelog_);
        notes->setText(description.toPlainText());
        notes->setVisible(state_ == State::Available || state_ == State::Ready);
    };
    connect(this, &ApplicationUpdater::changed, page, render);
    connect(automatic, &QCheckBox::toggled, this, [](bool value) { qAppSettings.setValue("updateAutoCheck"_str, value); qAppSettings.save(); });
    connect(autoDownload, &QCheckBox::toggled, this, [](bool value) { qAppSettings.setValue("updateAutoDownload"_str, value); qAppSettings.save(); });
    connect(cancel, &QPushButton::clicked, this, &ApplicationUpdater::cancelDownload);
    connect(action, &QPushButton::clicked, this, [this] {
        if (state_ == State::Ready) verifyAndInstall();
        else if (state_ == State::Available) download();
        else check();
    });
    layout->addStretch();
    auto* refresh = new QTimer(page);
    connect(refresh, &QTimer::timeout, page, render);
    refresh->start(1000);
    render();
    return page;
}

void ApplicationUpdater::cancelDownload() {
    if (state_ != State::Downloading || !reply_) return;
    cancelled_ = true;
    reply_->abort();
}

void ApplicationUpdater::verifyAndInstall() {
    if (state_ != State::Ready) return;
    state_ = State::Verifying;
    emit changed();
    const auto path = package_.fileName();
    verification_ = std::async(std::launch::async, [path] { return fileSha256(path); });
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, path] {
        if (verification_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
        timer->stop(); timer->deleteLater();
        if (!equalsSha256(verification_.get(), sha256_)) {
            QFile::remove(path);
            fail(tr("The update package failed SHA256 verification."));
            return;
        }
        state_ = State::Ready;
        installDownloadedUpdate(updateDefinitionsUrl(), path);
        emit changed();
    });
    timer->start(50);
}
