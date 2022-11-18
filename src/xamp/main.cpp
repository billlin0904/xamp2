#include <iostream>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/logger_impl.h>
#include <base/crashhandler.h>
#include <base/int24.h>
#include <base/encodingprofile.h>

#include <player/api.h>
#include <stream/soxresampler.h>
#include <stream/podcastcache.h>
#include <stream/pcm2dsdsamplewriter.h>

#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/str_utilts.h>
#include <widget/jsonsettings.h>
#include <widget/ui_utilts.h>
#include <widget/xwindow.h>

#include <QOperatingSystemVersion>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#endif

#include "thememanager.h"
#include "singleinstanceapplication.h"
#include "version.h"
#include "xamp.h"

#ifdef Q_OS_WIN32
static ConstLatin1String visualStudioVersion() {
    if constexpr (_MSC_VER >= 1930) {
        return "2022";
    }
    return "2019";
}
#endif

static void loadSoxrSetting() {
    XAMP_LOG_DEBUG("loadSoxrSetting.");

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
    XAMP_LOG_DEBUG("loadR8BrainSetting.");

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

static void loadPcm2DsdSetting() {
    XAMP_LOG_DEBUG("loadPcm2DsdSetting.");

    if (JsonSettings::getValueAsMap(kR8Brain).isEmpty()) {
        QMap<QString, QVariant> default_setting;
        default_setting[kPCM2DSDDsdTimes] = static_cast<uint32_t>(DsdTimes::DSD_TIME_6X);
        JsonSettings::setDefaultValue(kPCM2DSD, QVariant::fromValue(default_setting));
        AppSettings::setValue(kEnablePcm2Dsd, false);
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
    XAMP_LOG_DEBUG("loadLogConfig.");

    QMap<QString, QVariant> log;
    QMap<QString, QVariant> min_level;
    QMap<QString, QVariant> override_map;

    QMap<QString, QVariant> well_known_log_name;

    for (const auto& logger_name : XAMP_DEFAULT_LOG().GetWellKnownName()) {
        if (logger_name != kXampLoggerName) {
            well_known_log_name[fromStdStringView(logger_name)] = Q_TEXT("info");
        }
    }

    if (JsonSettings::getValueAsMap(kLog).isEmpty()) {
        min_level[kLogDefault] = Q_TEXT("debug");

        XAMP_DEFAULT_LOG().SetLevel(parseLogLevel(min_level[kLogDefault].toString()));

    	for (auto itr = well_known_log_name.begin()
            ; itr != well_known_log_name.end(); ++itr) {
            override_map[itr.key()] = itr.value();
            XAMP_DEFAULT_LOG().GetLogger(itr.key().toStdString())
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
        XAMP_DEFAULT_LOG().SetLevel(parseLogLevel(default_level));

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
            XAMP_DEFAULT_LOG().GetLogger(log_name.toStdString())
        	->SetLevel(parseLogLevel(log_level));
        }       
    }
}

static void loadSettings() {
    XAMP_LOG_DEBUG("loadSettings.");

    AppSettings::loadIniFile(Q_TEXT("xamp.ini"));

	AppSettings::setDefaultEnumValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
    AppSettings::setDefaultEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
    AppSettings::setDefaultEnumValue(kAppSettingReplayGainScanMode, ReplayGainScanMode::RG_SCAN_MODE_FULL);
    AppSettings::setDefaultEnumValue(kAppSettingTheme, ThemeColor::DARK_THEME);

    AppSettings::setDefaultValue(kAppSettingDeviceType, Qt::EmptyString);
    AppSettings::setDefaultValue(kAppSettingDeviceId, Qt::EmptyString);
    AppSettings::setDefaultValue(kAppSettingVolume, 50);    
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
    AppSettings::setDefaultValue(kAppSettingEnableReplayGain, true);
    AppSettings::setDefaultValue(kEnableBitPerfect, true);
    AppSettings::setDefaultValue(kAppSettingWindowState, false);
    AppSettings::setDefaultValue(kAppSettingScreenNumber, 1);

    AppSettings::save();
    XAMP_LOG_DEBUG("loadSettings success.");
}

static void loadLogAndResamplerConfig() {
    XAMP_LOG_DEBUG("loadLogAndResamplerConfig.");

    JsonSettings::loadJsonFile(Q_TEXT("config.json"));
    loadLogConfig();
    loadSoxrSetting();
    loadR8BrainSetting();
    loadPcm2DsdSetting();
    JsonSettings::save();
    XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
}

static void loadLang() {
    XAMP_LOG_DEBUG("loadLang.");

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
        "mimalloc-override.dll",
        "psapi.dll",
    #ifndef _DEBUG
        "Qt5Gui.dll",
        "Qt5Core.dll",
        "Qt5Widgets.dll",
        "Qt5Sql.dll",
        "Qt5Network.dll",
        "Qt5WinExtras.dll"
	#endif
    };
    std::vector<ModuleHandle> preload_module;
    for (const auto& file_name : dll_file_names) {
        try {
            auto module = LoadModule(file_name);
            if (PrefetchModule(module)) {
                preload_module.push_back(std::move(module));
                XAMP_LOG_DEBUG("\tPreload => {} success.", file_name);
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
    XAMP_LOG_DEBUG("registerMetaType.");

    qRegisterMetaTypeStreamOperators<AppEQSettings>("AppEQSettings");
    qRegisterMetaType<AppEQSettings>("AppEQSettings");
    qRegisterMetaType<Vector<TrackInfo>>("Vector<TrackInfo>");
    qRegisterMetaType<DeviceState>("DeviceState");
    qRegisterMetaType<PlayerState>("PlayerState");
    qRegisterMetaType<PlayListEntity>("PlayListEntity");
    qRegisterMetaType<Errors>("Errors");
    qRegisterMetaType<Vector<float>>("Vector<float>");
    qRegisterMetaType<Vector<PlayListEntity>>("Vector<PlayListEntity>");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<ComplexValarray>("ComplexValarray");
    qRegisterMetaType<ForwardList<TrackInfo>>("ForwardList<TrackInfo>");
    qRegisterMetaType<DriveInfo>("DriveInfo");
    qRegisterMetaType<EncodingProfile>("EncodingProfile");
}

#ifdef _DEBUG
static void logMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString str;
    QTextStream stream(&str);
    stream.setCodec("UTF-8");

    stream << context.file << ":" << context.line << ":"
        << context.function << ": " << msg;

    auto logger = LoggerManager::GetInstance().GetLogger("Qt");

    switch (type) {
    case QtDebugMsg:
        XAMP_LOG_D(logger, str.toStdString());
        break;
    case QtWarningMsg:
        XAMP_LOG_W(logger, str.toStdString());
        break;
    case QtCriticalMsg:
        XAMP_LOG_C(logger, str.toStdString());
        break;
    case QtFatalMsg:
        XAMP_LOG_C(logger, str.toStdString());
        break;
    default: 
        break;
    }
}
#endif

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
    if (!single_app.attach()) {
        XAMP_LOG_DEBUG("Attach app failure!");
        return -1;
    }
#ifndef _DEBUG    
#else
#ifdef XAMP_OS_WIN
    qInstallMessageHandler(logMessageHandler);
    QLoggingCategory::setFilterRules(Q_TEXT("*.info=false"));
#endif
#endif
    XAMP_LOG_DEBUG("attach app success.");

    loadLang();

    try {
        qDatabase.open(Q_TEXT("xamp.db"));
    }
    catch (const std::exception& e) {
        XAMP_LOG_ERROR("Init database failure. {}", e.what());
        return -1;
    }

    XAMP_LOG_DEBUG("Database init success.");

    qTheme.applyTheme();
    XAMP_LOG_DEBUG("ThemeManager applyTheme success.");

    XWindow top_win;
    Xamp win;

    XampIniter initer;

    try {
        initer.Init();
        initer.LoadLib();
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
    top_win.initMaximumState();
    
    return app.exec();
}

int main(int argc, char *argv[]) {
    LoggerManager::GetInstance()
        .AddDebugOutput()
#ifdef Q_OS_MAC
        .AddSink(std::make_shared<QDebugSink>())
#endif
        .AddLogFile("xamp.log")
        .Startup();

#ifdef Q_OS_WIN32
    XAMP_LOG_DEBUG(Q_STR("Version: %1 Build Visual Studio %2.%3.%4 (%5 %6)")
        .arg(kXAMPVersion)
        .arg(visualStudioVersion())
        .arg((_MSC_FULL_VER / 100000) % 100)
        .arg(_MSC_FULL_VER % 100000)
        .arg(Q_TEXT(__DATE__))
        .arg(Q_TEXT(__TIME__)).toStdString());
#else
    XAMP_LOG_DEBUG(Q_STR("Build Clang %1.%2.%3")
        .arg(__clang_major__)
        .arg(__clang_minor__)
        .arg(__clang_patchlevel__).toStdString());
#endif

#ifdef XAMP_OS_WIN
    const auto prefetch_dll = prefetchDLL();
    XAMP_LOG_DEBUG("Prefetch dll success.");

    try {
        XampIniter::LoadLib();
    }
    catch (const Exception& e) {
        XAMP_LOG_DEBUG(e.GetStackTrace());
        return -1;
    }   
#endif

    const auto os_ver = QOperatingSystemVersion::current();
    if (os_ver < QOperatingSystemVersion::Windows10) {
        QMessageBox::information(nullptr,
            Q_TEXT("Warning"),
            QString(Q_TEXT("You are running an unsupported version of Windows: %1.")).arg(os_ver.name()));
    }

    XAMP_LOG_DEBUG("Running {} {}.{}.{}",
        os_ver.name().toStdString(),
        os_ver.majorVersion(),
        os_ver.minorVersion(),
        os_ver.microVersion());

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
        LoggerManager::GetInstance().Shutdown();
    );

    auto exist_code = 0;

    try {
        exist_code = excute(argc, argv);
        if (exist_code == kRestartExistCode) {
            QProcess::startDetached(Q_STR(argv[0]), qApp->arguments());
        }
    }
    catch (Exception const& e) {
        exist_code = -1;
        XAMP_LOG_ERROR("{}", e.what());
        XAMP_LOG_ERROR("{}", e.GetStackTrace());
    }
    return exist_code;
}

