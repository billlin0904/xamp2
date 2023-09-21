#include <iostream>
#include <QDir>
#include <qlibraryinfo.h>

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
#include <widget/http.h>
#include <widget/databasefacade.h>
#include <widget/log_util.h>

#include <QLoggingCategory>
#include <QOperatingSystemVersion>
#include <QProcess>

#include <FramelessHelper/Widgets/framelessmainwindow.h>
#include <FramelessHelper/Core/private/framelessconfig_p.h>

#include <thememanager.h>
#include <xmessagebox.h>
#include <singleinstanceapplication.h>
#include <version.h>
#include <xamp.h>
#include <qmessagebox.h>

namespace {
#ifdef Q_OS_WIN32
    ConstLatin1String VisualStudioVersion() {
        if constexpr (_MSC_VER >= 1930) {
            return "2022";
        }
        return "2019";
    }
    std::string GetCompilerTime() {
        return qSTR("Version: %1 Build Visual Studio %2.%3.%4 (%5 %6)")
            .arg(kApplicationVersion)
            .arg(VisualStudioVersion())
            .arg((_MSC_FULL_VER / 100000) % 100)
            .arg(_MSC_FULL_VER % 100000)
            .arg(qTEXT(__DATE__))
            .arg(qTEXT(__TIME__)).toStdString();
    }
#else
    std::string GetCompilerTime() {
        return qSTR("Build Clang %1.%2.%3")
            .arg(__clang_major__)
            .arg(__clang_minor__)
            .arg(__clang_patchlevel__).toStdString()
    }
#endif

    void LoadSampleRateConverterConfig() {
        XAMP_LOG_DEBUG("LoadSampleRateConverterConfig.");
        AppSettings::LoadSoxrSetting();
        AppSettings::LoadR8BrainSetting();
        JsonSettings::save();
        XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
    }

    void LoadLang() {
        XAMP_LOG_DEBUG("Load language file.");

        if (AppSettings::ValueAsString(kAppSettingLang).isEmpty()) {
            const LocaleLanguage lang;
            XAMP_LOG_DEBUG("Load locale language file: {}.", lang.GetIsoCode().toStdString());
            AppSettings::LoadLanguage(lang.GetIsoCode());
            AppSettings::SetValue(kAppSettingLang, lang.GetIsoCode());
        }
        else {
            AppSettings::LoadLanguage(AppSettings::ValueAsString(kAppSettingLang));
            XAMP_LOG_DEBUG("Load locale language file: {}.",
                AppSettings::ValueAsString(kAppSettingLang).toStdString());
        }
    }

#ifdef XAMP_OS_WIN
    void SetWorkingSetSize() {
        const auto memory_size = GetAvailablePhysicalMemory();
        XAMP_LOG_DEBUG("GetAvailablePhysicalMemory {} success.", String::FormatBytes(memory_size));
        //auto working_size = memory_size * 0.6;
        const auto working_size = 128UL * 1024UL * 1024UL;
        if (working_size > 0) {
            SetProcessWorkingSetSize(working_size);
            XAMP_LOG_DEBUG("SetProcessWorkingSetSize {} success.", String::FormatBytes(working_size));
        }
    }

