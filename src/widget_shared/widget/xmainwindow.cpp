#include <widget/xmainwindow.h>

#include <thememanager.h>

#include <base/logger_impl.h>

#include <widget/util/ui_util.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/util/str_util.h>
#include <widget/actionmap.h>
#include <widget/globalshortcut.h>

#include <QLabel>
#include <QSystemTrayIcon>
#include <QStorageInfo>
#include <QApplication>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QDragEnterEvent>
#include <QMimeData>

#if defined(Q_OS_WIN)
#include <widget/win32/wintaskbar.h>
#include <windowsx.h>
#include <Dbt.h>
#else
#endif

XMainWindow::XMainWindow()
    : IXMainWindow()
#if defined(Q_OS_WIN)
	, screen_number_(1)
#endif
	, content_widget_(nullptr) {    
    setObjectName("XMainWindow"_str);
}

void XMainWindow::setShortcut(const QKeySequence& shortcut) {
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

void XMainWindow::setContentWidget(IXFrame *content_widget) {
    content_widget_ = content_widget;
    setCentralWidget(content_widget);
    setAcceptDrops(true);
    readDriveInfo();
    ensureInitTaskbar();    
}

// QScopedPointer require default destructor.
XMainWindow::~XMainWindow() = default;

void XMainWindow::onThemeChangedFinished(ThemeColor theme_color) {
}

void XMainWindow::ensureInitTaskbar() {
#if defined(Q_OS_WIN)
    if (!task_bar_) {
        task_bar_.reset(new WinTaskbar(this));
        (void)QObject::connect(task_bar_.get(), &WinTaskbar::playClicked, [this]() {
            content_widget_->playOrPause();
            });

        (void)QObject::connect(task_bar_.get(), &WinTaskbar::pauseClicked, [this]() {
            content_widget_->playOrPause();
            });

        (void)QObject::connect(task_bar_.get(), &WinTaskbar::forwardClicked, [this]() {
            content_widget_->playNext();
            });

        (void)QObject::connect(task_bar_.get(), &WinTaskbar::backwardClicked, [this]() {
            content_widget_->playPrevious();
            });
    }
#endif
}

void XMainWindow::saveAppGeometry() {
    qAppSettings.setValue(kAppSettingGeometry, saveGeometry());
    qAppSettings.setValue(kAppSettingWindowState, isMaximized());
    qAppSettings.setValue(kAppSettingScreenNumber, screen_number_);
}

void XMainWindow::systemThemeChanged(ThemeColor theme_color) {
    if (!content_widget_) {
        return;
    }
    emit qTheme.themeChangedFinished(theme_color);
}

void XMainWindow::setTheme() {
    const auto theme = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
    qTheme.setThemeColor(theme);
}

void XMainWindow::setTaskbarProgress(const int32_t percent) {
#if defined(Q_OS_WIN)
    ensureInitTaskbar();
    task_bar_->setTaskbarProgress(percent);
#else
    (void)percent;
#endif
}

void XMainWindow::resetTaskbarProgress() {
#if defined(Q_OS_WIN)
    ensureInitTaskbar();
    task_bar_->resetTaskbarProgress();
#endif
}

void XMainWindow::setTaskbarPlayingResume() {
#if defined(Q_OS_WIN)
    ensureInitTaskbar();
    task_bar_->setTaskbarPlayingResume();
#endif
}

void XMainWindow::setTaskbarPlayerPaused() {
#if defined(Q_OS_WIN)
    ensureInitTaskbar();
    task_bar_->setTaskbarPlayerPaused();
#endif
}

void XMainWindow::setTaskbarPlayerPlaying() {
#if defined(Q_OS_WIN)
    ensureInitTaskbar();
    task_bar_->setTaskbarPlayerPlaying();
#endif
}

void XMainWindow::setTaskbarPlayerStop() {
#if defined(Q_OS_WIN)
    ensureInitTaskbar();
    task_bar_->setTaskbarPlayerStop();
#endif
}

void XMainWindow::restoreAppGeometry() {
#if defined(Q_OS_WIN)
    if (qAppSettings.contains(kAppSettingWindowState)) {
        if (qAppSettings.valueAs(kAppSettingWindowState).toBool()) {
            showMaximized();
            return;
        }
    }

    if (qAppSettings.contains(kAppSettingGeometry)) {
        screen_number_ = qAppSettings.valueAs(kAppSettingScreenNumber).toUInt();
        if (screen_number_ != 1) {
            centerDesktop(this);
        } else {
            restoreGeometry(qAppSettings.valueAs(kAppSettingGeometry).toByteArray());
        }
    }
    else {
        centerDesktop(this);
    }
#else
    centerDesktop(this);
#endif
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
            content_widget_->addDropFileItem(url);
        }
        event->acceptProposedAction();
    }
}

