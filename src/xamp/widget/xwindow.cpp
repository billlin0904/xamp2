#include <QApplication>
#include <QLayout>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>
#include <QDragEnterEvent>
#include <QMimeData>

#include "thememanager.h"

#if defined(Q_OS_WIN)
#include <windowsx.h>
#include <widget/win32/win32.h>
#include <base/platfrom_handle.h>
#include <Dbt.h>
#else
#include <widget/osx/osx.h>
#endif

#include <QStorageInfo>
#include <base/logger_impl.h>

#include <version.h>
#include <widget/ui_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/xwindow.h>

#if defined(Q_OS_WIN)
static XAMP_ALWAYS_INLINE LRESULT hitTest(HWND hwnd, MSG const* msg) noexcept {
    const POINT border{
        ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
        ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
    };
    const POINT cursor{ GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };

    RECT window;
    if (!::GetWindowRect(hwnd, &window)) {
        return HTNOWHERE;
    }

    constexpr auto borderless_drag = true;
    constexpr auto borderless_resize = true;

    const auto drag = borderless_drag ? HTCAPTION : HTCLIENT;

    enum RegionMask {
        CLIENT_MASK = 0b0000,
        LEFT_MASK = 0b0001,
        RIGHT_MASK = 0b0010,
        TOP_MASK = 0b0100,
        BOTTOM_MASK = 0b1000,
    };

    const auto result =
        LEFT_MASK * (cursor.x < (window.left + border.x)) |
        RIGHT_MASK * (cursor.x >= (window.right - border.x)) |
        TOP_MASK * (cursor.y < (window.top + border.y)) |
        BOTTOM_MASK * (cursor.y >= (window.bottom - border.y));

    switch (result) {
    case LEFT_MASK: return borderless_resize ? HTLEFT : drag;
    case RIGHT_MASK: return borderless_resize ? HTRIGHT : drag;
    case TOP_MASK: return borderless_resize ? HTTOP : drag;
    case BOTTOM_MASK: return borderless_resize ? HTBOTTOM : drag;
    case TOP_MASK | LEFT_MASK: return borderless_resize ? HTTOPLEFT : drag;
    case TOP_MASK | RIGHT_MASK: return borderless_resize ? HTTOPRIGHT : drag;
    case BOTTOM_MASK | LEFT_MASK: return borderless_resize ? HTBOTTOMLEFT : drag;
    case BOTTOM_MASK | RIGHT_MASK: return borderless_resize ? HTBOTTOMRIGHT : drag;
    case CLIENT_MASK: return drag;
    default: return HTNOWHERE;
    }
}
#endif

XWindow::XWindow()
    : IXWindow()
#if defined(Q_OS_WIN)
    , current_screen_(nullptr)
#endif
	, content_widget_(nullptr) {
    setObjectName(Q_TEXT("framelessWindow"));
}

void XWindow::setContentWidget(IXampPlayer *content_widget) {
    content_widget_ = content_widget;
    if (content_widget_ != nullptr) {
        auto* default_layout = new QVBoxLayout(this);
        default_layout->addWidget(content_widget_);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }

#if defined(Q_OS_WIN)
    if (!qTheme.useNativeWindow()) {
        setWindowTitle(kAppTitle);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
        win32::setFramelessWindowStyle(winId());
        win32::addDwmShadow(winId());
        if (AppSettings::getValueAsBool(kAppSettingEnableBlur)) {
            content_widget->setAttribute(Qt::WA_TranslucentBackground, true);
            qTheme.enableBlur(this);
        }
    } else {
        win32::setWindowedWindowStyle(winId());
        win32::addDwmShadow(winId());
    }
    taskbar_.reset(new win32::WinTaskbar(this, content_widget));
#else
    if (!qTheme.useNativeWindow()) {
        if (content_widget_ != nullptr) {
            osx::hideTitleBar(content_widget_);
        }
        if (AppSettings::getValueAsBool(kAppSettingEnableBlur)) {
            setAttribute(Qt::WA_TranslucentBackground, true);
            qTheme.enableBlur(this);
        }
        setWindowTitle(kAppTitle);
    }
#endif

    setAcceptDrops(true);
    setMouseTracking(true);
    installEventFilter(this);
    readDriveInfo();
}

// QScopedPointer require default destructor.
XWindow::~XWindow() = default;

void XWindow::setTaskbarProgress(const int32_t percent) {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarProgress(percent);
#else
    (void)percent;
#endif
}

