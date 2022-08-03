#include <iostream>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/logger_impl.h>
#include <base/crashhandler.h>

#include <player/api.h>
#include <stream/soxresampler.h>
#include <stream/podcastcache.h>

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
#include "version.h"
#include "xamp.h"

static void loadSoxrSetting() {
    QMap<QString, QVariant> default_setting;

    default_setting[kResampleSampleRate] = 96000;
    default_setting[kSoxrEnableSteepFilter] = false;
    default_setting[kSoxrQuality] = static_cast<int32_t>(SoxrQuality::VHQ);
    default_setting[kSoxrPhase] = 46;
    default_setting[kSoxrStopBand] = 100;
    default_setting[kSoxrPassBand] = 96;
    default_setting[kSoxrRollOffLevel] = static_cast<int32_t>(SoxrRollOff::ROLLOFF_NONE);

    QMap<QString, QVariant> soxr_setting;
    soxr_setting[kSoxrDefaultSettingName] = default_setting;

    JsonSettings::setDefaultValue(kSoxr, QVariant::fromValue(soxr_setting));

    if (JsonSettings::getValueAsMap(kSoxr).isEmpty()) {     
        JsonSettings::setValue(kSoxr, QVariant::fromValue(soxr_setting));
        AppSettings::setValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
        AppSettings::setDefaultValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
    }
}

static void loadR8BrainSetting() {
    QMap<QString, QVariant> default_setting;

    default_setting[kResampleSampleRate] = 96000;
    JsonSettings::setDefaultValue(kR8Brain, QVariant::fromValue(default_setting));
    if (JsonSettings::getValueAsMap(kR8Brain).isEmpty()) {
        JsonSettings::setValue(kR8Brain, QVariant::fromValue(default_setting));
    }
    if (!AppSettings::contains(kAppSettingResamplerType)) {
        AppSettings::setValue(kAppSettingResamplerType, kR8Brain);
    }
}

static LogLevel parseLogLevel(const QString &str) {
    const static QMap<QString, LogLevel> logs{
    	{ Q_TEXT("info"), LogLevel::LOG_LEVEL_INFO},
        { Q_TEXT("debug"), LogLevel::LOG_LEVEL_DEBUG},
        { Q_TEXT("trace"), LogLevel::LOG_LEVEL_TRACE},
        { Q_TEXT("warn"), LogLevel::LOG_LEVEL_WARN},
        { Q_TEXT("err"), LogLevel::LOG_LEVEL_ERROR},
        { Q_TEXT("critical"), LogLevel::LOG_LEVEL_CRITICAL},
    };
    if (!logs.contains(str)) {
        return LogLevel::LOG_LEVEL_INFO;
    }
    return logs[str];
}

static void loadLogConfig() {
    QMap<QString, QVariant> log;
    QMap<QString, QVariant> min_level;
    QMap<QString, QVariant> override_map;

    QMap<QString, QVariant> well_known_log_name;

    for (const auto& logger_name : LoggerManager::GetInstance().GetWellKnownName()) {
        if (logger_name != kXampLoggerName) {
            well_known_log_name[fromStdStringView(logger_name)] = Q_TEXT("info");
        }
    }

    if (JsonSettings::getValueAsMap(kLog).isEmpty()) {
        min_level[kLogDefault] = Q_TEXT("debug");

        XAMP_SET_LOG_LEVEL(parseLogLevel(min_level[kLogDefault].toString()));

    	for (auto itr = well_known_log_name.begin()
            ; itr != well_known_log_name.end(); ++itr) {
            override_map[itr.key()] = itr.value();
            LoggerManager::GetInstance().GetLogger(itr.key().toStdString())
                ->SetLevel(parseLogLevel(itr.value().toString()));
        }

        min_level[kLogOverride] = override_map;
        log[kLogMinimumLevel] = min_level;
        JsonSettings::setValue(kLog, QVariant::fromValue(log));
        JsonSettings::setDefaultValue(kLog, QVariant::fromValue(log));
    } else {
        log = JsonSettings::getValueAsMap(kLog);
        min_level = log[kLogMinimumLevel].toMap();

        const auto default_level = min_level[kLogDefault].toString();
        XAMP_SET_LOG_LEVEL(parseLogLevel(default_level));

        override_map = min_level[kLogOverride].toMap();

        for (auto itr = well_known_log_name.begin()
            ; itr != well_known_log_name.end(); ++itr) {
            if (!override_map.contains(itr.key())) {
                override_map[itr.key()] = itr.value();
            }
        }

        for (auto itr = override_map.begin()
            ; itr != override_map.end(); ++itr) {
	        const auto& log_name = itr.key();
            auto log_level = itr.value().toString();
            LoggerManager::GetInstance().GetLogger(log_name.toStdString())
        	->SetLevel(parseLogLevel(log_level));
        }       
    }
}

static void loadSettings() {
    AppSettings::loadIniFile(Q_TEXT("xamp.ini"));
    AppSettings::setDefaultValue(kAppSettingDeviceType, Qt::EmptyString);
    AppSettings::setDefaultValue(kAppSettingDeviceId, Qt::EmptyString);
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
    AppSettings::setDefaultEnumValue(kAppSettingTheme, ThemeColor::DARK_THEME);
    AppSettings::setDefaultValue(kEnableBitPerfect, true);
    AppSettings::save();
    XAMP_LOG_DEBUG("loadSettings success.");
}

