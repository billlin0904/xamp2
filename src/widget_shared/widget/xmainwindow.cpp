#include <widget/xmainwindow.h>

#include <thememanager.h>

#include <FramelessHelper/Widgets/framelesswidgetshelper.h>
#include <base/logger_impl.h>

#include <widget/util/ui_util.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/util/str_util.h>
#include <widget/actionmap.h>
#include <widget/globalshortcut.h>
#include <thememanager.h>

#include <QLabel>
#include <QSystemTrayIcon>
#include <QStorageInfo>
#include <QApplication>
#include <QLayout>
#include <QPainter>
#include <QPainterPath>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTimer>

#if defined(Q_OS_WIN)
#include <widget/win32/wintaskbar.h>
#include <base/platfrom_handle.h>
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
    setObjectName(qTEXT("XMainWindow"));   
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
#if defined(Q_OS_WIN)
    content_widget_ = content_widget;
    if (content_widget_ != nullptr) {
        auto* default_layout = new QVBoxLayout(this);

        title_frame_ = new QFrame();
        title_frame_->setObjectName(QString::fromUtf8("titleFrame"));
        title_frame_->setMinimumSize(QSize(0, kMaxTitleHeight));
        title_frame_->setFrameShape(QFrame::NoFrame);
        title_frame_->setFrameShadow(QFrame::Plain);
        title_frame_->setStyleSheet(qTEXT("border-radius: 0px;"));

        auto f = font();
        f.setBold(true);
        f.setPointSize(qTheme.fontSize(kTitleFontSize));
        title_frame_label_ = new QLabel(title_frame_);
        title_frame_label_->setObjectName(QString::fromUtf8("titleFrameLabel"));
        QSizePolicy size_policy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
        size_policy3.setHorizontalStretch(1);
        size_policy3.setVerticalStretch(0);
        size_policy3.setHeightForWidth(title_frame_label_->sizePolicy().hasHeightForWidth());
        title_frame_label_->setFont(f);
        title_frame_label_->setSizePolicy(size_policy3);
        title_frame_label_->setAlignment(Qt::AlignCenter);

        close_button_ = new QToolButton(title_frame_);
        close_button_->setObjectName(QString::fromUtf8("closeButton"));
        close_button_->setMinimumSize(QSize(kMaxTitleHeight + 20, kMaxTitleHeight));
        close_button_->setMaximumSize(QSize(kMaxTitleHeight + 20, kMaxTitleHeight));
        close_button_->setFocusPolicy(Qt::NoFocus);

        max_win_button_ = new QToolButton(title_frame_);
        max_win_button_->setObjectName(QString::fromUtf8("maxWinButton"));
        max_win_button_->setMinimumSize(QSize(kMaxTitleHeight + 20, kMaxTitleHeight));
        max_win_button_->setMaximumSize(QSize(kMaxTitleHeight + 20, kMaxTitleHeight));
        max_win_button_->setFocusPolicy(Qt::NoFocus);

        min_win_button_ = new QToolButton(title_frame_);
        min_win_button_->setObjectName(QString::fromUtf8("minWinButton"));
        min_win_button_->setMinimumSize(QSize(kMaxTitleHeight + 20, kMaxTitleHeight));
        min_win_button_->setMaximumSize(QSize(kMaxTitleHeight + 20, kMaxTitleHeight));
        min_win_button_->setFocusPolicy(Qt::NoFocus);
        min_win_button_->setPopupMode(QToolButton::InstantPopup);

        icon_ = new QToolButton(title_frame_);
        icon_->setObjectName(QString::fromUtf8("minWinButton"));
        icon_->setMinimumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
        icon_->setMaximumSize(QSize(kMaxTitleHeight, kMaxTitleHeight));
        icon_->setIconSize(QSize(kMaxTitleIcon, kMaxTitleIcon));
        icon_->setFocusPolicy(Qt::NoFocus);
        icon_->setStyleSheet(qTEXT("background: transparent; border: none;"));

        auto* horizontal_spacer = new QSpacerItem(kMaxTitleHeight, kMaxTitleIcon, QSizePolicy::Expanding, QSizePolicy::Minimum);

        auto* horizontal_layout = new QHBoxLayout(title_frame_);
        horizontal_layout->addWidget(icon_);
        horizontal_layout->addItem(horizontal_spacer);
        horizontal_layout->addWidget(title_frame_label_);
        horizontal_layout->addWidget(min_win_button_);
        horizontal_layout->addWidget(max_win_button_);
        horizontal_layout->addWidget(close_button_);

        horizontal_layout->setSpacing(0);
        horizontal_layout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontal_layout->setContentsMargins(0, 0, 0, 0);

        default_layout->addWidget(title_frame_, 0);
        
        (void)QObject::connect(min_win_button_, &QToolButton::clicked, [this]() {
            showMinimized();
        });

        (void)QObject::connect(max_win_button_, &QToolButton::clicked, [this]() {
            updateMaximumState();
        });
        (void)QObject::connect(close_button_, &QToolButton::clicked, [this]() {
            hide();
        });

        (void)QObject::connect(&qTheme,
            &ThemeManager::themeChangedFinished,
            this,
            &XMainWindow::onThemeChangedFinished);

        FramelessWidgetsHelper::get(this)->setTitleBarWidget(title_frame_);
        FramelessWidgetsHelper::get(this)->setSystemButton(min_win_button_, Global::SystemButtonType::Minimize);
        FramelessWidgetsHelper::get(this)->setSystemButton(max_win_button_, Global::SystemButtonType::Maximize);
        FramelessWidgetsHelper::get(this)->setSystemButton(close_button_, Global::SystemButtonType::Close);
        FramelessWidgetsHelper::get(this)->extendsContentIntoTitleBar();
        title_frame_label_->setText(kApplicationTitle);

        onThemeChangedFinished(qTheme.themeColor());

        default_layout->addWidget(content_widget_);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }
