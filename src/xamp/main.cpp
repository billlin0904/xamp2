#include <cstdio>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/stacktrace.h>
#include <base/platform_thread.h>

#include <metadata/metadatareader.h>

#include <player/audio_player.h>
#include <player/soxresampler.h>
#include <widget/qdebugsink.h>

#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/str_utilts.h>
#include <widget/jsonsettings.h>

#include <QMessageBox>

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

static int excute(int argc, char* argv[]) {
    StackTrace::RegisterAbortHandler();

    Logger::Instance()
#ifdef Q_OS_WIN
        .AddDebugOutputLogger()
#else
        .AddSink(std::make_shared<QDebugSink>())
#endif
        .AddFileLogger("xamp.log")
        .GetLogger("default");

    XAMP_SET_LOG_LEVEL(spdlog::level::debug);

    XAMP_LOG_DEBUG("Logger init success.");
    XAMP_LOG_DEBUG("RegisterAbortHandler success.");

    std::vector<ModuleHandle> preload_modules;

    const std::vector<std::string_view> preload_modules_names {
        #ifdef Q_OS_WIN
        "psapi.dll",
        "comctl32.dll",
        "WindowsCodecs.dll",
        "AudioSes.dll",
        "avrt.dll"
        #else
        "libchromaprint.dylib",
        "libbass.dylib"
        #endif
    };

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef Q_OS_MAC
    qSetMessagePattern(Q_UTF8("%{message}"));
#endif

    QApplication app(argc, argv);

    try {
        for (auto name : preload_modules_names) {
            preload_modules.emplace_back(LoadModule(name));
        }
        AudioPlayer::LoadLib();
    }
    catch (const Exception& e) {
        QMessageBox::critical(nullptr,
                              Q_UTF8("Load dll failure."),
                              QString::fromStdString(e.GetErrorMessage()));
        return -1;
    }

    XAMP_LOG_DEBUG("Preload dll success.");

#ifdef Q_OS_WIN
    xamp::base::EnablePrivilege("SeLockMemoryPrivilege", true);
    XAMP_LOG_DEBUG("EnableLockMemPrivilege success.");
#endif

    SingleInstanceApplication singleApp;
    if (!singleApp.attach(QCoreApplication::arguments())) {
        return -1;
    }

    XAMP_LOG_DEBUG("attach app success.");

    (void)PixmapCache::instance();

    XAMP_LOG_DEBUG("PixmapCache init success.");

    try {
        Database::instance().open(Q_UTF8("xamp.db"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_INFO("Initial database failure. {}", e.what());
        return -1;
    }

    XAMP_LOG_DEBUG("Database init success.");

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

    AudioDeviceFactory::PreventSleep(AppSettings::getValueAsBool(kAppSettingPreventSleep));
    XAMP_LOG_DEBUG("PreventSleep success.");

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
