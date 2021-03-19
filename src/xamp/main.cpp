#include <iostream>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/stacktrace.h>
#include <base/platform_thread.h>
#include <base/vmmemlock.h>
#include <base/str_utilts.h>

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

#include "DarkStyle.h"
#include "singleinstanceapplication.h"
#include "xamp.h"

static void loadOrDefaultSoxrSetting() {
    const auto keys = JsonSettings::keys();
    if (keys.count() > 0) {
        return;
    }

    QMap<QString, QVariant> default_setting;

    default_setting[kSoxrResampleSampleRate] = 96000;
    default_setting[kSoxrEnableSteepFilter] = false;
    default_setting[kSoxrQuality] = static_cast<int32_t>(SoxrQuality::VHQ);
    default_setting[kSoxrPhase] = 45;
    default_setting[kSoxrPassBand] = 95;

    JsonSettings::setValue(kSoxrDefaultSettingName, QVariant::fromValue(default_setting));
    AppSettings::setValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
    AppSettings::setDefaultValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);

    JsonSettings::save();
    AppSettings::save();
}

static void loadSettings() {
    AppSettings::setOrDefaultConfig();
    JsonSettings::loadJsonFile(Q_UTF8("soxr.json"));
	
    loadOrDefaultSoxrSetting();

    XAMP_LOG_DEBUG("setOrDefaultConfig success.");

    if (AppSettings::getValueAsString(kAppSettingLang).isEmpty()) {
	    const LocaleLanguage lang;
        XAMP_LOG_DEBUG("Load locale lang file: {}.", lang.getIsoCode().toStdString());
        AppSettings::loadLanguage(lang.getIsoCode());
        AppSettings::setValue(kAppSettingLang, lang.getIsoCode());
    }
    else {
        AppSettings::loadLanguage(AppSettings::getValueAsString(kAppSettingLang));
        XAMP_LOG_DEBUG("Load locale lang file: {}.",
            AppSettings::getValueAsString(kAppSettingLang).toStdString());
    }

	if (!AppSettings::contains(kEnableCompressor)) {
        AppSettings::setDefaultValue(kEnableCompressor, true);
        AppSettings::setDefaultValue(kCompressorGain, -1);
        AppSettings::setDefaultValue(kCompressorThreshold, 0);
        AppSettings::setDefaultValue(kCompressorRatio, 1);
        AppSettings::setDefaultValue(kCompressorAttack, 20);
        AppSettings::setDefaultValue(kCompressorRelease, 200);
        AppSettings::save();
	}
}

static int excute(int argc, char* argv[]) {
    ::qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    StackTrace::RegisterAbortHandler();

    Logger::GetInstance()
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

    QApplication app(argc, argv);

    try {
        AudioPlayer::Initial();
    }
    catch (const Exception& e) {
        QMessageBox::critical(nullptr,
                              Q_UTF8("Load dll failure."),
                              QString::fromStdString(e.GetErrorMessage()));
        return -1;
    }

#ifdef XAMP_OS_WIN
    std::vector<std::string_view> preload_dll_file_name{
    	"AudioSes.dll",
        "ResourcePolicyClient.dll",
        "AUDIOKSE.dll",
    	"comctl32.dll",
        "WindowsCodecs.dll",
        "thumbcache.dll",
    	"psapi.dll"
    };
    std::vector<ModuleHandle> preload_module;
	for (auto file_name : preload_dll_file_name) {
        try {
            preload_module.push_back(LoadModule(file_name));
        } catch (...) {	        
        }
	}
    XAMP_LOG_DEBUG("Preload dll success.");
#endif    

    SingleInstanceApplication singleApp;
#ifndef _DEBUG
    if (!singleApp.attach(QCoreApplication::arguments())) {
        return -1;
    }
#endif
    XAMP_LOG_DEBUG("attach app success.");

    (void)Singleton<PixmapCache>::GetInstance();
    XAMP_LOG_DEBUG("PixmapCache init success.");

    XAMP_LOG_DEBUG("PixmapCache cache size:{}", 
        String::FormatBytes(Singleton<PixmapCache>::GetInstance().getImageSize()));

    try {
        Singleton<Database>::GetInstance().open(Q_UTF8("xamp.db"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_INFO("Init database failure. {}", e.what());
        return -1;
    }

    XAMP_LOG_DEBUG("Database init success.");

    loadSettings();    

    app.setStyle(new DarkStyle());    

    Xamp win;
    win.show();
    win.activateWindow();
    return app.exec();
}

#ifdef XAMP_OS_WIN
static int tryExcute(int argc, char* argv[]) {
    DWORD code = 0;
    LPEXCEPTION_POINTERS info = nullptr;

    __try {
        return excute(argc, argv);
    }
    __except (code = ::GetExceptionCode(), info = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
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
        Logger::GetInstance().Shutdown();
        JsonSettings::save();
        AppSettings::save();
    );
    return tryExcute(argc, argv);
}