void XWindow::resetTaskbarProgress() {
#if defined(Q_OS_WIN)
    taskbar_->resetTaskbarProgress();
#endif
}

void XWindow::setTaskbarPlayingResume() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayingResume();
#endif
}

void XWindow::setTaskbarPlayerPaused() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayerPaused();
#endif
}

void XWindow::setTaskbarPlayerPlaying() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayerPlaying();
#endif
}

void XWindow::setTaskbarPlayerStop() {
#if defined(Q_OS_WIN)
    taskbar_->setTaskbarPlayerStop();
#endif
}

void XWindow::restoreGeometry() {
#if defined(Q_OS_WIN)
    if (AppSettings::contains(kAppSettingGeometry)) {
        const auto rect = AppSettings::getValue(kAppSettingGeometry).toRect();
        setGeometry(rect);
    }
    else {
        centerDesktop(this);
    }
#endif
}

bool XWindow::eventFilter(QObject * object, QEvent * event) {
    if (event->type() == QEvent::KeyPress) {
        const auto* key_event = dynamic_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Delete) {
            content_widget_->deleteKeyPress();
        }
    }
    return QWidget::eventFilter(object, event);
}

void XWindow::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void XWindow::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void XWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}

void XWindow::dropEvent(QDropEvent* event) {
    const auto* mime_data = event->mimeData();

    if (mime_data->hasUrls()) {
        for (auto const& url : mime_data->urls()) {
            content_widget_->addDropFileItem(url);
        }
        event->acceptProposedAction();
    }
}

void XWindow::readDriveInfo() {
    static const QList<std::string> kCDFileSystemType = {
        "CDFS",
        "UDF",
        "ISO-9660",
        "ISO9660"
    };

    QList<DriveInfo> drives;
    Q_FOREACH(auto & storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            auto display_name = storage.displayName() + Q_TEXT("(") + storage.rootPath() + Q_TEXT(")");
            const auto driver_letter = storage.rootPath().left(1).toStdString()[0];
            const auto file_system_type = storage.fileSystemType();
            if (kCDFileSystemType.contains(file_system_type.toUpper().toStdString())) {
                auto device = OpenCD(driver_letter);
                auto device_info = device->GetCDDeviceInfo();
                display_name += QString::fromStdWString(L" " + device_info.product);

                auto itr = std::find_if(exist_drives_.begin(), exist_drives_.end(), [display_name](auto drive) {
                    return drive.display_name == display_name;
                    });
                if (itr == exist_drives_.end()) {
                    exist_drives_.push_back(DriveInfo{ driver_letter , display_name });
                    drives.push_back(DriveInfo{ driver_letter , display_name });
                }
            }
        }
    }
    if (drives.empty()) {
        return;
    }
    if (content_widget_ != nullptr) {
        content_widget_->drivesChanges(drives);
    }
}

