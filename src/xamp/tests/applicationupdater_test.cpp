#include "applicationupdater.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <widget/appsettings.h>
#include <iostream>
#include <stdexcept>
class ApplicationUpdaterTest {
    static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
    static void wait(ApplicationUpdater& updater) {
        QElapsedTimer time; time.start();
        while (updater.reply_ && time.elapsed() < 5000) QCoreApplication::processEvents();
        require(!updater.reply_, "request timed out");
    }
public:
    static void run() {
        QTemporaryDir dir;
        qAppSettings.loadIniFile(dir.filePath(QStringLiteral("settings.ini")));
        QStandardPaths::setTestModeEnabled(true);
        ApplicationUpdater updater;
        QFile manifest(dir.filePath(QStringLiteral("updates.json")));
        auto writeManifest = [&](const QByteArray& bytes) {
            require(manifest.open(QIODevice::WriteOnly | QIODevice::Truncate), "manifest open"); auto data = bytes;
#ifdef Q_OS_LINUX
            data.replace("windows", "linux");
#endif
            manifest.write(data); manifest.close();
            qputenv("XAMP_UPDATE_DEFINITIONS_URL", QUrl::fromLocalFile(manifest.fileName()).toEncoded());
        };
        writeManifest("{}"); updater.checkNow(); wait(updater);
        require(updater.state_ == ApplicationUpdater::State::Failed, "invalid manifest must fail");
        writeManifest(R"({"updates":{"windows":{"latest-version":"0.0.0"}}})");
        updater.checkNow(); wait(updater);
        require(updater.state_ == ApplicationUpdater::State::Idle, "old version must not update");
        writeManifest(R"({"updates":{"windows":{"latest-version":"999.0.0","download-url":"http://unsafe.invalid/a.exe","sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}}})");
        updater.checkNow(); wait(updater);
        require(updater.state_ == ApplicationUpdater::State::Failed, "HTTP download must be rejected");
        writeManifest(R"({"updates":{"windows":{"latest-version":"999.0.0","download-url":"https://example.invalid/a.exe","sha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}}})");
        updater.checkNow(); auto* active = updater.reply_.data(); updater.checkNow();
        require(updater.reply_ == active, "duplicate checks must be suppressed");
        wait(updater); require(updater.state_ == ApplicationUpdater::State::Available, "new release available");
        QFile source(dir.filePath(QStringLiteral("package.bin")));
        require(source.open(QIODevice::WriteOnly), "package open"); source.write("test package"); source.close();
        updater.download_url_ = QUrl::fromLocalFile(source.fileName()).toString();
        updater.download(); wait(updater);
        require(updater.state_ == ApplicationUpdater::State::Failed, "wrong hash must fail");
        require(!QFile::exists(updater.package_.fileName()), "corrupt package must be deleted");
        updater.state_ = ApplicationUpdater::State::Available;
        updater.sha256_ = QString::fromLatin1(QCryptographicHash::hash("test package", QCryptographicHash::Sha256).toHex());
        updater.download(); wait(updater);
        require(updater.state_ == ApplicationUpdater::State::Ready, "verified package must wait for installation");
        updater.checkNow(); require(!updater.reply_, "ready package must not be replaced by a check");
        updater.package_.remove();
        updater.state_ = ApplicationUpdater::State::Available;
        updater.download(); updater.cancelDownload(); wait(updater);
        require(updater.state_ == ApplicationUpdater::State::Available, "cancel must allow retry");
        require(!QFile::exists(updater.package_.fileName()), "partial package must be removed");
    }
};
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("xamp-updater-tests"));
    try { ApplicationUpdaterTest::run(); std::cout << "Updater checks passed\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