    Vector<SharedLibraryHandle> PinSystemDll() {
        constexpr std::array<std::string_view, 6> dll_file_names{
            R"("psapi.dll")",
            R"("setupapi.dll")",
            R"("WinTypes.dll")",
            R"("AudioSes.dll")",
            R"("AUDIOKSE.dll")",
            R"("DWrite.dll")",
        };
        Vector<SharedLibraryHandle> pin_module;
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

    Vector<SharedLibraryHandle> PrefetchDll() {
        constexpr std::array<std::string_view, 5> dll_file_names{
            R"(mimalloc-override.dll)",
            R"(C:\Program Files\Topping\USB Audio Device Driver\x64\ToppingUsbAudioasio_x64.dll)",
            R"(C:\Program Files\iFi\USB_HD_Audio_Driver\iFiHDUSBAudioasio_x64.dll)",
            R"(C:\Program Files\FiiO\FiiO_Driver\W10_x64\fiio_usbaudioasio_x64.dll)",
            R"(C:\Program Files\Bonjour\mdnsNSP.dll)",
        };
        Vector<SharedLibraryHandle> preload_module;
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

    void LogMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        QString str;
        QTextStream stream(&str);
        stream.setEncoding(QStringConverter::Utf8);

        const auto disable_stacktrace =
            AppSettings::ValueAsBool(kAppSettingEnableDebugStackTrace);

        stream << context.file << ":" << context.line << ":"
            << context.function << ": " << msg;
        if (!disable_stacktrace) {
            stream << QString::fromStdString(StackTrace{}.CaptureStack());
        }

        constexpr auto skip_png_handler_tag = qTEXT("image\\qpnghandler.cpp:525");
        if (str.contains(skip_png_handler_tag)) {
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

    void ApplyTheme() {
        const auto theme = AppSettings::ValueAsEnum<ThemeColor>(kAppSettingTheme);
        qTheme.SetThemeColor(theme);
        qTheme.LoadAndApplyQssTheme();
    }

    int Execute(int argc, char* argv[]) {
        QApplication::setApplicationName(kApplicationName);
        QApplication::setApplicationVersion(kApplicationVersion);
        QApplication::setOrganizationName(kApplicationName);
        QApplication::setOrganizationDomain(kApplicationName);

        const SingleInstanceApplication app(argc, argv);
        if (!app.IsAttach()) {
            XAMP_LOG_DEBUG("Application already running!");
            return -1;
        }

#ifdef _DEBUG    
#ifdef XAMP_OS_WIN
        qInstallMessageHandler(LogMessageHandler);
        QLoggingCategory::setFilterRules(qTEXT("*.info=false"));
#endif
#endif

        ApplyTheme();
        LoadLang();

        XMainWindow main_window;
        try {
            LoadComponentSharedLibrary();
        }
        catch (const Exception& e) {
            XMessageBox::ShowBug(e);
            return -1;
        }
        XAMP_LOG_DEBUG("Load component shared library success.");

        try {
            qMainDb.open();
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

        Xamp win(&main_window, MakeAudioPlayer());
        win.SetXWindow(&main_window);
        win.SetThemeColor(qTheme.GetBackgroundColor(),
            qTheme.GetThemeTextColor());

        if (AppSettings::ValueAsBool(kAppSettingEnableSandboxMode)) {
            XAMP_LOG_DEBUG("Set process mitigation.");
            SetProcessMitigation();
        }

        XAMP_LOG_DEBUG("Load all dll completed! Start sandbox mode.");

        main_window.SetContentWidget(&win);
        win.WaitForReady();
        main_window.RestoreGeometry();
        main_window.ShowWindow();

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

        return app.exec();
    }

    struct FramelessHelperRAII {
        FramelessHelperRAII() {
            FramelessHelper::Widgets::initialize();
            FramelessHelper::Core::setApplicationOSThemeAware();
            FramelessConfig::instance()->set(Global::Option::DisableWindowsSnapLayout);
        }

        ~FramelessHelperRAII() {
            FramelessHelper::Widgets::uninitialize();
        }
    };
}

int main() {
    LoggerManager::GetInstance()
        .AddDebugOutput()
#ifdef Q_OS_MAC
        .AddSink(std::make_shared<QDebugSink>())
#endif
        .AddLogFile("xamp.log")
        .Startup();

    const auto os_ver = QOperatingSystemVersion::current();
    if (os_ver >= QOperatingSystemVersion::Windows10
        && os_ver < QOperatingSystemVersion::Windows11) {
        SetWorkingSetSize();
	}
    XAMP_LOG_DEBUG("Running {} {}.{}.{}",
        os_ver.name().toStdString(),
        os_ver.majorVersion(),
        os_ver.minorVersion(),
        os_ver.microVersion());

    FramelessHelperRAII frameless_helper_raii;

    AppSettings::LoadIniFile(qTEXT("xamp.ini"));
    JsonSettings::LoadJsonFile(qTEXT("config.json"));

    AppSettings::LoadOrSaveLogConfig();
    AppSettings::LoadAppSettings();
    LoadSampleRateConverterConfig();

    XAMP_LOG_DEBUG(GetCompilerTime());

#ifdef Q_OS_WIN32
    const auto components_path = GetComponentsFilePath();
    if (!AddSharedLibrarySearchDirectory(components_path)) {
        XAMP_LOG_ERROR("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage());
        return -1;
    }

    auto prefetch_dll = PrefetchDll();
    XAMP_LOG_DEBUG("Prefetch dll success.");

    auto pin_system_dll = PinSystemDll();
    XAMP_LOG_DEBUG("Pin system dll success.");
#endif

    SharedCrashHandler.SetProcessExceptionHandlers();
    XAMP_LOG_DEBUG("SetProcessExceptionHandlers success.");

    SharedCrashHandler.SetThreadExceptionHandlers();
    XAMP_LOG_DEBUG("SetThreadExceptionHandlers success.");

    XAMP_ON_SCOPE_EXIT(
        JsonSettings::save();
        AppSettings::save();
        AppSettings::SaveLogConfig();
        qMainDb.close();
        prefetch_dll.clear();
        pin_system_dll.clear();
        LoggerManager::GetInstance().Shutdown();
    );

    static char app_name[] = "xamp2";
    static constexpr int argc = 1;
    static char* argv[] = { app_name, nullptr };

    auto exist_code = 0;
    try {
        exist_code = Execute(argc, argv);        
    }
    catch (Exception const& e) {
        exist_code = -1;
        XAMP_LOG_ERROR("message:{} {}", e.what(), e.GetStackTrace());
    }

    if (exist_code == kRestartExistCode) {
        QProcess::startDetached(qSTR(argv[0]), qApp->arguments());
    }
    return exist_code;
}

