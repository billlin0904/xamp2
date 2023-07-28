#include <iostream>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/logger_impl.h>
#include <base/crashhandler.h>
#include <base/encodingprofile.h>
#include <base/stacktrace.h>

#include <player/api.h>
#include <stream/soxresampler.h>

#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/imagecache.h>
#include <widget/str_utilts.h>
#include <widget/jsonsettings.h>
#include <widget/ui_utilts.h>
#include <widget/xmainwindow.h>
#include <widget/xmessagebox.h>
#include <widget/http.h>
#include <widget/databasefacade.h>
#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#include <QLoggingCategory>
#endif

#include <QOperatingSystemVersion>
#include <QNetworkReply>
#include <QProcess>

#include <FramelessHelper/Widgets/framelessmainwindow.h>

#include <thememanager.h>
#include <singleinstanceapplication.h>
#include <version.h>
#include <xamp.h>

#ifdef Q_OS_WIN32
static ConstLatin1String VisualStudioVersion() {
    if constexpr (_MSC_VER >= 1930) {
        return "2022";
    }
    return "2019";
}
static std::string GetCompilerTime() {
    return qSTR("Version: %1 Build Visual Studio %2.%3.%4 (%5 %6)")
        .arg(kApplicationVersion)
        .arg(VisualStudioVersion())
        .arg((_MSC_FULL_VER / 100000) % 100)
        .arg(_MSC_FULL_VER % 100000)
        .arg(qTEXT(__DATE__))
        .arg(qTEXT(__TIME__)).toStdString();
}
#else
static std::string GetCompilerTime() {
    return qSTR("Build Clang %1.%2.%3")
        .arg(__clang_major__)
        .arg(__clang_minor__)
        .arg(__clang_patchlevel__).toStdString()
}
#endif

static void LoadSoxrSetting() {
    XAMP_LOG_DEBUG("LoadSoxrSetting.");

    QMap<QString, QVariant> default_setting;

    default_setting[kResampleSampleRate] = 96000;
    default_setting[kSoxrQuality] = static_cast<int32_t>(SoxrQuality::VHQ);
    default_setting[kSoxrPhase] = 46;
    default_setting[kSoxrStopBand] = 100;
    default_setting[kSoxrPassBand] = 96;
    default_setting[kSoxrRollOffLevel] = static_cast<int32_t>(SoxrRollOff::ROLLOFF_NONE);

    QMap<QString, QVariant> soxr_setting;
    soxr_setting[kSoxrDefaultSettingName] = default_setting;

    JsonSettings::SetDefaultValue(kSoxr, QVariant::fromValue(soxr_setting));

    if (JsonSettings::ValueAsMap(kSoxr).isEmpty()) {     
        JsonSettings::SetValue(kSoxr, QVariant::fromValue(soxr_setting));
        AppSettings::SetValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
        AppSettings::SetDefaultValue(kAppSettingSoxrSettingName, kSoxrDefaultSettingName);
    }
}

static void LoadR8BrainSetting() {
    XAMP_LOG_DEBUG("LoadR8BrainSetting.");

    QMap<QString, QVariant> default_setting;

    default_setting[kResampleSampleRate] = 96000;
    JsonSettings::SetDefaultValue(kR8Brain, QVariant::fromValue(default_setting));
    if (JsonSettings::ValueAsMap(kR8Brain).isEmpty()) {
        JsonSettings::SetValue(kR8Brain, QVariant::fromValue(default_setting));
    }
    if (!AppSettings::contains(kAppSettingResamplerType)) {
        AppSettings::SetValue(kAppSettingResamplerType, kR8Brain);
    }
}

static LogLevel ParseLogLevel(const QString &str) {
    const static QMap<QString, LogLevel> logs{
    	{ qTEXT("info"), LogLevel::LOG_LEVEL_INFO},
        { qTEXT("debug"), LogLevel::LOG_LEVEL_DEBUG},
        { qTEXT("trace"), LogLevel::LOG_LEVEL_TRACE},
        { qTEXT("warn"), LogLevel::LOG_LEVEL_WARN},
        { qTEXT("err"), LogLevel::LOG_LEVEL_ERROR},
        { qTEXT("critical"), LogLevel::LOG_LEVEL_CRITICAL},
    };
    if (!logs.contains(str)) {
        return LogLevel::LOG_LEVEL_INFO;
    }
    return logs[str];
}