static void loadLogAndResamplerConfig() {
    JsonSettings::loadJsonFile(Q_TEXT("config.json"));
    loadLogConfig();
    loadSoxrSetting();
    loadR8BrainSetting();
    JsonSettings::save();
    XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
}

static void loadLang() {
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
}

#ifdef XAMP_OS_WIN
static std::vector<ModuleHandle> prefetchDLL() {
    const std::vector<std::string_view> dll_file_names{
        "ResourcePolicyClient.dll", // WASAPI
        "AudioSes.dll", // WASAPI
        "AUDIOKSE.dll",// WASAPI
        "Mmdevapi.dll", // WASAPI
        "DWrite.dll",
        "gdi32.dll",
        "d3d9.dll",
        "GdiPlus.dll",
        "psapi.dll",
    	"comctl32.dll",
        "thumbcache.dll",
        "setupapi.dll",
        "mimalloc-override.dll",
    #ifndef _DEBUG
        "Qt5Gui.dll",
        "Qt5Core.dll",
        "Qt5Widgets.dll",
        "Qt5Sql.dll",
        "Qt5Network.dll",
        "Qt5WinExtras.dll",
        "qwindows.dll",
        "qsqlite.dll",
        "qjpeg.dll",
	#endif
    };
    std::vector<ModuleHandle> preload_module;
    for (const auto& file_name : dll_file_names) {
        try {
            auto module = LoadModule(file_name);
            if (PrefetchModule(module)) {
                preload_module.push_back(std::move(module));
                XAMP_LOG_DEBUG("Preload {} success.", file_name);
            }
        }
        catch (std::exception const& e) {
            XAMP_LOG_DEBUG("Preload {} failure! {}.", file_name, e.what());
        }
    }
    return preload_module;
}
#endif 

static void registerMetaType() {
    qRegisterMetaTypeStreamOperators<AppEQSettings>("AppEQSettings");
    qRegisterMetaType<AppEQSettings>("AppEQSettings");
    qRegisterMetaType<Vector<Metadata>>("Vector<Metadata>");
    qRegisterMetaType<DeviceState>("DeviceState");
    qRegisterMetaType<PlayerState>("PlayerState");
    qRegisterMetaType<PlayListEntity>("PlayListEntity");
    qRegisterMetaType<Errors>("Errors");
    qRegisterMetaType<Vector<float>>("Vector<float>");
    qRegisterMetaType<Vector<PlayListEntity>>("Vector<PlayListEntity>");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<ComplexValarray>("ComplexValarray");
    qRegisterMetaType<ForwardList<Metadata>>("ForwardList<Metadata>");
    qRegisterMetaType<DriveInfo>("DriveInfo");
}

static int excute(int argc, char* argv[]) {
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QApplication::setDesktopSettingsAware(false);
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication::setApplicationName(Q_TEXT("XAMP2"));
    QApplication::setApplicationVersion(kXAMPVersion);
    QApplication::setOrganizationName(Q_TEXT("XAMP2 Project"));
    QApplication::setOrganizationDomain(Q_TEXT("XAMP2 Project"));

    QApplication app(argc, argv);

    SingleInstanceApplication single_app;
#ifndef _DEBUG
    if (!single_app.attach(QCoreApplication::arguments())) {
        return -1;
    }
#endif

    loadLang();

    XAMP_LOG_DEBUG("attach app success.");

    try {
        qDatabase.open(Q_TEXT("xamp.db"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_INFO("Init database failure. {}", e.what());
        return -1;
    }

    XAMP_LOG_DEBUG("Database init success.");

    qTheme.applyTheme();
    XAMP_LOG_DEBUG("ThemeManager applyTheme success.");

    XWindow top_win;
    Xamp win;

#ifdef XAMP_OS_WIN
    const auto prefetch_dll = prefetchDLL();
    XAMP_LOG_DEBUG("Prefetch dll success.");
#endif

    XampIniter initer;

    try {
        initer.Init();
    }
    catch (const Exception& e) {
        QMessageBox::critical(nullptr,
            Q_TEXT("Initialize failure."),
            QString::fromStdString(e.GetErrorMessage()));
        XAMP_LOG_DEBUG(e.GetStackTrace());
        return -1;
    }

    win.setXWindow(&top_win);
    top_win.setContentWidget(&win);
    //top_win.setContentWidget(nullptr);

    top_win.show();
    top_win.activateWindow();
    top_win.restoreGeometry();
    
    return app.exec();
}

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    /*if (qgetenv("MIMALLOC_VERBOSE") == "1") {
        RedirectStdOut();
    }*/
#endif
    LoggerManager::GetInstance()
        .AddDebugOutput()
#ifdef Q_OS_MAC
        .AddSink(std::make_shared<QDebugSink>())
#endif
        .AddLogFile("xamp.log")
        .Startup();

    registerMetaType();
    loadSettings();
    loadLogAndResamplerConfig();

    PodcastCache.SetTempPath(AppSettings::getValueAsString(kAppSettingPodcastCachePath).toStdWString());

    CrashHandler crash_handler;
    crash_handler.SetProcessExceptionHandlers();    
    XAMP_LOG_DEBUG("SetProcessExceptionHandlers success.");
    crash_handler.SetThreadExceptionHandlers();
    XAMP_LOG_DEBUG("SetThreadExceptionHandlers success.");

    XAMP_ON_SCOPE_EXIT(
        JsonSettings::save();
        AppSettings::save();
    );

    if (excute(argc, argv) == kRestartExistCode) {
        QProcess::startDetached(Q_STR(argv[0]), qApp->arguments());
    }
    return 0;
}

