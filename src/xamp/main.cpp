#include <thememanager.h>
#include <xapplication.h>
#include <version.h>
#include <xamp.h>

#include <iostream>
#include <QLoggingCategory>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <mimalloc.h>
#endif

#include <base/scopeguard.h>
#include <base/dll.h>
#include <base/crashhandler.h>
#include <base/platfrom_handle.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/base_sink.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/xmessagebox.h>
#include <widget/xmainwindow.h>
#include <widget/jsonsettings.h>
#include <widget/imagecache.h>
#include <widget/database.h>

#include <QPermissions>
#include <QSslSocket>
#include <QProcess>
#include <fcntl.h>

namespace {
#ifdef Q_OS_WIN
    void configureMimallocForPerformance() noexcept {
        // Favor allocation throughput over returning memory to the OS quickly.
        mi_option_set_default(mi_option_eager_commit, 1);
        // Eagerly commit even the first per-thread segment to avoid first-use stalls.
        mi_option_set_default(mi_option_eager_commit_delay, 0);
        // Keep freed pages around a bit longer so bursty UI/audio workloads can reuse them.
        mi_option_set_default(mi_option_purge_delay, 500);
        // Apply an even longer purge delay for arena memory, trading RSS for steadier latency.
        mi_option_set_default(mi_option_arena_purge_mult, 20);
        // Reset pages instead of decommitting them; this is usually faster on Windows.
        mi_option_set_enabled_default(mi_option_purge_decommits, false);
        // Let active threads reclaim memory from finished threads during later frees.
        mi_option_set_enabled_default(mi_option_abandoned_reclaim_on_free, true);
    }

    void logMimallocOptions() {
        XAMP_LOG_DEBUG(
            "mimalloc performance options: eager_commit={}, eager_commit_delay={}, purge_delay={}ms, purge_decommits={}, arena_purge_mult={}.",
            mi_option_get(mi_option_eager_commit),
            mi_option_get(mi_option_eager_commit_delay),
            mi_option_get(mi_option_purge_delay),
            mi_option_get(mi_option_purge_decommits),
            mi_option_get(mi_option_arena_purge_mult));
    }
#else
    void configureMimallocForPerformance() noexcept {
    }

    void logMimallocOptions() {
    }
#endif

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
        if (str.contains("qpnghandler.cpp"_str)) {
            return;
        }

        if (str.contains("qwindowswindow.cpp"_str)) {
            stream << QString::fromStdString(StackTrace{}.CaptureStack());
        }

        const auto logger = XAMP_LOG_CREATE_LOGGER(Qt);

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
        XampCrashHandler.SetThreadExceptionHandlers();

#ifdef Q_OS_WIN
        const auto components_path = GetComponentsFilePath();
        if (!AddSharedLibrarySearchDirectory(components_path)) {
            XAMP_LOG_ERROR("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage());
            return -1;
        }

#endif
        QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

        QLoggingCategory::setFilterRules("qt.gui.imageio.warning=false"_str);
        qputenv("QT_ICC_PROFILE", QByteArray());
    	qputenv("QT_WIN_DEBUG_CONSOLE", "attach");
        qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
        qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");

        QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        QApplication::setApplicationName(kApplicationName);
        QApplication::setApplicationVersion(kApplicationVersion);
        QApplication::setOrganizationName(kApplicationName);
        QApplication::setOrganizationDomain(kApplicationName);       

        XApplication app(argc, argv);
        /*if (!app.isAttach()) {
            XAMP_LOG_DEBUG("Application already running!");
            return -1;
        }*/        
        
        if (!QSslSocket::supportsSsl()) {
            XMessageBox::showError("SSL initialization failed."_str);
            return -1;
        }

		app.initial();
        app.loadLang();
        app.loadSampleRateConverterConfig();        
        
        qTheme.setThemeQssFile();
        
#ifdef _DEBUG
        qInstallMessageHandler(logMessageHandler);
        QLoggingCategory::setFilterRules("*.info=false"_str);
#endif        
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

        //qImageCache.loadUnknownCover();

        XAMP_LOG_DEBUG("Database init success.");

        XAMP_LOG_DEBUG("Start XAMP window...");

        XMainWindow main_window;
        //main_window.setContentWidget(nullptr);
        Xamp win(&main_window, MakeAudioPlayer());
        win.setMainWindow(&main_window);
        main_window.setContentWidget(&win);
        win.adjustSize();
        main_window.restoreAppGeometry();
        main_window.showWindow();

        logMimallocOptions();

        XAMP_LOG_DEBUG("<<<Initial XAMP window done!>>>");

        if (qAppSettings.valueAsBool(kAppSettingEnableShortcut)) {
            main_window.setShortcut(QKeySequence(Qt::Key_MediaPlay));
            main_window.setShortcut(QKeySequence(Qt::Key_MediaStop));
            main_window.setShortcut(QKeySequence(Qt::Key_MediaPrevious));
            main_window.setShortcut(QKeySequence(Qt::Key_MediaNext));
            main_window.setShortcut(QKeySequence(Qt::Key_VolumeUp));
            main_window.setShortcut(QKeySequence(Qt::Key_VolumeDown));
            main_window.setShortcut(QKeySequence(Qt::Key_VolumeMute));
            main_window.setShortcut(QKeySequence(Qt::Key_F10));
            main_window.setShortcut(QKeySequence(Qt::Key_F1));
        }
        return app.exec();
    }
}

int main() {
    configureMimallocForPerformance();

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

    std::atexit([]() {
        XampLoggerFactory.Shutdown();
        });

    static char app_name[] = "xamp2";
    static constexpr int argc = 1;
    static char* argv[] = { app_name, nullptr };

    std::ios::sync_with_stdio(false);

    QStringList args;
    auto exist_code = 0;
    try {
        exist_code = execute(argc, argv, args);
    }
    catch (const Exception& e) {
        exist_code = -1;
        XAMP_LOG_ERROR("message:{} {}", e.what(), e.GetStackTrace());
    }
	catch (const std::exception& e) {
		exist_code = -1;
		XAMP_LOG_ERROR("message:{} {}", e.what(), StackTrace{}.CaptureStack());
	}
    return exist_code;
}