static void SaveLogConfig() {
    QMap<QString, QVariant> log;
    QMap<QString, QVariant> min_level;
    QMap<QString, QVariant> well_known_log_name;
    QMap<QString, QVariant> override_map;

    for (const auto& logger : XAMP_DEFAULT_LOG().GetAllLogger()) {
        if (logger->GetName() != std::string(kXampLoggerName)) {
            well_known_log_name[FromStdStringView(logger->GetName())] = qTEXT("info");
        }
    }

    min_level[kLogDefault] = qTEXT("debug");

    XAMP_DEFAULT_LOG().SetLevel(ParseLogLevel(min_level[kLogDefault].toString()));

    for (auto itr = well_known_log_name.begin()
        ; itr != well_known_log_name.end(); ++itr) {
        override_map[itr.key()] = itr.value();
        XAMP_DEFAULT_LOG().GetLogger(itr.key().toStdString())
            ->SetLevel(ParseLogLevel(itr.value().toString()));
    }

    min_level[kLogOverride] = override_map;
    log[kLogMinimumLevel] = min_level;
    JsonSettings::SetValue(kLog, QVariant::fromValue(log));
    JsonSettings::SetDefaultValue(kLog, QVariant::fromValue(log));
}

static void LoadOrSaveLogConfig() {
    XAMP_LOG_DEBUG("LoadOrSaveLogConfig.");

    QMap<QString, QVariant> log;
    QMap<QString, QVariant> min_level;
    QMap<QString, QVariant> override_map;

    QMap<QString, QVariant> well_known_log_name;

    for (const auto& logger : XAMP_DEFAULT_LOG().GetAllLogger()) {
        if (logger->GetName() != std::string(kXampLoggerName)) {
            well_known_log_name[FromStdStringView(logger->GetName())] = qTEXT("info");
        }
    }

    if (JsonSettings::ValueAsMap(kLog).isEmpty()) {
        min_level[kLogDefault] = qTEXT("debug");

        XAMP_DEFAULT_LOG().SetLevel(ParseLogLevel(min_level[kLogDefault].toString()));

    	for (auto itr = well_known_log_name.begin()
            ; itr != well_known_log_name.end(); ++itr) {
            override_map[itr.key()] = itr.value();
            XAMP_DEFAULT_LOG().GetLogger(itr.key().toStdString())
                ->SetLevel(ParseLogLevel(itr.value().toString()));
        }

        min_level[kLogOverride] = override_map;
        log[kLogMinimumLevel] = min_level;
        JsonSettings::SetValue(kLog, QVariant::fromValue(log));
        JsonSettings::SetDefaultValue(kLog, QVariant::fromValue(log));
    } else {
        log = JsonSettings::ValueAsMap(kLog);
        min_level = log[kLogMinimumLevel].toMap();

        const auto default_level = min_level[kLogDefault].toString();
        XAMP_DEFAULT_LOG().SetLevel(ParseLogLevel(default_level));

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
        	->SetLevel(ParseLogLevel(log_level));
        }       
    }

#ifdef _DEBUG
    XAMP_DEFAULT_LOG().GetLogger(kCrashHandlerLoggerName)
        ->SetLevel(LOG_LEVEL_DEBUG);
#endif
}

static void RegisterMetaType() {
    XAMP_LOG_DEBUG("RegisterMetaType.");

    // For QSetting read
    qRegisterMetaTypeStreamOperators<AppEQSettings>("AppEQSettings");

    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<QSharedPointer<DatabaseFacade>>("QSharedPointer<DatabaseFacade>");
    qRegisterMetaType<AppEQSettings>("AppEQSettings");
    qRegisterMetaType<Vector<TrackInfo>>("Vector<TrackInfo>");
    qRegisterMetaType<DeviceState>("DeviceState");
    qRegisterMetaType<PlayerState>("PlayerState");
    qRegisterMetaType<PlayListEntity>("PlayListEntity");
    qRegisterMetaType<Errors>("Errors");
    qRegisterMetaType<Vector<float>>("Vector<float>");
    qRegisterMetaType<QList<PlayListEntity>>("QList<PlayListEntity>");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<ComplexValarray>("ComplexValarray");
    qRegisterMetaType<QList<TrackInfo>>("QList<TrackInfo>");
    qRegisterMetaType<Vector<TrackInfo>>("Vector<TrackInfo>");
    qRegisterMetaType<DriveInfo>("DriveInfo");
    qRegisterMetaType<EncodingProfile>("EncodingProfile");
    qRegisterMetaType<std::wstring>("std::wstring");
}

