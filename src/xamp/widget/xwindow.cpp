#include <widget/xwindow.h>

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

XWindow::XWindow()
    : IXWindow()
#if defined(Q_OS_WIN)
	, screen_number_(1)
    , current_screen_(nullptr)
#endif
	, player_control_frame_(nullptr) {
    setObjectName(qTEXT("framelessWindow"));
}

void XWindow::setShortcut(const QKeySequence& shortcut) {
    constexpr auto all_mods =
        Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    const auto xkey_code = static_cast<uint>(shortcut.isEmpty() ? 0 : shortcut[0]);

    const uint key_code = QChar::toLower(xkey_code & ~all_mods);

    const auto key = static_cast<Qt::Key>(key_code);
    const auto mods = Qt::KeyboardModifiers(xkey_code & all_mods);

    const auto native_key = qGlobalShortcut.nativeKeycode(key);
    const auto native_mods = qGlobalShortcut.nativeModifiers(mods);

    if (qGlobalShortcut.registerShortcut(winId(), native_key, native_mods)) {
        shortcuts_.insert(qMakePair(native_key, native_mods), shortcut);
    }
}

void XWindow::setContentWidget(IXPlayerControlFrame *content_widget) {
    player_control_frame_ = content_widget;
    if (player_control_frame_ != nullptr) {
        auto* default_layout = new QVBoxLayout(this);
        default_layout->addWidget(player_control_frame_);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }

#if defined(Q_OS_WIN)
    if (!qTheme.useNativeWindow()) {
        setWindowTitle(kApplicationTitle);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_StyledBackground, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
        win32::setFramelessWindowStyle(winId());
        win32::addDwmShadow(winId());
        if (AppSettings::getValueAsBool(kAppSettingEnableBlur)) {
            player_control_frame_->setAttribute(Qt::WA_TranslucentBackground, true);
        }
        installEventFilter(this);
    } else {
        win32::setWindowedWindowStyle(winId());
        win32::setTitleBarColor(winId(), qTheme.themeColor());
        win32::addDwmShadow(winId());
        setMouseTracking(true);
    }
    taskbar_.reset(new win32::WinTaskbar(this, player_control_frame_));
    last_rect_ = win32::windowRect(winId());
#else
    if (!qTheme.useNativeWindow()) {
        if (player_control_frame_ != nullptr) {
            osx::hideTitleBar(player_control_frame_);
        }
        if (AppSettings::getValueAsBool(kAppSettingEnableBlur)) {
            setAttribute(Qt::WA_TranslucentBackground, true);
        }
        setWindowTitle(kApplicationTitle);
    }
#endif

    setAcceptDrops(true);

#if defined(Q_OS_WIN)
    QTimer::singleShot(2500, [this]() {
        readDriveInfo();
        });
#endif
}

// QScopedPointer require default destructor.
XWindow::~XWindow() = default;

#if defined(Q_OS_WIN)
// Ref : https://github.com/melak47/BorderlessWindow
static XAMP_ALWAYS_INLINE LRESULT hitTest(const WId window_id, MSG const* msg) noexcept {
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

bool XWindow::nativeEvent(const QByteArray& event_type, void* message, long* result) {
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
                return i + 'A';
            };

            const auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
            if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                const auto lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
                if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                    const auto driver_letter = firstDriveFromMask(lpdbv->dbcv_unitmask);
                    drivesRemoved(driver_letter);
                }
            }
        }
        break;
        }
        break;
    case WM_NCHITTEST:
        if (!isMaximized()) {
            *result = hitTest(winId(), msg);
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
        if (win32::compositionEnabled()) {
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
            systemThemeChanged(win32::isDarkModeAppEnabled() ? ThemeColor::DARK_THEME : ThemeColor::LIGHT_THEME);
        }
        break;
    case WM_HOTKEY:
    {
        const auto native_key = HIWORD(msg->lParam);
        const auto native_mods = LOWORD(msg->lParam);
        shortcutsPressed(native_key, native_mods);
        XAMP_LOG_DEBUG("Hot key press native_key:{} native_mods:{}", native_key, native_mods);
    }
    break;
    default:
        break;
    }
    return QWidget::nativeEvent(event_type, message, result);
}
#else
bool XWindow::nativeEvent(const QByteArray& event_type, void* message, long* result) {
    return QWidget::nativeEvent(event_type, message, result);
}
#endif

