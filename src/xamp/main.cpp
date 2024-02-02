#include <iostream>
#include <QDir>
#include <qlibraryinfo.h>
#include <QLoggingCategory>
#include <QOperatingSystemVersion>
#include <QStyleFactory>

#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/logger_impl.h>
#include <base/crashhandler.h>
#include <base/stacktrace.h>
#include <base/simd.h>

#include <player/api.h>
#include <stream/soxresampler.h>

#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/imagecache.h>
#include <widget/util/str_utilts.h>
#include <widget/jsonsettings.h>
#include <widget/util/ui_utilts.h>
#include <widget/xmainwindow.h>
#include <widget/http.h>
#include <widget/xmessagebox.h>

#include <FramelessHelper/Widgets/framelessmainwindow.h>
#include <FramelessHelper/Core/private/framelessconfig_p.h>

#include <thememanager.h>
#include <singleinstanceapplication.h>
#include <version.h>
#include <xamp.h>
#include <qmessagebox.h>

namespace {
    void loadSampleRateConverterConfig() {
        XAMP_LOG_DEBUG("LoadSampleRateConverterConfig.");
        qAppSettings.loadSoxrSetting();
        qAppSettings.LoadR8BrainSetting();
        JsonSettings::save();
        XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
    }

    void loadLang() {
        XAMP_LOG_DEBUG("Load language file.");

        if (qAppSettings.valueAsString(kAppSettingLang).isEmpty()) {
            const LocaleLanguage lang;
            XAMP_LOG_DEBUG("Load locale language file: {}.", lang.isoCode().toStdString());
            qAppSettings.loadLanguage(lang.isoCode());
            qAppSettings.loadLanguage(qSTR("qt_%1").arg(lang.isoCode()));
            qAppSettings.setValue(kAppSettingLang, lang.isoCode());
        }
        else {
            qAppSettings.loadLanguage(qAppSettings.valueAsString(kAppSettingLang));
            XAMP_LOG_DEBUG("Load locale language file: {}.",
                qAppSettings.valueAsString(kAppSettingLang).toStdString());
        }
    }

#ifdef XAMP_OS_WIN
    void setWorkingSetSize() {
        const auto memory_size = GetAvailablePhysicalMemory();
        XAMP_LOG_DEBUG("GetAvailablePhysicalMemory {} success.", String::FormatBytes(memory_size));
        constexpr auto kWorkingSize = 512UL * 1024UL * 1024UL;
        SetProcessWorkingSetSize(kWorkingSize);
        XAMP_LOG_DEBUG("SetProcessWorkingSetSize {} success.", String::FormatBytes(kWorkingSize));
    }

