#include <widget/xmainwindow.h>

#include "thememanager.h"

#include <version.h>
#include <widget/ui_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/actionmap.h>
#include <widget/globalshortcut.h>

#include <QStorageInfo>
#include <QApplication>
#include <QLayout>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QScreen>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#include <base/platfrom_handle.h>
#include <windowsx.h>
#include <Dbt.h>
#else
#include <Carbon/Carbon.h>
#include <widget/osx/osx.h>
#endif

#include <QTimer>
#include <base/logger_impl.h>

XMainWindow::XMainWindow()
    : IXMainWindow()
#if defined(Q_OS_WIN)
	, screen_number_(1)
    , current_screen_(nullptr)
#endif
	, content_widget_(nullptr) {
    setObjectName(qTEXT("framelessWindow"));
}

void XMainWindow::setShortcut(const QKeySequence& shortcut) {
    constexpr auto all_mods =
        Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    const auto xkey_code = static_cast<uint>(shortcut.isEmpty() ? 0 : shortcut[0]);

    const uint key_code = QChar::toLower(xkey_code & ~all_mods);

    const auto key = static_cast<Qt::Key>(key_code);
    const auto mods = Qt::KeyboardModifiers(xkey_code & all_mods);

    const auto native_key = qGlobalShortcut.NativeKeycode(key);
    const auto native_mods = qGlobalShortcut.NativeModifiers(mods);

    if (qGlobalShortcut.RegisterShortcut(winId(), native_key, native_mods)) {
        shortcuts_.insert(qMakePair(native_key, native_mods), shortcut);
    }
}

void XMainWindow::SetContentWidget(IXFrame *content_widget) {
    content_widget_ = content_widget;
    if (content_widget_ != nullptr) {
        auto* default_layout = new QVBoxLayout(this);
        default_layout->addWidget(content_widget_);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }

#if defined(Q_OS_WIN)
    if (!qTheme.UseNativeWindow()) {
        setWindowTitle(kApplicationTitle);        
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowMaximizeButtonHint);
        win32::SetFramelessWindowStyle(winId());
        win32::AddDwmShadow(winId());
        if (AppSettings::ValueAsBool(kAppSettingEnableBlur)) {
            content_widget_->setAttribute(Qt::WA_TranslucentBackground, true);
        }
        installEventFilter(this);
        win32::SetAccentPolicy(winId());
    } else {
        win32::SetWindowedWindowStyle(winId());
        win32::AddDwmShadow(winId());
        setMouseTracking(true);
    }
    task_bar_.reset(new win32::WinTaskbar(this, content_widget_));
    last_rect_ = win32::GetWindowRect(winId());
#else
    if (!qTheme.UseNativeWindow()) {
        if (content_widget_ != nullptr) {
            osx::hideTitleBar(content_widget_);
        }
        if (AppSettings::ValueAsBool(kAppSettingEnableBlur)) {
            setAttribute(Qt::WA_TranslucentBackground, true);
        }
        setWindowTitle(kApplicationTitle);
    }
#endif

    setAcceptDrops(true);

#if defined(Q_OS_WIN)
    QTimer::singleShot(2500, [this]() {
        ReadDriveInfo();
        });
#endif

    win32::IsValidMutexName("285d604e-5bdb-7900-81f1-ad9fd2bad315", "guest");
}

// QScopedPointer require default destructor.
XMainWindow::~XMainWindow() {
    XAMP_LOG_DEBUG("XMainWindow destory!");
}

#if defined(Q_OS_WIN)
// Ref : https://github.com/melak47/BorderlessWindow
static XAMP_ALWAYS_INLINE LRESULT hitTest(const WId window_id, MSG const* msg, const IXFrame* content_widget) noexcept {
    auto hwnd = reinterpret_cast<HWND>(window_id);

    const POINT border{
        ::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
        ::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
    };
    const POINT cursor{ GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };

    RECT window;
    if (!::GetWindowRect(hwnd, &window)) {
        return HTNOWHERE;
    }

    constexpr auto borderless_resize = true;

    auto drag = HTCLIENT;
    if (content_widget->HitTitleBar(QPoint(cursor.x, cursor.y))) {
        drag = HTCAPTION;
    }

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
    case CLIENT_MASK:
        return drag;
    default: 
        return HTNOWHERE;
    }
}

