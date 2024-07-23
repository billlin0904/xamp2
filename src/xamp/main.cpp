#include <thememanager.h>
#include <xapplication.h>
#include <version.h>
#include <xamp.h>

#include <iostream>

#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/crashhandler.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/base_sink.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/xmessagebox.h>
#include <widget/xmainwindow.h>
#include <widget/jsonsettings.h>

#include <FramelessHelper/Widgets/framelessmainwindow.h>
#include <FramelessHelper/Core/private/framelessconfig_p.h>
#include <FramelessHelper/Core/framelesshelpercore_global.h>

#include <QPermissions>
#include <QSslSocket>

namespace {
#ifdef Q_OS_MAC
    class QDebugSink : public spdlog::sinks::base_sink<LoggerMutex> {
    public:
        void sink_it_(const spdlog::details::log_msg& msg) override {
            spdlog::memory_buf_t formatted;
            formatter_->format(msg, formatted);

            std::cout << fmt::to_string(formatted);
        }

        void flush_() override {
        }
    };
#else
    Vector<SharedLibraryHandle> prefetchDll() {
        Vector<SharedLibraryHandle> preload_module;
#ifdef Q_OS_WIN
        // 某些DLL無法在ProcessMitigation 再次載入但是這些DLL都是必須要的.               
        const Vector<std::string_view> dll_file_names{
            //R"(WS2_32.dll)",
            //R"(Python3.dll)",
            //R"(mimalloc-override.dll)",
            //R"(C:\Program Files\Topping\USB Audio Device Driver\x64\ToppingUsbAudioasio_x64.dll)",
            //R"(C:\Program Files\iFi\USB_HD_Audio_Driver\iFiHDUSBAudioasio_x64.dll)",
            //R"(C:\Program Files\FiiO\FiiO_Driver\W10_x64\fiio_usbaudioasio_x64.dll)",
            R"(C:\Program Files\Bonjour\mdnsNSP.dll)",
        };
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
#endif

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

    int execute(int argc, char* argv[], QStringList &args) {
#ifdef Q_OS_WIN32
        struct FramelessHelperScoped {
            FramelessHelperScoped() {
                FramelessHelper::Widgets::initialize();
                FramelessHelper::Core::initialize();
                FramelessConfig::instance()->set(Global::Option::DisableWindowsSnapLayout);
                //FramelessConfig::instance()->set(Global::Option::EnableBlurBehindWindow);
                //FramelessConfig::instance()->set(Global::Option::DisableLazyInitializationForMicaMaterial);
                //FramelessHelper::Core::setApplicationOSThemeAware();
            }

            ~FramelessHelperScoped() {
                FramelessHelper::Widgets::uninitialize();
                FramelessHelper::Core::uninitialize();
            }
        };

        const auto components_path = GetComponentsFilePath();
        if (!AddSharedLibrarySearchDirectory(components_path)) {
            XAMP_LOG_ERROR("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage());
            return -1;
        }

        auto prefetch_dll = prefetchDll();
        XAMP_LOG_DEBUG("Prefetch dll success.");
        FramelessHelperScoped scoped;
#else

#endif

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
        qInstallMessageHandler(logMessageHandler);
        QLoggingCategory::setFilterRules(qTEXT("*.info=false"));
#endif

        if (!QSslSocket::supportsSsl()) {
            XMessageBox::showError(qTEXT("SSL initialization failed."));
            return -1;
        }

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
        win.setThemeColor(qTheme.backgroundColor(), qTheme.themeTextColor());

        main_window.setContentWidget(&win);
        //main_window.setContentWidget(nullptr);
        win.adjustSize();
        main_window.restoreAppGeometry();
        main_window.showWindow();

#ifdef Q_OS_WIN
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
#endif

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
		return -1;
	}

    XampCrashHandler.SetProcessExceptionHandlers();
    XAMP_LOG_DEBUG("SetProcessExceptionHandlers success.");

    XampCrashHandler.SetThreadExceptionHandlers();
    XAMP_LOG_DEBUG("SetThreadExceptionHandlers success.");

    bool init_done = false;

    XAMP_ON_SCOPE_EXIT(
        if (init_done) {
            qJsonSettings.save();
            qAppSettings.save();
            qAppSettings.saveLogConfig();
            qGuiDb.close();
        }
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
        init_done = exist_code != -1;
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