    Vector<SharedLibraryHandle> prefetchDll() {
        // 某些DLL無法在ProcessMitigation 再次載入但是這些DLL都是必須要的.               
        const Vector<std::string_view> dll_file_names{
            R"(Python3.dll)",
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

    void logMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        QString str;
        QTextStream stream(&str);
        stream.setEncoding(QStringConverter::Utf8);

        const auto disable_stacktrace =
            qAppSettings.valueAsBool(kAppSettingEnableDebugStackTrace);

        auto get_file_name = [&context]() {
            const std::string str(context.file);
            const auto pos = str.rfind("\\");
            if (pos != std::string::npos) {
                return str.substr(pos + 1);
            }
            return str;
            };

        stream << QString::fromStdString(get_file_name()) << ":" << context.line << " (" << QString::fromStdString(GetLastErrorMessage()) << ") \r\n"
            << context.function << ": " << msg;
        if (!disable_stacktrace) {
            stream << QString::fromStdString(StackTrace{}.CaptureStack());
        }

        // Skip PNG image error
        if (str.contains(qTEXT("qpnghandler.cpp"))) {
            return;
        }

        if (str.contains(qTEXT("qwindowswindow.cpp"))) {
            stream << QString::fromStdString(StackTrace{}.CaptureStack());
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

    void applyTheme() {
        const auto theme = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
        qTheme.setThemeColor(theme);
        qTheme.loadAndApplyTheme();
    }

    int execute(int argc, char* argv[], QStringList &args) {
        qAppSettings;

        QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

        QApplication::setApplicationName(kApplicationName);
        QApplication::setApplicationVersion(kApplicationVersion);
        QApplication::setOrganizationName(kApplicationName);
        QApplication::setOrganizationDomain(kApplicationName);

        const SingleInstanceApplication app(argc, argv);
        if (!app.IsAttach()) {
            XAMP_LOG_DEBUG("Application already running!");
            return -1;
        }

        if (!QSslSocket::supportsSsl()) {
            XMessageBox::showError(qTEXT("SSL initialization failed."));
            return -1;
        }

        loadLang();

        args = app.arguments();

#ifdef _DEBUG    
#ifdef XAMP_OS_WIN
        qInstallMessageHandler(logMessageHandler);
        QLoggingCategory::setFilterRules(qTEXT("*.info=false"));
#endif
#endif

    	applyTheme();

        XMainWindow main_window;

        //*** 系統載入DLL必須在此函數中, 如果之後再載入DLL會出錯 ***//
        try {
            LoadComponentSharedLibrary();
        }
        catch (const Exception& e) {
            XMessageBox::showBug(e);
            return -1;
        }
        XAMP_LOG_DEBUG("Load component shared library success.");

        try {
            qMainDb.open();
        }
        catch (const Exception& e) {
            XMessageBox::showBug(e);
            return -1;
        }
        XAMP_LOG_DEBUG("Database init success.");

        XAMP_LOG_DEBUG("Start XAMP ...");

        Xamp win(&main_window, MakeAudioPlayer());
        win.setMainWindow(&main_window);
        win.setThemeColor(qTheme.backgroundColor(),
            qTheme.themeTextColor());

        if (qAppSettings.valueAsBool(kAppSettingEnableSandboxMode)) {
            XAMP_LOG_DEBUG("Set process mitigation.");
            SetProcessMitigation();
        }

        XAMP_LOG_DEBUG("Load all dll completed! Start sandbox mode.");

        main_window.setContentWidget(&win);
        win.adjustSize();
        win.waitForReady();
        main_window.restoreAppGeometry();
        main_window.showWindow();

        if (qAppSettings.valueAsBool(kAppSettingEnableShortcut)) {
            main_window.setShortcut(QKeySequence(Qt::Key_MediaPlay));
            main_window.setShortcut(QKeySequence(Qt::Key_MediaStop));
            main_window.setShortcut(QKeySequence(Qt::Key_MediaPrevious));
            main_window.setShortcut(QKeySequence(Qt::Key_MediaNext));
            main_window.setShortcut(QKeySequence(Qt::Key_VolumeUp));
            main_window.setShortcut(QKeySequence(Qt::Key_VolumeDown));
            main_window.setShortcut(QKeySequence(Qt::Key_VolumeMute));
            main_window.setShortcut(QKeySequence(Qt::Key_F11));
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

    qAppSettings.loadIniFile(qTEXT("xamp.ini"));
    JsonSettings::loadJsonFile(qTEXT("config.json"));

    const auto os_ver = QOperatingSystemVersion::current();
    if (os_ver >= QOperatingSystemVersion::Windows10) {
        setWorkingSetSize();
	}

    XAMP_LOG_DEBUG("Running {} {}.{}.{}",
        os_ver.name().toStdString(),
        os_ver.majorVersion(),
        os_ver.minorVersion(),
        os_ver.microVersion());

    FramelessHelperRAII frameless_helper_raii;

    qAppSettings.loadOrSaveLogConfig();
    qAppSettings.loadAppSettings();
    loadSampleRateConverterConfig();

#ifdef Q_OS_WIN32
    const auto components_path = GetComponentsFilePath();
    if (!AddSharedLibrarySearchDirectory(components_path)) {
        XAMP_LOG_ERROR("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage());
        return -1;
    }

    auto prefetch_dll = prefetchDll();
    XAMP_LOG_DEBUG("Prefetch dll success.");
#endif

    SharedCrashHandler.SetProcessExceptionHandlers();
    XAMP_LOG_DEBUG("SetProcessExceptionHandlers success.");

    SharedCrashHandler.SetThreadExceptionHandlers();
    XAMP_LOG_DEBUG("SetThreadExceptionHandlers success.");

    XAMP_ON_SCOPE_EXIT(
        JsonSettings::save();
        qAppSettings.save();
        qAppSettings.saveLogConfig();
        qMainDb.close();
        prefetch_dll.clear();
        LoggerManager::GetInstance().Shutdown();
    );

    static char app_name[] = "xamp2";
    static constexpr int argc = 1;
    static char* argv[] = { app_name, nullptr };

    QStringList args;
    auto exist_code = 0;
    try {
        exist_code = execute(argc, argv, args);        
    }
    catch (Exception const& e) {
        exist_code = -1;
        XAMP_LOG_ERROR("message:{} {}", e.what(), e.GetStackTrace());
    }

    if (exist_code == kRestartExistCode) {
        QProcess::startDetached(qSTR(argv[0]), args);
    }
    return exist_code;
}