bool XMainWindow::nativeEvent(const QByteArray& event_type, void* message, long* result) {
#define WM_NCUAHDRAWCAPTION (0x00AE)
#define WM_NCUAHDRAWFRAME (0x00AF)
    const auto* msg = static_cast<MSG const*>(message);

    switch (msg->message) {
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    {
        Qt::FocusReason reason;
        if (::GetKeyState(VK_LBUTTON) < 0 || ::GetKeyState(VK_RBUTTON) < 0)
            reason = Qt::MouseFocusReason;
        else if (::GetKeyState(VK_SHIFT) < 0)
            reason = Qt::BacktabFocusReason;
        else
            reason = Qt::TabFocusReason;
        if (msg->message == WM_SETFOCUS) {
            QFocusEvent e(QEvent::FocusIn, reason);
            QApplication::sendEvent(this, &e);
        }
        else {
            QFocusEvent e(QEvent::FocusOut, reason);
            QApplication::sendEvent(this, &e);
        }
    }
    break;
    case WM_DEVICECHANGE:
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
        {
            auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                auto lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
                if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                    ReadDriveInfo();
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
                return i + 'A';
            };

            const auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                const auto lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
                if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                    const auto driver_letter = firstDriveFromMask(lpdbv->dbcv_unitmask);
                    DrivesRemoved(driver_letter);
                }
            }
        }
        break;
        }
        break;
    case WM_NCHITTEST:
        if (!isMaximized()) {
            *result = hitTest(winId(), msg, content_widget_);
            if (*result == HTNOWHERE) {
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
        }
        else {
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
        if (win32::IsCompositionEnabled()) {
            *result = DefWindowProc(msg->hwnd, WM_NCACTIVATE, msg->wParam, -1);
        }
        else {
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
    case WM_WININICHANGE:
        if (!lstrcmp(reinterpret_cast<LPCTSTR>(msg->lParam), L"ImmersiveColorSet")) {
            SystemThemeChanged(win32::IsDarkModeAppEnabled() ? ThemeColor::DARK_THEME : ThemeColor::LIGHT_THEME);
        }
        break;
    case WM_HOTKEY:
    {
        const auto native_key = HIWORD(msg->lParam);
        const auto native_mods = LOWORD(msg->lParam);
        ShortcutsPressed(native_key, native_mods);
        XAMP_LOG_DEBUG("Hot key press native_key:{} native_mods:{}", native_key, native_mods);
    }
    break;
    default:
        break;
    }
    return QWidget::nativeEvent(event_type, message, result);
}
#else
bool XMainWindow::nativeEvent(const QByteArray& event_type, void* message, long* result) {
    return QWidget::nativeEvent(event_type, message, result);
}
#endif

void XMainWindow::SaveGeometry() {
#if defined(Q_OS_WIN) 
    AppSettings::SetValue(kAppSettingGeometry, win32::GetWindowRect(winId()));
    AppSettings::SetValue(kAppSettingWindowState, isMaximized());
    AppSettings::SetValue(kAppSettingScreenNumber, screen_number_);
    XAMP_LOG_INFO("restoreGeometry: ({}, {}, {}, {})", last_rect_.x(), last_rect_.y(), last_rect_.width(), last_rect_.height());
    XAMP_LOG_INFO("Screen number: {}", screen_number_);
#endif
}

void XMainWindow::SystemThemeChanged(ThemeColor theme_color) {
    if (!content_widget_) {
        return;
    }
    content_widget_->SystemThemeChanged(theme_color);
}

void XMainWindow::SetTaskbarProgress(const int32_t percent) {
#if defined(Q_OS_WIN)
    task_bar_->SetTaskbarProgress(percent);
#else
    (void)percent;
#endif
}

void XMainWindow::ResetTaskbarProgress() {
#if defined(Q_OS_WIN)
    task_bar_->ResetTaskbarProgress();
#endif
}

void XMainWindow::SetTaskbarPlayingResume() {
#if defined(Q_OS_WIN)
    task_bar_->SetTaskbarPlayingResume();
#endif
}

void XMainWindow::SetTaskbarPlayerPaused() {
#if defined(Q_OS_WIN)
    task_bar_->SetTaskbarPlayerPaused();
#endif
}

void XMainWindow::SetTaskbarPlayerPlaying() {
#if defined(Q_OS_WIN)
    task_bar_->SetTaskbarPlayerPlaying();
#endif
}

void XMainWindow::SetTaskbarPlayerStop() {
#if defined(Q_OS_WIN)
    task_bar_->SetTaskbarPlayerStop();
#endif
}

void XMainWindow::RestoreGeometry() {
#if defined(Q_OS_WIN)
    if (AppSettings::contains(kAppSettingWindowState)) {
        if (AppSettings::GetValue(kAppSettingWindowState).toBool()) {
            showMaximized();
            return;
        }
    }

    if (AppSettings::contains(kAppSettingGeometry)) {
        last_rect_ = AppSettings::GetValue(kAppSettingGeometry).toRect();
        screen_number_ = AppSettings::GetValue(kAppSettingScreenNumber).toUInt();
        if (screen_number_ != 1) {
            if (QGuiApplication::screens().size() <= screen_number_) {
                const auto screen_index = screen_number_ - 1;
                const auto screens = QGuiApplication::screens();
                if (screens.size() < screen_index) {
	                const auto screenres = screens.at(screen_index)->availableGeometry();
                    move(QPoint(screenres.x() + 100, screenres.y() + 100));
                    resize(last_rect_.width(), last_rect_.height());
                }                
            }
        } else {
            setGeometry(last_rect_);
        }
        XAMP_LOG_DEBUG("restoreGeometry: ({}, {}, {}, {})",
            last_rect_.x(), last_rect_.y(), last_rect_.width(), last_rect_.height());
        XAMP_LOG_DEBUG("Screen number: {}", screen_number_);
    }
    else {
        CenterDesktop(this);
    }    
#endif
}

bool XMainWindow::eventFilter(QObject * object, QEvent * event) {
    if (!content_widget_) {
        return QWidget::eventFilter(object, event);
    }

    if (event->type() == QEvent::KeyPress) {
        const auto* key_event = dynamic_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Delete) {
            content_widget_->DeleteKeyPress();
        }
    } else {
        if (event->type() == QEvent::FocusIn) {
            focusInEvent(static_cast<QFocusEvent*>(event));
        } else if (event->type() == QEvent::FocusOut) {
            focusOutEvent(static_cast<QFocusEvent*>(event));
        }
    }
    return QWidget::eventFilter(object, event);
}

