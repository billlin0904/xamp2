#include <widget/widget_shared.h>
#include <widget/xmessagebox.h>
#include <widget/widget_shared_global.h>

void logAndShowMessage(const std::exception_ptr& ptr) {
    QString uiMessage;
    std::string logMessage;
    std::string stack;

    try {
        std::rethrow_exception(ptr);
    }
    catch (const PlatformException& e) {
        logMessage = e.GetErrorMessage();
        uiMessage = qFormat(e.GetErrorMessage());
		stack = e.GetStackTrace();
    }
    catch (const Exception& e) {
        logMessage = e.GetErrorMessage();
        uiMessage = QString::fromUtf8(logMessage.c_str());
        stack = e.GetStackTrace();
    }
    catch (const std::exception& e) {
        logMessage = String::LocaleStringToUTF8(e.what());
        uiMessage = QString::fromUtf8(logMessage.c_str());
        stack = StackTrace{}.CaptureStack();
    }
    catch (...) {
        logMessage = "Unknown error.";
        uiMessage = qApp->tr("Unknown error");
    }

    XAMP_LOG_DEBUG("{} {}", logMessage, stack);

    QMetaObject::invokeMethod(qApp, [msg = uiMessage]() {
        XMessageBox::showError(msg);
        }, Qt::QueuedConnection);
}
