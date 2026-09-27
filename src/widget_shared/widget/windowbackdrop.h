#pragma once

#include <QApplication>
#include <QDynamicPropertyChangeEvent>
#include <QOperatingSystemVersion>
#include <QTimer>
#include <QMenu>
#include <QLibrary>
#include <QSettings>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif
#include <QWKWidgets/widgetwindowagent.h>
#include <widget/appsettings.h>
#include <thememanager.h>

// Uses the existing window agent so native frame handling stays in one place.
class WindowBackdrop final : public QObject {
public:
    static QString settingKey() { return QStringLiteral("windowBackdrop"); }
    static int mode() {
        return qAppSettings.contains(settingKey())
            ? qBound(0, qAppSettings.valueAsInt(settingKey()), 2) : 1;
    }
    static bool supported() {
#ifdef Q_OS_WIN
        const auto version = QOperatingSystemVersion::current();
        return version.majorVersion() >= 10 && version.microVersion() >= 22621;
#else
        return false;
#endif
    }
    static bool effectsEnabled() {
#ifdef Q_OS_WIN
        HIGHCONTRASTW contrast{ sizeof(HIGHCONTRASTW) };
        SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0);
        QSettings personalization(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QSettings::NativeFormat);
        return supported() && !(contrast.dwFlags & HCF_HIGHCONTRASTON)
            && personalization.value(QStringLiteral("EnableTransparency"), 1).toBool();
#else
        return false;
#endif
    }
    static void setMode(int value) {
        qAppSettings.setValue(settingKey(), qBound(0, value, 2));
        qAppSettings.save();
        qApp->setProperty("windowBackdrop", value);
    }
    WindowBackdrop(QWidget* window, QWK::WidgetWindowAgent* agent)
        : QObject(window), window_(window), agent_(agent) {
        qApp->installEventFilter(this);
        connect(&qTheme, &ThemeManager::themeChangedFinished, this,
            [this](ThemeColor) { QTimer::singleShot(0, this, [this] { apply(); }); });
    }
    void apply() {
        bool enabled = false;
#ifdef Q_OS_WIN
        if (supported()) {
            agent_->setWindowAttribute(QStringLiteral("dark-mode"), qTheme.isDarkTheme());
            agent_->setWindowAttribute(QStringLiteral("mica"), false);
            agent_->setWindowAttribute(QStringLiteral("acrylic-material"), false);
            if (mode() != 0 && effectsEnabled()) {
                enabled = agent_->setWindowAttribute(mode() == 1
                    ? QStringLiteral("mica") : QStringLiteral("acrylic-material"), true);
            }
        }
#endif
        // Expose the native material through the shell and playlist surfaces.
        window_->setProperty("backdropActive", enabled);
        window_->setStyleSheet(enabled ? QStringLiteral(
            "QWidget#XMainWindow { background: %2; }"
            "QWidget#XampWindow, QWidget#currentView, QFrame#richPlaylistPage, "
            "QFrame#richPlaylistContentPanel, QFrame#richPlaylistListPanel, QFrame#playlistHero { background: transparent; }"
            "QFrame#richPlaylistCoverPanel { background: %1; }"
            "QTableView#richPlaylistTableView, QTableView#richPlaylistTableView QWidget, "
            "QTableView#richPlaylistTableView QHeaderView::section { background: transparent; }"
            "QSlider#volumeSlider { background: transparent; }"
            "QFrame#sliderFrame2, QFrame#sliderFrame2 QWidget { background: transparent; }"
            "QFrame#sliderFrame2 QPushButton#settingsButton:checked { background: %3; }"
            "QFrame#sliderFrame2 QPushButton#settingsButton:hover { background: rgba(100,145,120,75); }"
            "QFrame#sliderFrame2 QLineEdit#sidebarSearch { background: rgba(90,100,96,24); }"
            "QFrame#bottomFrame { background: %1; }").arg(qTheme.isDarkTheme()
                ? QStringLiteral("rgba(20,23,25,65)") : QStringLiteral("rgba(225,235,228,65)"))
            .arg(mode() == 2 ? (qTheme.isDarkTheme() ? QStringLiteral("rgba(16,21,23,35)")
                : QStringLiteral("rgba(225,235,228,20)")) : QStringLiteral("transparent"))
            .arg(qTheme.isDarkTheme() ? QStringLiteral("rgba(41,70,62,230)")
                : QStringLiteral("rgba(180,210,190,210)")) : QString());
        if (agent_->titleBar()) agent_->titleBar()->setStyleSheet(enabled
            ? QStringLiteral("background: transparent;") : QString());
        if (auto* content = window_->findChild<QWidget*>(QStringLiteral("currentView"))) {
            content->setProperty("topLeftCornerOutsideColor", enabled
                ? QStringLiteral("transparent") : qTheme.backgroundColorString());
        }
        window_->update();
    }
protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        if (event->type() == QEvent::Show) {
            if (auto* menu = qobject_cast<QMenu*>(object)) applyMenu(menu);
        }
        if ((object == qApp && event->type() == QEvent::DynamicPropertyChange
            && static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName() == "windowBackdrop")
            || (object == window_ && event->type() == QEvent::Show)) {
            QTimer::singleShot(0, this, [this] { apply(); });
        }
        return QObject::eventFilter(object, event);
    }
private:
    void applyMenu(QMenu* menu) {
#ifdef Q_OS_WIN
        if (!supported()) return;
        static QLibrary dwm(QStringLiteral("dwmapi"));
        using SetAttribute = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        using ExtendFrame = HRESULT (WINAPI*)(HWND, const MARGINS*);
        static auto setAttribute = reinterpret_cast<SetAttribute>(dwm.resolve("DwmSetWindowAttribute"));
        static auto extendFrame = reinterpret_cast<ExtendFrame>(dwm.resolve("DwmExtendFrameIntoClientArea"));
        if (!setAttribute || !extendFrame) return;
        const auto hwnd = reinterpret_cast<HWND>(menu->winId());
        const BOOL dark = qTheme.isDarkTheme();
        setAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        const bool enable = mode() != 0 && effectsEnabled();
        const DWM_SYSTEMBACKDROP_TYPE backdrop = enable ? DWMSBT_TRANSIENTWINDOW : DWMSBT_NONE;
        const MARGINS margins = enable ? MARGINS{-1, -1, -1, -1} : MARGINS{0, 0, 0, 0};
        const bool applied = SUCCEEDED(setAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop)))
            && SUCCEEDED(extendFrame(hwnd, &margins));
        menu->setStyleSheet(enable && applied ? QStringLiteral(
            "QMenu { background: %2; border: 1px solid %1; }").arg(dark
                ? QStringLiteral("#46504D") : QStringLiteral("#CCD1CE"))
            .arg(dark ? QStringLiteral("rgba(20,27,28,110)")
                : QStringLiteral("rgba(225,235,228,100)")) : QString());
#else
        Q_UNUSED(menu);
#endif
    }
    QWidget* window_;
    QWK::WidgetWindowAgent* agent_;
};