void XMainWindow::focusInEvent(QFocusEvent* event) {
    if (!content_widget_) {
        return;
    }
    content_widget_->FocusIn();
}

void XMainWindow::focusOutEvent(QFocusEvent* event) {
    if (!content_widget_) {
        return;
    }
    content_widget_->FocusOut();
}

void XMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void XMainWindow::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void XMainWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}

void XMainWindow::dropEvent(QDropEvent* event) {
    if (!content_widget_) {
        return;
    }

    const auto* mime_data = event->mimeData();

    if (mime_data->hasUrls()) {
        for (auto const& url : mime_data->urls()) {
            content_widget_->AddDropFileItem(url);
        }
        event->acceptProposedAction();
    }
}

void XMainWindow::ShortcutsPressed(uint16_t native_key, uint16_t native_mods) {
    const auto shortcut = shortcuts_.value(qMakePair(native_key, native_mods));
    if (!shortcut.isEmpty()) {
        content_widget_->ShortcutsPressed(shortcut);
    }
}

void XMainWindow::DrivesRemoved(char driver_letter) {
#if defined(Q_OS_WIN)
    auto itr = std::find_if(exist_drives_.begin(),
        exist_drives_.end(), [driver_letter](auto drive) {
            return drive.driver_letter == driver_letter;
        });
    if (itr != exist_drives_.end()) {
        content_widget_->DrivesRemoved(*itr);
        exist_drives_.erase(itr);
    }
#endif
}

void XMainWindow::ReadDriveInfo() {
#if defined(Q_OS_WIN)
    static const QSet<QByteArray> kCDFileSystemType = {
        "CDFS",
        "UDF",
        "ISO-9660",
        "ISO9660"
    };

    QList<DriveInfo> drives;
    Q_FOREACH(auto & storage, QStorageInfo::mountedVolumes()) {
        if (!storage.isValid() || !storage.isReady()) {
            return;
        }

        auto display_name = storage.displayName() + qTEXT("(") + storage.rootPath() + qTEXT(")");
        const auto driver_letter = storage.rootPath().left(1).toStdString()[0];
        const auto file_system_type = storage.fileSystemType();
        if (kCDFileSystemType.contains(file_system_type.toUpper())) {
            const auto device = OpenCD(driver_letter);
            const auto device_info = device->GetCDDeviceInfo();
            display_name += QString::fromStdWString(L" " + device_info.product);
            auto itr = exist_drives_.find(display_name);
            if (itr == exist_drives_.end()) {
                XAMP_LOG_DEBUG("Add new drive : {}", display_name.toStdString());
                DriveInfo info{
                   driver_letter ,
                   display_name,
                    QString(qTEXT("%1:\\")).arg(driver_letter)
                };
                exist_drives_[display_name] = info;
                drives.push_back(info);
            }
        }
    }
    if (drives.empty()) {
        return;
    }
    if (content_widget_ != nullptr) {
        content_widget_->DrivesChanges(drives);
    }
#endif
}

void XMainWindow::changeEvent(QEvent* event) {
#if defined(Q_OS_MAC)
    if (!qTheme.UseNativeWindow() && content_widget_ != nullptr) {
        osx::hideTitleBar(content_widget_);
	}
#endif
	QFrame::changeEvent(event);
}

void XMainWindow::closeEvent(QCloseEvent* event) {
    if (!content_widget_) {
        QWidget::closeEvent(event);
        return;
    }

    content_widget_->close();
    QWidget::closeEvent(event);
}