static void LoadAppSettings() {
    RegisterMetaType();

    XAMP_LOG_DEBUG("LoadAppSettings.");

	AppSettings::SetDefaultEnumValue(kAppSettingOrder, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
    AppSettings::SetDefaultEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
    AppSettings::SetDefaultEnumValue(kAppSettingReplayGainScanMode, ReplayGainScanMode::RG_SCAN_MODE_FAST);
    AppSettings::SetDefaultEnumValue(kAppSettingTheme, ThemeColor::DARK_THEME);

    AppSettings::SetDefaultValue(kAppSettingDeviceType, kEmptyString);
    AppSettings::SetDefaultValue(kAppSettingDeviceId, kEmptyString);
    AppSettings::SetDefaultValue(kAppSettingVolume, 50);
    AppSettings::SetDefaultValue(kAppSettingUseFramelessWindow, true);
    AppSettings::SetDefaultValue(kLyricsFontSize, 12);
    AppSettings::SetDefaultValue(kAppSettingMinimizeToTray, true);
    AppSettings::SetDefaultValue(kAppSettingDiscordNotify, false);
    AppSettings::SetDefaultValue(kFlacEncodingLevel, 8);
    AppSettings::SetDefaultValue(kAppSettingShowLeftList, true);
    AppSettings::SetDefaultValue(kAppSettingReplayGainTargetGain, kReferenceGain);
    AppSettings::SetDefaultValue(kAppSettingReplayGainTargetLoudnes, kReferenceLoudness);
    AppSettings::SetDefaultValue(kAppSettingEnableReplayGain, true);
    AppSettings::SetDefaultValue(kEnableBitPerfect, true);
    AppSettings::SetDefaultValue(kAppSettingWindowState, false);
    AppSettings::SetDefaultValue(kAppSettingScreenNumber, 1);
    AppSettings::SetDefaultValue(kAppSettingEnableSpectrum, true);
    AppSettings::SetDefaultValue(kAppSettingEnableShortcut, true);
    AppSettings::SetDefaultValue(kAppSettingEnterFullScreen, false);
    AppSettings::SetDefaultValue(kAppSettingEnableSandboxMode, true);
    AppSettings::SetDefaultValue(kAppSettingEnableDebugStackTrace, true);

    AppSettings::SetDefaultValue(kAppSettingAlbumPlaylistColumnName, qTEXT("3, 6, 10"));
    AppSettings::SetDefaultValue(kAppSettingFileSystemPlaylistColumnName, qTEXT("3, 6, 10"));
    AppSettings::SetDefaultValue(kAppSettingCdPlaylistColumnName, qTEXT("3, 6, 10"));
    XAMP_LOG_DEBUG("loadAppSettings success.");
}

static void LoadSampleRateConverterConfig() {
    XAMP_LOG_DEBUG("LoadSampleRateConverterConfig.");
    LoadSoxrSetting();
    LoadR8BrainSetting();
    JsonSettings::save();
    XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
}

static void LoadLang() {
    XAMP_LOG_DEBUG("Load language file.");

    if (AppSettings::ValueAsString(kAppSettingLang).isEmpty()) {
        const LocaleLanguage lang;
        XAMP_LOG_DEBUG("Load locale Language file: {}.", lang.GetIsoCode().toStdString());
        AppSettings::LoadLanguage(lang.GetIsoCode());
        AppSettings::SetValue(kAppSettingLang, lang.GetIsoCode());
    }
    else {
        AppSettings::LoadLanguage(AppSettings::ValueAsString(kAppSettingLang));
        XAMP_LOG_DEBUG("Load locale Language file: {}.",
            AppSettings::ValueAsString(kAppSettingLang).toStdString());
    }
}

#ifdef XAMP_OS_WIN
static void SetWorkingSetSize() {
	const auto memory_size = GetAvailablePhysicalMemory();
    XAMP_LOG_DEBUG("GetAvailablePhysicalMemory {} success.", String::FormatBytes(memory_size));
    //auto working_size = memory_size * 0.6;
    auto working_size = 128UL * 1024UL * 1024UL;
    if (working_size > 0) {
        SetProcessWorkingSetSize(working_size);
        XAMP_LOG_DEBUG("SetProcessWorkingSetSize {} success.", String::FormatBytes(working_size));
    }
}

static std::vector<SharedLibraryHandle> PinSystemDll() {
    const std::vector<std::string_view> dll_file_names{
        "psapi.dll",
        "setupapi.dll",
        "WinTypes.dll",
        "AudioSes.dll",
        "AUDIOKSE.dll",
        "DWrite.dll",        
    };
    std::vector<SharedLibraryHandle> pin_module;
    for (const auto& file_name : dll_file_names) {
        try {
            auto module = PinSystemLibrary(file_name);
            if (PrefetchSharedLibrary(module)) {
                pin_module.push_back(std::move(module));
                XAMP_LOG_DEBUG("\tPreload => {} success.", file_name);
            }
        }
        catch (Exception const& e) {
            XAMP_LOG_DEBUG("Pin {} failure! {}.", file_name, e.GetErrorMessage());
        }
    }
    return pin_module;
}

static std::vector<SharedLibraryHandle> PrefetchDll() {
    const std::vector<std::string_view> dll_file_names{
        R"("mimalloc-override.dll)",
        R"(C:\Program Files\Topping\USB Audio Device Driver\x64\ToppingUsbAudioasio_x64.dll)",
        R"(C:\Program Files\iFi\USB_HD_Audio_Driver\iFiHDUSBAudioasio_x64.dll)",
        R"(C:\Program Files\FiiO\FiiO_Driver\W10_x64\fiio_usbaudioasio_x64.dll)",
    	R"(C:\Program Files\Bonjour\mdnsNSP.dll)",
    #ifndef _DEBUG
        "Qt5Gui.dll",
        "Qt5Core.dll",
        "Qt5Widgets.dll",
        "Qt5Sql.dll",
        "Qt5Network.dll",
        "Qt5WinExtras.dll"
	#endif
    };
    std::vector<SharedLibraryHandle> preload_module;
    for (const auto& file_name : dll_file_names) {
        try {
            auto module = LoadSharedLibrary(file_name);
            if (PrefetchSharedLibrary(module)) {
                preload_module.push_back(std::move(module));
                XAMP_LOG_DEBUG("\tPreload => {} success.", file_name);
            }
        }
        catch (Exception const& e) {
            XAMP_LOG_DEBUG("Preload {} failure! {}.", file_name, e.GetErrorMessage());
        }
    }
    return preload_module;
}
#endif 

