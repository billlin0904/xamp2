#include <widget/widget_shared.h>
#include <widget/xmessagebox.h>
#include <widget/widget_shared_global.h>

void logAndShowMessage(const std::exception_ptr& ptr) {
    try {
        std::rethrow_exception(ptr);
    }
    catch (const PlatformException& e) {
        XAMP_LOG_DEBUG("{} {}", e.GetErrorMessage(), StackTrace{}.CaptureStack());
        XMessageBox::showError(qFormat(e.GetErrorMessage()));
    }
    catch (const Exception& e) {
        XAMP_LOG_DEBUG("{} {}", e.GetErrorMessage(), StackTrace{}.CaptureStack());
        XMessageBox::showError(QString::fromStdString(e.GetErrorMessage()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_DEBUG("{} {}", String::LocaleStringToUTF8(e.what()), StackTrace{}.CaptureStack());
        XMessageBox::showError(QString::fromStdString(String::LocaleStringToUTF8(e.what())));
    }
    catch (...) {
        XAMP_LOG_DEBUG("Unknown error. {}", StackTrace{}.CaptureStack());
        XMessageBox::showError(qApp->tr("Unknown error"));
    }
}
