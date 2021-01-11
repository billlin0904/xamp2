#include "osx.h"
#include <Cocoa/Cocoa.h>

namespace osx {
void hideTitleBar(const QWidget* widget) {
    long winid = widget->winId();
/*
    NSView *nativeView = reinterpret_cast<NSView *>(winid);
    NSWindow* nativeWindow = [nativeView window];

    [nativeWindow setStyleMask:
                      [nativeWindow styleMask] | NSFullSizeContentViewWindowMask | NSWindowTitleHidden];

    [nativeWindow setTitlebarAppearsTransparent:YES];
*/
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