#ifdef _DEBUG
XAMP_DECLARE_LOG_NAME(Qt);

static void LogMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString str;
    QTextStream stream(&str);
    stream.setCodec("UTF-8");

    const auto disable_stacktrace = 
        AppSettings::ValueAsBool(kAppSettingEnableDebugStackTrace);

    stream << context.file << ":" << context.line << ":"
        << context.function << ": " << msg;
    if (!disable_stacktrace) {
        stream << QString::fromStdString(StackTrace{}.CaptureStack());
    }

    constexpr auto skip_tag = qTEXT("image\\qpnghandler.cpp:525");
    if (str.contains(skip_tag)) {
        return;
    }
    const auto logger = LoggerManager::GetInstance().GetLogger(kQtLoggerName);

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

static void ApplyTheme() {
	const auto theme = AppSettings::ValueAsEnum<ThemeColor>(kAppSettingTheme);
    qTheme.SetThemeColor(theme);
    qTheme.LoadAndApplyQssTheme();    
}

static int Execute(int argc, char* argv[]) {
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

    QApplication::setApplicationName(kApplicationName);
    QApplication::setApplicationVersion(kApplicationVersion);
    QApplication::setOrganizationName(kApplicationName);
    QApplication::setOrganizationDomain(kApplicationName);

    const SingleInstanceApplication app(argc, argv);
    if (!app.IsAttach()) {
        XAMP_LOG_DEBUG("Application already running!");
        return -1;
    }

    ApplyTheme();

#ifdef _DEBUG    
#ifdef XAMP_OS_WIN
    qInstallMessageHandler(LogMessageHandler);
    QLoggingCategory::setFilterRules(qTEXT("*.info=false"));
#endif
#endif

    XMainWindow main_window;

    XAMP_LOG_DEBUG("attach application success.");

    try {
        LoadComponentSharedLibrary();
    }
    catch (const Exception& e) {
        XMessageBox::ShowBug(e);
        return -1;
    }
    XAMP_LOG_DEBUG("Load component shared library success.");

    LoadLang();    

    try {
        qDatabase.open();
    }
    catch (const Exception& e) {        
        XMessageBox::ShowBug(e);
        return -1;
    }
    XAMP_LOG_DEBUG("Database init success.");    

    if (!QSslSocket::supportsSsl()) {
        XMessageBox::ShowError(qTEXT("SSL initialization failed."));
        return -1;
    }    
    
    if (AppSettings::ValueAsBool(kAppSettingEnableShortcut)) {
        main_window.SetShortcut(QKeySequence(Qt::Key_MediaPlay));
        main_window.SetShortcut(QKeySequence(Qt::Key_MediaStop));
        main_window.SetShortcut(QKeySequence(Qt::Key_MediaPrevious));
        main_window.SetShortcut(QKeySequence(Qt::Key_MediaNext));
        main_window.SetShortcut(QKeySequence(Qt::Key_VolumeUp));
        main_window.SetShortcut(QKeySequence(Qt::Key_VolumeDown));
        main_window.SetShortcut(QKeySequence(Qt::Key_VolumeMute));
        main_window.SetShortcut(QKeySequence(Qt::Key_F11));
    }

    Xamp win(&main_window, MakeAudioPlayer());    
    win.SetXWindow(&main_window);
    win.SetThemeColor(qTheme.GetBackgroundColor(),
        qTheme.GetThemeTextColor());

#ifdef Q_OS_WIN32
    const auto os_ver = QOperatingSystemVersion::current();
    XAMP_LOG_DEBUG("Running {} {}.{}.{}",
        os_ver.name().toStdString(),
        os_ver.majorVersion(),
        os_ver.minorVersion(),
        os_ver.microVersion());

    if (AppSettings::ValueAsBool(kAppSettingEnableSandboxMode)) {
        XAMP_LOG_DEBUG("Set process mitigation.");
        SetProcessMitigation();
    }

    XAMP_LOG_DEBUG("Load all dll completed! Start sandbox mode.");
#endif

    main_window.SetContentWidget(&win);
    //top_win.SetContentWidget(nullptr);
    main_window.RestoreGeometry();
    main_window.ShowWindow();
    return app.exec();
}

