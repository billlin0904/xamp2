#include "osx.h"
#include <QLayout>
#include <QMacCocoaViewContainer>
#include <Cocoa/Cocoa.h>

#import <AppKit/NSWindow.h>

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

void setBlurMaterial(const QWidget* widget, bool enable) {
    long winid = widget->winId();
    NSView* view = reinterpret_cast<NSView *>(winid);
    NSWindow* wndd = [view window];
    NSRect rect = NSMakeRect(0, 0, widget->width(), widget->height());

    auto effect_view = [[NSVisualEffectView alloc] init];
    effect_view.material = NSVisualEffectMaterialPopover;
    effect_view.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    effect_view.state = NSVisualEffectStateActive;
    effect_view.appearance = [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark];
    [effect_view setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];
    [effect_view setFrame:rect];

    //[effect_view setWantsLayer:YES];
    auto container = new QMacCocoaViewContainer(nullptr, const_cast<QWidget*>(widget));
    [[wndd contentView] addSubview:effect_view positioned:NSWindowBelow relativeTo:container->cocoaView()];
}
}
