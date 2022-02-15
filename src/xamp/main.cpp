#include <iostream>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/platform.h>
#include <base/str_utilts.h>
#include <base/simd.h>

#include <player/api.h>
#include <stream/soxresampler.h>

#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/str_utilts.h>
#include <widget/jsonsettings.h>
#include <widget/ui_utilts.h>
#include <widget/xwindow.h>

#include <QMessageBox>
#include <QProcess>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#endif

#include "thememanager.h"
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
    default_setting[kSoxrPhase] = 46;
    default_setting[kSoxrStopBand] = 100;
    default_setting[kSoxrPassBand] = 96;
    default_setting[kAppSettingSoxrSettingRollOff] = static_cast<int32_t>(SoxrRollOff::ROLLOFF_NONE);

    JsonSettings::setValue(kSoxrDefaultSettingName, QVariant::fromValue(default_setting));
    AppSettings::setValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
    AppSettings::setDefaultValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);

    JsonSettings::save();
    AppSettings::save();
}

static void loadSettings() {
    AppSettings::loadIniFile(Q_UTF8("xamp.ini"));
    AppSettings::setDefaultValue(kAppSettingDeviceType, Qt::EmptyString);
    AppSettings::setDefaultValue(kAppSettingDeviceId, Qt::EmptyString);
    AppSettings::setDefaultValue(kAppSettingWidth, 600);
    AppSettings::setDefaultValue(kAppSettingHeight, 500);
    AppSettings::setDefaultValue(kAppSettingVolume, 50);
    AppSettings::setDefaultEnumValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
    AppSettings::setDefaultValue(kAppSettingEnableBlur, false);
    AppSettings::setDefaultValue(kAppSettingUseFramelessWindow, true);
    AppSettings::setDefaultValue(kLyricsFontSize, 12);
    AppSettings::setDefaultValue(kAppSettingMinimizeToTrayAsk, true);
    AppSettings::setDefaultValue(kAppSettingMinimizeToTray, false);
    AppSettings::setDefaultValue(kAppSettingDiscordNotify, false);
    AppSettings::setDefaultValue(kFlacEncodingLevel, 8);
    AppSettings::setDefaultValue(kAppSettingAlbumImageCacheSize, 32);
    AppSettings::setDefaultValue(kAppSettingShowLeftList, true);
    AppSettings::setDefaultValue(kAppSettingReplayGainTargetGain, kReferenceLoudness);
    AppSettings::setDefaultEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
    AppSettings::setDefaultEnumValue(kAppSettingReplayGainScanMode, ReplayGainScanMode::RG_SCAN_MODE_FAST);
    AppSettings::setDefaultValue(kAppSettingEnableReplayGain, true);
    AppSettings::setDefaultEnumValue(kAppSettingTheme, ThemeColor::LIGHT_THEME);
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

    AppSettings::save();
}

static std::vector<ModuleHandle> preloadDll() {
#ifdef XAMP_OS_WIN
    std::vector<std::string_view> preload_dll_file_name{
        "mimalloc-override.dll",
        "AudioSes.dll",
        "psapi.dll",
        "AUDIOKSE.dll",
        "comctl32.dll",
        "WindowsCodecs.dll",
        "thumbcache.dll",
        "setupapi.dll",
        "d3d9.dll",
        "opengl32.dll",
        "glu32.dll",
        "DWrite.dll",
        "wininet.dll",
    };
    std::vector<ModuleHandle> preload_module;
    for (const auto file_name : preload_dll_file_name) {
        try {
            preload_module.push_back(LoadModule(file_name));
        }
        catch (std::exception const& e) {
            XAMP_LOG_DEBUG("Preload {} failure! {}.", file_name, e.what());
        }
    }
    XAMP_LOG_DEBUG("Preload dll success.");
    return preload_module;
#else
	return std::vector<ModuleHandle>();
#endif 
}

static void setLogLevel(spdlog::level::level_enum level = spdlog::level::debug) {
    Logger::GetInstance().GetLogger(kWASAPIThreadPoolLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kPlaybackThreadPoolLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kExclusiveWasapiDeviceLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kSharedWasapiDeviceLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kAsioDeviceLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kVirtualMemoryLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kResamplerLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kCompressorLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kCoreAudioLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger("PixmapCache")->set_level(level);
    Logger::GetInstance().GetLogger(kDspManagerLoggerName)->set_level(level);
    Logger::GetInstance().GetLogger(kAudioPlayerLoggerName)->set_level(level);
}

static int excute(int argc, char* argv[]) {
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    XAMP_SET_LOG_LEVEL(spdlog::level::debug);
    XAMP_LOG_DEBUG("Logger init success.");

    const auto preload_module = preloadDll();

    QApplication app(argc, argv);

    Xamp::registerMetaType();

    try {
	    XStartup();
    }
    catch (const Exception& e) {
        QMessageBox::critical(nullptr,
                              Q_UTF8("Load dll failure."),
                              QString::fromStdString(e.GetErrorMessage()));
        return -1;
    }   

    SingleInstanceApplication single_app;
    QApplication::setApplicationName(Q_UTF8("XAMP2"));
    QApplication::setApplicationVersion(Q_UTF8("0.0.1"));
    QApplication::setOrganizationName(Q_UTF8("XAMP2 Project"));
    QApplication::setOrganizationDomain(Q_UTF8("XAMP2 Project"));

#ifndef _DEBUG
    if (!single_app.attach(QCoreApplication::arguments())) {
        return -1;
    }
#endif

    XAMP_LOG_DEBUG("attach app success.");

    loadSettings();

    try {
        Singleton<Database>::GetInstance().open(Q_UTF8("xamp.db"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_INFO("Init database failure. {}", e.what());
        return -1;
    }

    XAMP_LOG_DEBUG("Database init success.");
    setLogLevel(spdlog::level::info);
    /*Logger::GetInstance().GetLogger(kAudioPlayerLoggerName)->set_level(spdlog::level::debug);
    Logger::GetInstance().GetLogger(kExclusiveWasapiDeviceLoggerName)->set_level(spdlog::level::debug);*/

    foreach(const QString & path, app.libraryPaths()) {
        XAMP_LOG_DEBUG("Library path : {}.", path.toStdString());
    }

    QApplication::setFont(Singleton<ThemeManager>::GetInstance().defaultFont());

    XWindow top_win;
    Xamp win;
    win.setXWindow(&top_win);
    top_win.setContentWidget(&win);
    //top_win.setContentWidget(nullptr);

    top_win.show();
    top_win.activateWindow();
    top_win.resize(1250, 960);
    centerDesktop(&top_win);
    return app.exec();
}

int main(int argc, char *argv[]) {
    Logger::GetInstance()
#ifdef Q_OS_WIN
        .AddDebugOutputLogger()
#else
        .AddSink(std::make_shared<QDebugSink>())
#endif
        .AddFileLogger("xamp.log")
        .GetLogger(kDefaultLoggerName);

    XAMP_ON_SCOPE_EXIT(
        Logger::GetInstance().Shutdown();
        JsonSettings::save();
        AppSettings::save();
    );

    if (excute(argc, argv) == -2) {
        QProcess::startDetached(Q_STR(argv[0]), qApp->arguments());
    }
    return 0;
}

