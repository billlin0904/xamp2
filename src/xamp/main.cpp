#include <thememanager.h>
#include <xapplication.h>
#include <version.h>
#include <xamp.h>

#include <base/scopeguard.h>
#include <base/dll.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/xmessagebox.h>
#include <widget/xmainwindow.h>
#include <widget/jsonsettings.h>

#include <FramelessHelper/Widgets/framelessmainwindow.h>
#include <FramelessHelper/Core/private/framelessconfig_p.h>

#include <QSslSocket>

namespace {
    Vector<SharedLibraryHandle> prefetchDll() {
        // 某些DLL無法在ProcessMitigation 再次載入但是這些DLL都是必須要的.               
        const Vector<std::string_view> dll_file_names{
            R"(WS2_32.dll)",
            R"(Python3.dll)",
            R"(mimalloc-override.dll)",
            R"(C:\Program Files\Topping\USB Audio Device Driver\x64\ToppingUsbAudioasio_x64.dll)",
            R"(C:\Program Files\iFi\USB_HD_Audio_Driver\iFiHDUSBAudioasio_x64.dll)",
            R"(C:\Program Files\FiiO\FiiO_Driver\W10_x64\fiio_usbaudioasio_x64.dll)",
            R"(C:\Program Files\Bonjour\mdnsNSP.dll)",
        };
        Vector<SharedLibraryHandle> preload_module;
#ifdef Q_OS_WIN
        for (const auto& file_name : dll_file_names) {
            try {
                auto module = LoadSharedLibrary(file_name);
                if (PrefetchSharedLibrary(module)) {
                    preload_module.push_back(std::move(module));
                    XAMP_LOG_DEBUG("\tPreload => {} success.", file_name);
                }
            }
            catch (const Exception& e) {
                XAMP_LOG_DEBUG("Preload {} failure! {}.", file_name, e.GetErrorMessage());
            }
        }
#endif
        return preload_module;
    }

#ifdef _DEBUG
    XAMP_DECLARE_LOG_NAME(Qt);

    void logMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        QString str;
        QTextStream stream(&str);
        stream.setEncoding(QStringConverter::Utf8);

        const auto disable_stack_trace =
            qAppSettings.valueAsBool(kAppSettingEnableDebugStackTrace);

        auto get_file_name = [&context]() -> std::string {
            if (!context.file) {
                return "";
            }
            const std::string str(context.file);
            const auto pos = str.rfind("\\");
            if (pos != std::string::npos) {
                return str.substr(pos + 1);
            }
            return str;
            };

        stream << QString::fromStdString(get_file_name()) << ":" << context.line << " (" << QString::fromStdString(GetLastErrorMessage()) << ") \r\n"
            << context.function << ": " << msg;
        if (!disable_stack_trace) {
            stream << QString::fromStdString(StackTrace{}.CaptureStack());
        }

        // Skip PNG image error
        if (str.contains(qTEXT("qpnghandler.cpp"))) {
            return;
        }

        if (str.contains(qTEXT("qwindowswindow.cpp"))) {
            stream << QString::fromStdString(StackTrace{}.CaptureStack());
        }

        const auto logger = XampLoggerFactory.GetLogger(kQtLoggerName);

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

    struct FramelessHelperScoped {
        FramelessHelperScoped() {
            FramelessHelper::Widgets::initialize();
            FramelessHelper::Core::setApplicationOSThemeAware();
            FramelessConfig::instance()->set(Global::Option::DisableWindowsSnapLayout);
            FramelessConfig::instance()->set(Global::Option::EnableBlurBehindWindow);
        }

		~FramelessHelperScoped() {
            FramelessHelper::Widgets::uninitialize();
		}
    };

    int execute(int argc, char* argv[], QStringList &args) {
#ifdef Q_OS_WIN32
        const auto components_path = GetComponentsFilePath();
        if (!AddSharedLibrarySearchDirectory(components_path)) {
            XAMP_LOG_ERROR("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage());
            return -1;
        }

        auto prefetch_dll = prefetchDll();
        XAMP_LOG_DEBUG("Prefetch dll success.");
#endif

		FramelessHelperScoped scoped;

        QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

        QApplication::setApplicationName(kApplicationName);
        QApplication::setApplicationVersion(kApplicationVersion);
        QApplication::setOrganizationName(kApplicationName);
        QApplication::setOrganizationDomain(kApplicationName);

        XApplication app(argc, argv);
        /*if (!app.isAttach()) {
            XAMP_LOG_DEBUG("Application already running!");
            return -1;
        }*/        

		app.initial();
        app.loadLang();
        app.loadSampleRateConverterConfig();
        app.applyTheme();
        
#ifdef _DEBUG    
#ifdef Q_OS_WIN
        qInstallMessageHandler(logMessageHandler);
        QLoggingCategory::setFilterRules(qTEXT("*.info=false"));
#endif
#endif

        if (!QSslSocket::supportsSsl()) {
            XMessageBox::showError(qTEXT("SSL initialization failed."));
            return -1;
        }

        //*** 系統載入DLL必須在此函數中, 如果之後再載入DLL會出錯 ***//
        try {            
            LoadComponentSharedLibrary();
        }
        catch (const Exception& e) {
            XMessageBox::showBug(e);
            return -1;
        }
        XAMP_LOG_DEBUG("Load component shared library success.");

        XAMP_LOG_DEBUG("Database start initial...");

        try {			
            qGuiDb.open();
        }
        catch (const Exception& e) {
            XMessageBox::showBug(e);
            return -1;
        }
        XAMP_LOG_DEBUG("Database init success.");

        XAMP_LOG_DEBUG("Start XAMP window...");

        XMainWindow main_window;
        Xamp win(&main_window, MakeAudioPlayer());
        win.setMainWindow(&main_window);
        win.setThemeColor(qTheme.backgroundColor(),
            qTheme.themeTextColor());

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
}

int main() { 
    try {
        XampLoggerFactory
            .AddDebugOutput()
#ifdef Q_OS_MAC
            .AddSink(std::make_shared<QDebugSink>())
#endif
            .AddLogFile("xamp.log")
            .Startup();
    }
	catch (const std::exception& e) {
        auto message = String::ToStdWString(e.what());
		::MessageBox(nullptr, message.c_str(), L"Logger startup failure!", MB_OK | MB_ICONERROR);
		return -1;
	}

    // Disable ECO-QOS mode.
	SetCurrentProcessPriority(ProcessPriority::PRIORITY_FOREGROUND);

    XAMP_ON_SCOPE_EXIT(
        qJsonSettings.save();
        qAppSettings.save();
        qAppSettings.saveLogConfig();
        qGuiDb.close();
        XampLoggerFactory.Shutdown();
    );

    qputenv("QT_ICC_PROFILE", QByteArray());
    QLoggingCategory::setFilterRules(QStringLiteral("qt.gui.imageio.warning=false"));

    static char app_name[] = "xamp2";
    static constexpr int argc = 1;
    static char* argv[] = { app_name, nullptr };

    QStringList args;
    auto exist_code = 0;
    try {
        exist_code = execute(argc, argv, args);        
    }
    catch (const Exception& e) {
        exist_code = -1;
        XAMP_LOG_ERROR("message:{} {}", e.what(), e.GetStackTrace());
    }

    if (exist_code == kRestartExistCode) {
        QProcess::startDetached(qSTR(argv[0]), args);
    }
    return exist_code;
}

