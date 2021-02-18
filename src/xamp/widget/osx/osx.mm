#include "osx.h"
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
}