void XMainWindow::mousePressEvent(QMouseEvent* event) {
    if (!content_widget_) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (!content_widget_->HitTitleBar(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }

#if 0
    // todo: When maximize window must can be drag window.
    if (isMaximized()) {
        if (event->button() == Qt::LeftButton) {
            setWindowState(windowState() & ~(Qt::WindowMinimized
                | Qt::WindowMaximized
                | Qt::WindowFullScreen));
            resize(last_rect_.size());
            last_pos_ = event->globalPos() - pos();
            move(last_pos_);
            return;
        }
    }
#else
    if (isMaximized()) {
        return;
    }
#endif
    
#if defined(Q_OS_WIN)    
    last_pos_ = event->globalPos() - pos();
#else
    QWidget::mousePressEvent(event);
#endif
}

void XMainWindow::mouseReleaseEvent(QMouseEvent* event) {
#if defined(Q_OS_WIN)
    last_pos_ = QPoint();
#else
    QWidget::mouseReleaseEvent(event);
#endif
}

void XMainWindow::InitMaximumState() {
    if (!content_widget_) {
        return;
    }
    content_widget_->UpdateMaximumState(isMaximized());
}

void XMainWindow::UpdateMaximumState() {
    if (!content_widget_) {
        return;
    }

    if (isMaximized()) {
        showNormal();
        content_widget_->UpdateMaximumState(false);
    }
    else {
        showMaximized();
        content_widget_->UpdateMaximumState(true);
    }
}

void XMainWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    if (!content_widget_) {
        return;
    }

    if (!content_widget_->HitTitleBar(event->pos())) {
        return;
    }

    UpdateMaximumState();
}

#ifdef Q_OS_WIN
void XMainWindow::AddSystemMenu(QWidget* widget) {
    widget->setContextMenuPolicy(Qt::CustomContextMenu);

    (void)QObject::connect(widget, &QFrame::customContextMenuRequested, [this, widget](auto pt) {
        ActionMap<QWidget> action_map(widget);
        auto* restore_act = action_map.AddAction(tr("Restore(R)"));
        action_map.SetCallback(restore_act, [this]() {
            showNormal();
            });
        restore_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_RESTORE_WINDOW));
        restore_act->setEnabled(isMaximized());

        auto* move_act = action_map.AddAction(tr("Move(M)"));
        move_act->setEnabled(true);

        auto* size_act = action_map.AddAction(tr("Size(S)"));
        size_act->setEnabled(true);

        auto* mini_act = action_map.AddAction(tr("Minimize(N)"));
        action_map.SetCallback(mini_act, [this]() {
            showMinimized();
            });
        mini_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_MINIMIZE_WINDOW));
        mini_act->setEnabled(true);

        auto* max_act = action_map.AddAction(tr("Maximum(M)"));
        action_map.SetCallback(max_act, [this]() {
            showMaximized();
            });
        max_act->setEnabled(!isMaximized());
        max_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
        action_map.AddSeparator();

        auto* close_act = action_map.AddAction(tr("Close(X)"));
        action_map.SetCallback(close_act, [this]() {
            close();
            });
        close_act->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CLOSE_WINDOW));

        action_map.exec(pt);
        });
}
#endif

void XMainWindow::SetTitleBarAction(QFrame* title_bar) {
#ifdef Q_OS_WIN
    AddSystemMenu(title_bar);
#endif
}

void XMainWindow::mouseMoveEvent(QMouseEvent* event) {
    if (!content_widget_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

#if defined(Q_OS_WIN)
    if (!last_pos_.isNull()) {
        move(event->globalPos() - last_pos_);
    }

    last_rect_ = win32::GetWindowRect(winId());

    if (current_screen_ == nullptr) {
        current_screen_ = content_widget_->window()->windowHandle()->screen();
    }
    else if (current_screen_ != content_widget_->window()->windowHandle()->screen()) {
        current_screen_ = content_widget_->window()->windowHandle()->screen();

        ::SetWindowPos(reinterpret_cast<HWND>(winId()), nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }

    UpdateScreenNumber();

#else
    QWidget::mouseMoveEvent(event);
#endif   
}

void XMainWindow::UpdateScreenNumber() {
#ifdef Q_OS_WIN32
    auto i = 1;
    Q_FOREACH(auto screen, QGuiApplication::screens()) {
        if (screen == current_screen_) {
            screen_number_ = i;
        }
        ++i;
    }
#else
#endif
    XAMP_LOG_TRACE("screen_number_: {}", screen_number_);
}

void XMainWindow::showEvent(QShowEvent* event) {
#ifdef Q_OS_WIN32
    task_bar_->showEvent();
#endif
    setAttribute(Qt::WA_Mapped);
    QFrame::showEvent(event);
}

void XMainWindow::ShowWindow() {
    show();
    raise(); // for MacOS
    activateWindow(); // for Windows
}
