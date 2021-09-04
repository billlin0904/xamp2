#include "osx.h"
#include <QLayout>
#include <QMacCocoaViewContainer>
#include <Cocoa/Cocoa.h>

namespace osx {
void hideTitleBar(const QWidget* widget) {
    long winid = widget->winId();
    NSView* view = reinterpret_cast<NSView *>(winid);
    NSWindow* wndd = [view window];
    wndd.titlebarAppearsTransparent = YES;
    wndd.titleVisibility = NSWindowTitleHidden;
    wndd.styleMask |= NSFullSizeContentViewWindowMask;
    [[wndd standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [[wndd standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
    [[wndd standardWindowButton:NSWindowCloseButton] setHidden:YES];
}

void setBlurMaterial(const QWidget* widget, bool enable, bool use_native_window) {
    //auto effect_view = [[NSVisualEffectView alloc] init];
    //effect_view.material = NSVisualEffectMaterialDark;
    //effect_view.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    //effect_view.state = NSVisualEffectStateFollowsWindowActiveState;
    //auto mv = new QMacCocoaViewContainer(effect_view);
    //widget->layout()->addWidget(mv);
    //[effect_view release];
}
}
