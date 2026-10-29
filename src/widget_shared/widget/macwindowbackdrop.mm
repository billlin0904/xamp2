#import <AppKit/AppKit.h>

#include <widget/macwindowbackdrop.h>
#include <QPointer>
#include <QWidget>

struct MacWindowBackdrop::Impl {
    NSVisualEffectView* effect = nil;
    id observer = nil;
};

MacWindowBackdrop::MacWindowBackdrop(QWidget* window,
    std::function<void()> accessibilityChanged)
    : QObject(window), impl_(std::make_unique<Impl>()), window_(window) {
    const QPointer<MacWindowBackdrop> guard(this);
    impl_->observer = [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserverForName:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification
        object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification*) {
            if (guard) {
                QMetaObject::invokeMethod(guard.data(), [guard, accessibilityChanged] {
                    if (guard) accessibilityChanged();
                }, Qt::QueuedConnection);
            }
        }];
}

MacWindowBackdrop::~MacWindowBackdrop() {
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:impl_->observer];
    [impl_->effect removeFromSuperview];
    [impl_->effect release];
}

bool MacWindowBackdrop::effectsEnabled() {
    NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
    return !workspace.accessibilityDisplayShouldReduceTransparency
        && !workspace.accessibilityDisplayShouldIncreaseContrast;
}

bool MacWindowBackdrop::apply(bool enabled, int mode, bool dark) {
    NSView* content = reinterpret_cast<NSView*>(window_->winId());
    NSWindow* nativeWindow = content.window;
    NSView* host = content.superview;
    if (!nativeWindow || !host) return false;

    nativeWindow.appearance = [NSAppearance appearanceNamed:
        dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    if (enabled && !impl_->effect) {
        impl_->effect = [[NSVisualEffectView alloc] initWithFrame:host.bounds];
        impl_->effect.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        impl_->effect.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        impl_->effect.state = NSVisualEffectStateFollowsWindowActiveState;
    }
    if (enabled) {
        if (impl_->effect.superview != host) {
            [host addSubview:impl_->effect positioned:NSWindowBelow relativeTo:content];
        }
        impl_->effect.frame = host.bounds;
        impl_->effect.material = mode == 2 ? NSVisualEffectMaterialUnderWindowBackground
                                         : NSVisualEffectMaterialSidebar;
        impl_->effect.appearance = nativeWindow.appearance;
    }
    impl_->effect.hidden = !enabled;
    nativeWindow.opaque = !enabled;
    nativeWindow.backgroundColor = enabled ? [NSColor clearColor] : [NSColor windowBackgroundColor];
    nativeWindow.titlebarAppearsTransparent = enabled;
    return enabled;
}
