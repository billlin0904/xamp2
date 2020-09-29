#include <cstdio>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/stacktrace.h>
#include <base/platform_thread.h>
#include <base/threadpool.h>
#include <base/vmmemlock.h>

#include <metadata/metadatareader.h>

#include <player/audio_player.h>
#include <player/soxresampler.h>

#include <output_device/devicefactory.h>

#include <widget/qdebugsink.h>

#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/str_utilts.h>
#include <widget/jsonsettings.h>

#include <QMessageBox>

#include "DarkStyle.h"
#include "singleinstanceapplication.h"
#include "xamp.h"

void loadOrDefaultSoxrSetting() {
    const auto keys = JsonSettings::keys();
    if (keys.count() > 0) {
        return;
    }

    QMap<QString, QVariant> defaultSetting;

    defaultSetting[kSoxrResampleSampleRate] = 44100;
    defaultSetting[kSoxrEnableSteepFilter] = false;
    defaultSetting[kSoxrQuality] = static_cast<int32_t>(SoxrQuality::HQ);
    defaultSetting[kSoxrPhase] = static_cast<int32_t>(SoxrPhaseResponse::LINEAR_PHASE);
    defaultSetting[kSoxrPassBand] = 91;

    JsonSettings::setValue(kSoxrDefaultSettingName, QVariant::fromValue(defaultSetting));
    AppSettings::setValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
    AppSettings::setDefaultValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);

    JsonSettings::save();
}

static void loadSettings() {
    AppSettings::setOrDefaultConfig();
    JsonSettings::loadJsonFile(Q_UTF8("soxr.json"));
    loadOrDefaultSoxrSetting();

    XAMP_LOG_DEBUG("setOrDefaultConfig success.");

    if (AppSettings::getValueAsString(kAppSettingLang).isEmpty()) {
        LocaleLanguage l;
        XAMP_LOG_DEBUG("Load locale lang file: {}.", l.getIsoCode().toStdString());
        AppSettings::loadLanguage(l.getIsoCode());
        AppSettings::setValue(kAppSettingLang, l.getIsoCode());
    }
    else {
        AppSettings::loadLanguage(AppSettings::getValueAsString(kAppSettingLang));
        XAMP_LOG_DEBUG("Load locale lang file: {}.",
            AppSettings::getValueAsString(kAppSettingLang).toStdString());
    }

    DeviceManager::PreventSleep(AppSettings::getValueAsBool(kAppSettingPreventSleep));
    XAMP_LOG_DEBUG("PreventSleep success.");
}

static int excute(int argc, char* argv[]) {   
    ::qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    StackTrace::RegisterAbortHandler();

    Logger::Instance()
#ifdef Q_OS_WIN
        .AddDebugOutputLogger()
#else
        .AddSink(std::make_shared<QDebugSink>())
#endif
        .AddFileLogger("xamp.log")
        .GetLogger("xamp");

    XAMP_SET_LOG_LEVEL(spdlog::level::debug);

    XAMP_LOG_DEBUG("Logger init success.");
    XAMP_LOG_DEBUG("RegisterAbortHandler success.");

    if (StackTrace::LoadSymbol()) {
        XAMP_LOG_DEBUG("Load symbol success.");
    }
    else {
        XAMP_LOG_DEBUG("Load symbol failure!");
    }

#ifdef Q_OS_WIN32    
    // https://social.msdn.microsoft.com/Forums/en-US/4890ecba-0325-4edf-99a8-bfc5d4f410e8/win10-major-issue-for-audio-processing-os-special-mode-for-small-buffer?forum=windowspro-audiodevelopment
    // Everything the SetProcessWorkingSetSize says is true. You should only lock what you need to lock.
    // And you need to lock everything you touch from the realtime thread. Because if the realtime thread
    // touches something that was paged out, you glitch.
    constexpr size_t kWorkingSetSize = 300 * 1024 * 1024;
    if (EnablePrivilege("SeLockMemoryPrivilege", true)) {
        XAMP_LOG_DEBUG("EnableLockMemPrivilege success.");

        if (ExterndProcessWorkingSetSize(kWorkingSetSize)) {
            XAMP_LOG_DEBUG("ExterndProcessWorkingSetSize {} success.", kWorkingSetSize);
        }
    }    
#endif

    QApplication app(argc, argv);

    try {
        AudioPlayer::LoadLib();
    }
    catch (const Exception& e) {
        QMessageBox::critical(nullptr,
                              Q_UTF8("Load dll failure."),
                              QString::fromStdString(e.GetErrorMessage()));
        return -1;
    }

    XAMP_LOG_DEBUG("Preload dll success.");

    SingleInstanceApplication singleApp;
    if (!singleApp.attach(QCoreApplication::arguments())) {
        return -1;
    }

    XAMP_LOG_DEBUG("attach app success.");

    XAMP_LOG_DEBUG("PixmapCache cache size:{}", PixmapCache::instance().getImageSize());

    try {
        Database::instance().open(Q_UTF8("xamp.db"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_INFO("Initial database failure. {}", e.what());
        return -1;
    }

    XAMP_LOG_DEBUG("Database init success.");

    loadSettings();    

    app.setStyle(new DarkStyle());

    (void)ThreadPool::DefaultThreadPool();
    XAMP_LOG_DEBUG("ThreadPool init success.");

    Xamp win;
    win.show();
    return app.exec();
}

#ifdef XAMP_OS_WIN
static int tryExcute(int argc, char* argv[]) {
    DWORD code = 0;
    LPEXCEPTION_POINTERS info = nullptr;

    __try {
        return excute(argc, argv);
    }
    __except (code = GetExceptionCode(), info = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
        char buffer[256];
        sprintf_s(buffer, sizeof(buffer), "Exception code: 0x%08x", code);
        ::MessageBoxA(nullptr, buffer, "Something wrong!", MB_OK);
        return -1;
    }
}
#else
static int tryExcute(int argc, char* argv[]) {
    return excute(argc, argv);
}
#endif

int main(int argc, char *argv[]) {
    XAMP_ON_SCOPE_EXIT(
        Logger::Instance().Shutdown();
        JsonSettings::save();
        AppSettings::save();
    );
    return tryExcute(argc, argv);
}