#else
    if (content_widget_ != nullptr) {
        auto* default_layout = new QVBoxLayout(this);

        title_bar_ = new StandardTitleBar(this);
        title_bar_->setWindowIconVisible(true);
        default_layout->addWidget(title_bar_);
        default_layout->addWidget(content_, 1);
        default_layout->setContentsMargins(0, 0, 0, 0);

        auto* helper = FramelessWidgetsHelper::get(this);
        helper->setTitleBarWidget(title_bar_);
        helper->setSystemButton(title_bar_->minimizeButton(), SystemButtonType::Minimize);
        helper->setSystemButton(title_bar_->maximizeButton(), SystemButtonType::Maximize);
        helper->setSystemButton(title_bar_->closeButton(), SystemButtonType::Close);

        default_layout->addWidget(title_bar_);
        default_layout->addWidget(content_widget_);
        default_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(default_layout);
    }    
#endif
    setAcceptDrops(true);
    readDriveInfo();
    installEventFilter(this);
    ensureInitTaskbar();
}

// QScopedPointer require default destructor.
XMainWindow::~XMainWindow() {
    XAMP_LOG_DEBUG("XMainWindow destory!");
}

void XMainWindow::onThemeChangedFinished(ThemeColor theme_color) {
#ifdef Q_OS_WIN
    qTheme.setTitleBarButtonStyle(close_button_, min_win_button_, max_win_button_);

    QString color;
    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        color = qTEXT("white");
        break;
    case ThemeColor::LIGHT_THEME:
        color = qTEXT("gray");
        break;
    }

    title_frame_label_->setStyleSheet(qSTR(R"(
        QLabel#titleFrameLabel {
        border: none;
        background: transparent;
	    color: %1;
        }
        )").arg(color));
#endif
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
#endif
}

bool XMainWindow::eventFilter(QObject * object, QEvent * event) {
    return QWidget::eventFilter(object, event);
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

void XMainWindow::initMaximumState() {
    if (!content_widget_) {
        return;
    }
    qTheme.updateMaximumIcon(max_win_button_, isMaximized());
    content_widget_->updateMaximumState(isMaximized());
}

void XMainWindow::updateMaximumState() {
    if (!content_widget_) {
        return;
    }

    if (isMaximized()) {
        setWindowState(windowState() & ~Qt::WindowMinimized);
        showNormal();
        content_widget_->updateMaximumState(false);
        qTheme.updateMaximumIcon(max_win_button_, false);
    }
    else {
        showMaximized();
        content_widget_->updateMaximumState(true);
        qTheme.updateMaximumIcon(max_win_button_, true);
    }
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
