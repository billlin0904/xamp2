#include <widget/widget_shared.h>
#include <widget/ytmusicserverprocessor.h>

#include <QDir>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>

YtMusicServerProcessor::~YtMusicServerProcessor() {
    auto pid = proc_.processId();
    KillProcessByPidAndChildren(pid);

    if (proc_.state() != QProcess::NotRunning) {
        if (!proc_.waitForFinished(2000)) {
            proc_.terminate();
            proc_.kill();
        }
    }
}

QCoro::Task<int> YtMusicServerProcessor::start() {
    if (KillProcessByNameAndChildren("ytmusic.exe")) {
        XAMP_LOG_DEBUG("Kill ytmusic.exe process");
    }

    auto workDir = qFormat(applicationPath()
        + QDir::separator()
        + "ytmusic"_str
        + QDir::separator());
    auto program = qFormat(workDir + "ytmusic.exe"_str);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("SSLKEYLOGFILE"_str, ""_str);

    auto poToken = qAppSettings.valueAsString(kAppSettingYtMusicPoToken);
    if (poToken.isEmpty()) {
        XAMP_LOG_DEBUG("Can't no find po token.");
        co_return -1;
    }

    env.insert("PO_TOKEN_VALUE"_str, poToken);
    proc_.setWorkingDirectory(workDir);
    proc_.setProgram(program);
    proc_.setProcessEnvironment(env);
    proc_.setProcessChannelMode(QProcess::MergedChannels);

    auto cProc = qCoro(proc_);

    bool started = co_await cProc.start();
    if (!started) {
        XAMP_LOG_DEBUG("Process failed to start.");
        co_return -1;
    }

    while (proc_.state() == QProcess::Running) {
        if (co_await cProc.waitForReadyRead(500)) {
            const QByteArray chunk = proc_.readAllStandardOutput();
            auto msg = QString::fromLocal8Bit(chunk);
            auto newline = msg.indexOf("\r\n"_str);
            msg = msg.remove(newline, 2);
            XAMP_LOG_DEBUG(msg.toStdString());
        }
    }

    co_await cProc.waitForFinished();
    co_return proc_.exitCode();
}
