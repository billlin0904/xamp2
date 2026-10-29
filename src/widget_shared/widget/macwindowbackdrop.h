#pragma once

#include <QObject>
#include <functional>
#include <memory>
#include <widget/widget_shared_global.h>

class QWidget;

// AppKit stays behind the Qt content; Cocoa retains the native window frame.
class XAMP_WIDGET_SHARED_API MacWindowBackdrop final : public QObject {
public:
    MacWindowBackdrop(QWidget* window, std::function<void()> accessibilityChanged);
    ~MacWindowBackdrop() override;

    static bool effectsEnabled();
    bool apply(bool enabled, int mode, bool dark);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    QWidget* window_;
};