void XWindow::saveGeometry() {
#if defined(Q_OS_WIN) 
    AppSettings::setValue(kAppSettingGeometry, win32::windowRect(winId()));
    AppSettings::setValue(kAppSettingWindowState, isMaximized());
    AppSettings::setValue(kAppSettingScreenNumber, screen_number_);
    XAMP_LOG_INFO("restoreGeometry: ({}, {}, {}, {})", last_rect_.x(), last_rect_.y(), last_rect_.width(), last_rect_.height());
    XAMP_LOG_INFO("Screen number: {}", screen_number_);
#endif
}

void XWindow::systemThemeChanged(ThemeColor theme_color) {
    if (!player_control_frame_) {
        return;
    }
    player_control_frame_->systemThemeChanged(theme_color);
}

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
    if (AppSettings::contains(kAppSettingWindowState)) {
        if (AppSettings::getValue(kAppSettingWindowState).toBool()) {
            showMaximized();
            return;
        }
    }

    if (AppSettings::contains(kAppSettingGeometry)) {
        last_rect_ = AppSettings::getValue(kAppSettingGeometry).toRect();
        screen_number_ = AppSettings::getValue(kAppSettingScreenNumber).toUInt();
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
        centerDesktop(this);
    }    
#endif
}

bool XWindow::eventFilter(QObject * object, QEvent * event) {
    if (!player_control_frame_) {
        return QWidget::eventFilter(object, event);
    }

    if (event->type() == QEvent::KeyPress) {
        const auto* key_event = dynamic_cast<QKeyEvent*>(event);
        if (key_event->key() == Qt::Key_Delete) {
            player_control_frame_->deleteKeyPress();
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

void XWindow::focusInEvent(QFocusEvent* event) {
    if (!player_control_frame_) {
        return;
    }
    player_control_frame_->focusIn();
}

void XWindow::focusOutEvent(QFocusEvent* event) {
    if (!player_control_frame_) {
        return;
    }
    player_control_frame_->focusOut();
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
    if (!player_control_frame_) {
        return;
    }

    const auto* mime_data = event->mimeData();

    if (mime_data->hasUrls()) {
        for (auto const& url : mime_data->urls()) {
            player_control_frame_->addDropFileItem(url);
        }
        event->acceptProposedAction();
    }
}

void XWindow::shortcutsPressed(uint16_t native_key, uint16_t native_mods) {
    const auto shortcut = shortcuts_.value(qMakePair(native_key, native_mods));
    if (!shortcut.isEmpty()) {
        player_control_frame_->shortcutsPressed(shortcut);
    }
}

void XWindow::drivesRemoved(char driver_letter) {
#if defined(Q_OS_WIN)
    auto itr = std::find_if(exist_drives_.begin(),
        exist_drives_.end(), [driver_letter](auto drive) {
            return drive.driver_letter == driver_letter;
        });
    if (itr != exist_drives_.end()) {
        player_control_frame_->drivesRemoved(*itr);
        exist_drives_.erase(itr);
    }
#endif
}

void XWindow::readDriveInfo() {
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
    if (player_control_frame_ != nullptr) {
        player_control_frame_->drivesChanges(drives);
    }
#endif
}

void XWindow::changeEvent(QEvent* event) {
#if defined(Q_OS_MAC)
    if (!qTheme.useNativeWindow() && player_control_frame_ != nullptr) {
        osx::hideTitleBar(player_control_frame_);
	}
#endif
	QFrame::changeEvent(event);
}

void XWindow::closeEvent(QCloseEvent* event) {
    if (!player_control_frame_) {
        QWidget::closeEvent(event);
        return;
    }

    player_control_frame_->close();
    QWidget::closeEvent(event);
}

void XWindow::mousePressEvent(QMouseEvent* event) {
    if (!player_control_frame_) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (!player_control_frame_->hitTitleBar(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }

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
    
#if defined(Q_OS_WIN)    
    last_pos_ = event->globalPos() - pos();
#else
    QWidget::mousePressEvent(event);
#endif
}

void XWindow::mouseReleaseEvent(QMouseEvent* event) {
#if defined(Q_OS_WIN)
    last_pos_ = QPoint();
#else
    QWidget::mouseReleaseEvent(event);
#endif
}

void XWindow::initMaximumState() {
    if (!player_control_frame_) {
        return;
    }
    player_control_frame_->updateMaximumState(isMaximized());
}

void XWindow::updateMaximumState() {
    if (!player_control_frame_) {
        return;
    }

    if (isMaximized()) {
        showNormal();
        player_control_frame_->updateMaximumState(false);
    }
    else {
        showMaximized();
        player_control_frame_->updateMaximumState(true);
    }
}

void XWindow::mouseDoubleClickEvent(QMouseEvent* event) {
    if (!player_control_frame_) {
        return;
    }

    if (!player_control_frame_->hitTitleBar(event->pos())) {
        return;
    }

    updateMaximumState();
}

#ifdef Q_OS_WIN
void XWindow::addSystemMenu(QWidget* widget) {
    widget->setContextMenuPolicy(Qt::CustomContextMenu);

    (void)QObject::connect(widget, &QFrame::customContextMenuRequested, [this, widget](auto pt) {
        ActionMap<QWidget> action_map(widget);
        auto* restore_act = action_map.addAction(tr("Restore(R)"));
        action_map.setCallback(restore_act, [this]() {
            showNormal();
            });
        restore_act->setIcon(qTheme.fontIcon(Glyphs::ICON_RESTORE_WINDOW));
        restore_act->setEnabled(isMaximized());

        auto* move_act = action_map.addAction(tr("Move(M)"));
        move_act->setEnabled(true);

        auto* size_act = action_map.addAction(tr("Size(S)"));
        size_act->setEnabled(true);

        auto* mini_act = action_map.addAction(tr("Minimize(N)"));
        action_map.setCallback(mini_act, [this]() {
            showMinimized();
            });
        mini_act->setIcon(qTheme.fontIcon(Glyphs::ICON_MINIMIZE_WINDOW));
        mini_act->setEnabled(true);

        auto* max_act = action_map.addAction(tr("Maximum(M)"));
        action_map.setCallback(max_act, [this]() {
            showMaximized();
            });
        max_act->setEnabled(!isMaximized());
        max_act->setIcon(qTheme.fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
        action_map.addSeparator();

        auto* close_act = action_map.addAction(tr("Close(X)"));
        action_map.setCallback(close_act, [this]() {
            close();
            });
        close_act->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW));

        action_map.exec(pt);
        });
}
#endif

void XWindow::setTitleBarAction(QFrame* title_bar) {
#ifdef Q_OS_WIN
    addSystemMenu(title_bar);
#endif
}

void XWindow::mouseMoveEvent(QMouseEvent* event) {
    if (!player_control_frame_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

#if defined(Q_OS_WIN)
    if (!last_pos_.isNull()) {
        move(event->globalPos() - last_pos_);
    }

    last_rect_ = win32::windowRect(winId());

    if (current_screen_ == nullptr) {
        current_screen_ = player_control_frame_->window()->windowHandle()->screen();
    }
    else if (current_screen_ != player_control_frame_->window()->windowHandle()->screen()) {
        current_screen_ = player_control_frame_->window()->windowHandle()->screen();

        ::SetWindowPos(reinterpret_cast<HWND>(winId()), nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }

    updateScreenNumber();

#else
    QWidget::mouseMoveEvent(event);
#endif   
}

void XWindow::updateScreenNumber() {
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

#ifdef Q_OS_WIN32
void XWindow::showEvent(QShowEvent* event) {
    taskbar_->showEvent();
}
#endif