bool XWindow::nativeEvent(const QByteArray& event_type, void * message, long * result) {
    if (qTheme.useNativeWindow()) {
        return QWidget::nativeEvent(event_type, message, result);
    }

#if defined(Q_OS_WIN)
#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif
    const auto* msg = static_cast<MSG const*>(message);

    switch (msg->message) {
    case WM_DEVICECHANGE:
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
        {
            auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                auto lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
                if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                    readDriveInfo();
                }
            }
        }
        break;
        case DBT_DEVICEREMOVECOMPLETE:
        {
            constexpr auto firstDriveFromMask = [](ULONG unitmask) -> char {
                char i = 0;
                for (i = 0; i < 26; ++i) {
                    if (unitmask & 0x1)
                        break;
                    unitmask = unitmask >> 1;
                }
                return (i + 'A');
            };

            auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                auto lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
                if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                    auto driver_letter = firstDriveFromMask(lpdbv->dbcv_unitmask);
                    auto itr = std::find_if(exist_drives_.begin(), exist_drives_.end(), [driver_letter](auto drive) {
                        return drive.driver_letter == driver_letter;
                        });
                    if (itr != exist_drives_.end()) {
                        content_widget_->drivesRemoved(*itr);
                        exist_drives_.erase(itr);
                    }
                }
            }
        }
        break;
        }
        break;
    case WM_NCHITTEST:
        if (!isMaximized()) {
            *result = hitTest(reinterpret_cast<HWND>(winId()), msg);
            if (*result == HTCAPTION) {
                return false;
            }
            return true;
        }
        break;
    case WM_GETMINMAXINFO:
        if (layout() != nullptr) {
            if (isMaximized()) {
                RECT frame = { 0, 0, 0, 0 };
                ::AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
                frame.left = abs(frame.left);
                frame.top = abs(frame.bottom);
                layout()->setContentsMargins(frame.left, frame.top, frame.right, frame.bottom);
            }
            else {
                layout()->setContentsMargins(0, 0, 0, 0);
            }
        }            
        *result = ::DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        break;
    case WM_NCCALCSIZE:
        // this kills the window frame and title bar we added with WS_THICKFRAME and WS_CAPTION
        if (msg->wParam == FALSE) {
            // 如果 wParam 為 FALSE，則 lParam 指向一個 RECT 結構。輸入時，該結構包含建議的視窗矩形視窗。退出時，該結構應包含相應視窗客戶區的螢屏坐標。
            *result = 0;
        } else {
            // 如果 wParam 為 TRUE，lParam 指向一個 NCCALCSIZE_PARAMS 結構，該結構包含應用程式可以用來計算客戶矩形的新大小和位置的資訊。
            auto* nccalcsize_params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(msg->lParam);
            nccalcsize_params->rgrc[2] = nccalcsize_params->rgrc[1];
            nccalcsize_params->rgrc[1] = nccalcsize_params->rgrc[0];
            *result = WVR_REDRAW;
        }
        return true;
    case WM_NCUAHDRAWCAPTION:
    case WM_NCUAHDRAWFRAME:
        // These undocumented messages are sent to draw themed window
        // borders. Block them to prevent drawing borders over the client
        // area.
        *result = WM_NCPAINT;
        return true;
    case WM_NCACTIVATE:
        if (win32::compositionEnabled()) {
            *result = DefWindowProc(msg->hwnd, WM_NCACTIVATE, msg->wParam, -1);
        } else {
            if (msg->wParam == FALSE) {
                *result = TRUE;
            }
            else {
                *result = FALSE;
            }
        }
        return true;
    case WM_WINDOWPOSCHANGING:
	    {
        const auto window_pos = reinterpret_cast<LPWINDOWPOS>(msg->lParam);
        window_pos->flags |= SWP_NOCOPYBITS;
	    }
        break;
    default:
        break;
    }
    return QWidget::nativeEvent(event_type, message, result);
#else
    return QWidget::nativeEvent(event_type, message, result);
#endif
}

void XWindow::changeEvent(QEvent* event) {
#if defined(Q_OS_MAC)
    if (!qTheme.useNativeWindow() && content_widget_ != nullptr) {
        osx::hideTitleBar(content_widget_);
	}
#endif
    QWidget::changeEvent(event);
}

void XWindow::closeEvent(QCloseEvent* event) {
    if (content_widget_ != nullptr) {
        content_widget_->close();
    }
    QWidget::closeEvent(event);
}

void XWindow::mousePressEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mousePressEvent(event);
        return;
	}

    // todo: When maximize window must can be drag window.
    if (isMaximized()) {
        if (event->button() == Qt::LeftButton) {
            return;
        }
    }

    if (!content_widget_->hitTitleBar(event->pos())) {
        return;
    }
    
#if defined(Q_OS_WIN)    
    last_pos_ = event->globalPos() - pos();
#else
    QWidget::mousePressEvent(event);
#endif
}

void XWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mouseReleaseEvent(event);
        return;
	}
#if defined(Q_OS_WIN)
    last_pos_ = QPoint();
#else
    QWidget::mouseReleaseEvent(event);
#endif
}

void XWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    if (!content_widget_->hitTitleBar(event->pos())) {
        return;
    }

    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void XWindow::mouseMoveEvent(QMouseEvent* event) {
    if (qTheme.useNativeWindow()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

#if defined(Q_OS_WIN)
    if (!last_pos_.isNull()) {
        move(event->globalPos() - last_pos_);
    }

    if (!content_widget_) {
        return;
    }

    if (current_screen_ == nullptr) {
        current_screen_ = content_widget_->window()->windowHandle()->screen();
    }
    else if (current_screen_ != content_widget_->window()->windowHandle()->screen()) {
        current_screen_ = content_widget_->window()->windowHandle()->screen();

        ::SetWindowPos(reinterpret_cast<HWND>(winId()), nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }
#else
    QWidget::mouseMoveEvent(event);
#endif   
}
#ifdef Q_OS_WIN32
void XWindow::showEvent(QShowEvent* event) {
    taskbar_->showEvent();
}

void XWindow::resizeEvent(QResizeEvent* event) {
    last_size_ = frameGeometry().size();
}
#endif