struct FramelessHelperRAII {
    FramelessHelperRAII() {
        FramelessHelper::Widgets::initialize();
    }

    ~FramelessHelperRAII() {
        FramelessHelper::Widgets::uninitialize();
    }
};

int main() {
    LoggerManager::GetInstance()
        .AddDebugOutput()
#ifdef Q_OS_MAC
        .AddSink(std::make_shared<QDebugSink>())
#endif
        .AddLogFile("xamp.log")
        .Startup();

    FramelessHelperRAII frameless_helper_raii;

    AppSettings::LoadIniFile(qTEXT("xamp.ini"));
    JsonSettings::LoadJsonFile(qTEXT("config.json"));

	LoadOrSaveLogConfig();

    XAMP_LOG_DEBUG(GetCompilerTime());

#ifdef Q_OS_WIN32
    SetWorkingSetSize();

    const auto components_path = GetComponentsFilePath();
    if (!AddSharedLibrarySearchDirectory(components_path)) {
        XAMP_LOG_ERROR("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage());
        return -1;
    }

    const auto prefetch_dll = PrefetchDll();
    XAMP_LOG_DEBUG("Prefetch dll success.");

    const auto pin_system_dll = PinSystemDll();
    XAMP_LOG_DEBUG("Pin system dll success.");
#endif

    LoadAppSettings();
    LoadSampleRateConverterConfig();

    SharedCrashHandler.SetProcessExceptionHandlers();
    XAMP_LOG_DEBUG("SetProcessExceptionHandlers success.");

    SharedCrashHandler.SetThreadExceptionHandlers();
    XAMP_LOG_DEBUG("SetThreadExceptionHandlers success.");

    XAMP_ON_SCOPE_EXIT(
        JsonSettings::save();
        AppSettings::save();
        SaveLogConfig();
        LoggerManager::GetInstance().Shutdown();
    );

    auto exist_code = 0;
    try {
        static char app_name[] = "xamp2";
        static const int argc = 1;
        static char* argv[] = { app_name, nullptr };

        exist_code = Execute(argc, argv);
        if (exist_code == kRestartExistCode) {
            QProcess::startDetached(qSTR(argv[0]), qApp->arguments());
        }
    }
    catch (Exception const& e) {
        exist_code = -1;
        XAMP_LOG_ERROR("message:{} {}", e.what(), e.GetStackTrace());
    }    
    return exist_code;
}