void XMainWindow::shortcutsPressed(uint16_t native_key, uint16_t native_mods) {
    const auto shortcut = shortcuts_.value(qMakePair(native_key, native_mods));
    if (!shortcut.isEmpty()) {
        content_widget_->shortcutsPressed(shortcut);
    }
}

void XMainWindow::drivesRemoved(char driver_letter) {
#if defined(Q_OS_WIN)
    auto itr = std::ranges::find_if(exist_drives_, [driver_letter](auto drive) {
	                                    return drive.driver_letter == driver_letter;
                                    });
    if (itr != exist_drives_.end()) {
        content_widget_->drivesRemoved(*itr);
        exist_drives_.erase(itr);
    }
#endif
}

bool XMainWindow::nativeEvent(const QByteArray& event_type, void* message, qintptr* result) {
#if defined(Q_OS_WIN)
    const auto* msg = static_cast<MSG const*>(message);
    switch (msg->message) {
    case DBT_DEVICEARRIVAL: {
	    auto* lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
        if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
	        auto lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
            if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                readDriveInfo();
            }
        }
        }
        break;
    case DBT_DEVICEREMOVECOMPLETE: {
        constexpr auto first_drive_from_mask = [](ULONG unitmask) -> char {
            char i = 0;
            for (i = 0; i < 26; ++i) {
                if (unitmask & 0x1)
                    break;
                unitmask = unitmask >> 1;
            }
            return i + 'A';
        };

        auto lpdb = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam);
        if (lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) {
            auto lpdbv = reinterpret_cast<PDEV_BROADCAST_VOLUME>(lpdb);
            if (lpdbv->dbcv_flags & DBTF_MEDIA) {
                const auto driver_letter = first_drive_from_mask(lpdbv->dbcv_unitmask);
                drivesRemoved(driver_letter);
            }
        }
        }
        break;
    case WM_HOTKEY: {
        const auto native_key = HIWORD(msg->lParam);
        const auto native_mods = LOWORD(msg->lParam);
        shortcutsPressed(native_key, native_mods);
        XAMP_LOG_DEBUG("Hot key press native_key:{} native_mods:{}", native_key, native_mods);
        }
        break;
default: ;
    }    
#endif
    return IXMainWindow::nativeEvent(event_type, message, result);
}

void XMainWindow::readDriveInfo() {
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

        auto display_name = storage.displayName() + "("_str + storage.rootPath() + ")"_str;
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
                    QString("%1:\\"_str).arg(driver_letter)
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
        content_widget_->drivesChanges(drives);
    }
#endif
}

void XMainWindow::closeEvent(QCloseEvent* event) {
    if (!content_widget_) {
        QWidget::closeEvent(event);
        return;
    }

    content_widget_->close();
    QWidget::closeEvent(event);
}

void XMainWindow::showEvent(QShowEvent* event) {
#if defined(Q_OS_WIN)
    if (!task_bar_) {
        return;
    }
    task_bar_->updateProgressIndicator();
    task_bar_->updateOverlay();
#endif
}

void XMainWindow::setIconicThumbnail(const QPixmap& image) {
#if defined(Q_OS_WIN)
    ensureInitTaskbar();
    task_bar_->setIconicThumbnail(image);
#endif
}

void XMainWindow::showWindow() {
    show();

    auto state = windowState();
    state &= ~Qt::WindowMinimized;
    state |= Qt::WindowActive;

    raise(); // for MacOS
    activateWindow(); // for Windows
}

IXFrame* XMainWindow::contentWidget() const {
    return content_widget_;
}
