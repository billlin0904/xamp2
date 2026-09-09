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
#include <base/zib_util.h>

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

#include <QSslSocket>
#include <QProcess>
#include <fcntl.h>

namespace {

#ifndef Q_OS_WIN
    class QDebugSink : public spdlog::sinks::base_sink<LoggerMutex> {
    public:
        void sink_it_(const spdlog::details::log_msg& msg) override {
            spdlog::memory_buf_t formatted;
            formatter_->format(msg, formatted);

            std::cerr << fmt::to_string(formatted);
        }

        void flush_() override {
            std::cerr.flush();
        }
    };
#endif

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

        stream << QString::fromStdString(get_file_name()) << ":" << context.line << " (" << QString::fromStdString(getLastErrorMessage()) << ") \r\n"
            << context.function << ": " << msg;
        if (!disable_stack_trace) {
            stream << QString::fromStdString(StackTrace{}.captureStack());
        }

        // Skip PNG image error
        if (str.contains("qpnghandler.cpp"_str)) {
            return;
        }

        if (str.contains("qwindowswindow.cpp"_str)) {
            stream << QString::fromStdString(StackTrace{}.captureStack());
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

    int execute(int argc, char* argv[], QStringList &args) {
#ifdef Q_OS_WIN       
        const auto components_path = getComponentsFilePath();
        if (!addSharedLibrarySearchDirectory(components_path)) {
            XAMP_LOG_ERROR("addSharedLibrarySearchDirectory return fail! ({})", getLastErrorMessage());
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
        app.initial();

        QGuiApplication::setDesktopFileName("xamp"_str);
        QApplication::setWindowIcon(qTheme.applicationIcon());

        if (!QSslSocket::supportsSsl()) {
            XMessageBox::showError("SSL initialization failed."_str);
            return -1;
        }
		
        app.loadLang();
        app.loadSampleRateConverterConfig();        
        qTheme.setThemeQssFile();
        
        qInstallMessageHandler(logMessageHandler);
        QLoggingCategory::setFilterRules("*.info=false"_str);

        try {            
            loadComponentSharedLibrary();
        }
        catch (const Exception& e) {
            XMessageBox::showBug(e);
            return -1;
        }
        catch (...) {
            return -1;
        }

        XAMP_LOG_DEBUG("load component shared library success.");
        XAMP_LOG_DEBUG("Database start initial...");

        try {			
            qGuiDb.open();
        }
        catch (const Exception& e) {
            XMessageBox::showBug(e);
            return -1;
        }

        XAMP_LOG_DEBUG("Database init success.");
        XAMP_LOG_DEBUG("start XAMP window...");

        XMainWindow main_window;
        //main_window.setContentWidget(nullptr);
        Xamp win(&main_window, makeAudioPlayer());
        win.setMainWindow(&main_window);
        main_window.setContentWidget(&win);
        win.adjustSize();
        main_window.restoreAppGeometry();
        main_window.showWindow();

        win.setupSystemMenu();

        XAMP_LOG_DEBUG("<<<initial XAMP window done!>>>");

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

#ifdef Q_OS_WIN 
        setProcessMitigation();
#endif
        return app.exec();
    }
}

int main() {
    try {
        XampLoggerFactory
            .addDebugOutput()
#if !defined(Q_OS_WIN) && defined(_DEBUG)
            .addSink(std::make_shared<QDebugSink>())
#endif
            .addLogFile("xamp.log")
            .startup();        
    }
    catch (const std::exception& e) {
        return -1;
    }

    XampCrashHandler.setProcessExceptionHandlers();
    XampCrashHandler.setThreadExceptionHandlers();

    std::atexit([]() {
        unloadComponentSharedLibrary();
        XAMP_LOG_DEBUG("<<<shutdown XAMP logger>>>");
        XampLoggerFactory.shutdown();
        });

    static char app_name[] = "xamp";
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
        XAMP_LOG_ERROR("message:{} {}", e.what(), e.getStackTrace());
    }
	catch (const std::exception& e) {
		exist_code = -1;
		XAMP_LOG_ERROR("message:{} {}", e.what(), StackTrace{}.captureStack());
	}
    return exist_code;
}
