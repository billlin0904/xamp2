#import <AppKit/AppKit.h>
#include <widget/macwindowbackdrop.h>
#include <QTest>
#include <QWidget>

class MacBackdropTest : public QObject {
    Q_OBJECT
private slots:
    void nativeMaterialLifecycle() {
        QWidget window;
        window.setAttribute(Qt::WA_TranslucentBackground);
        window.resize(640, 480);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        NSView* content = reinterpret_cast<NSView*>(window.winId());
        NSView* host = content.superview;
        NSWindow* native = content.window;
        const auto originalMask = native.styleMask;
        auto* backdrop = new MacWindowBackdrop(&window, [] {});
        QVERIFY(backdrop->apply(true, 1, true));
        NSVisualEffectView* effect = nil;
        for (NSView* child in host.subviews) {
            if ([child isKindOfClass:[NSVisualEffectView class]] && !child.hidden) {
                effect = static_cast<NSVisualEffectView*>(child);
            }
        }
        QVERIFY(effect);
        QCOMPARE(effect.material, NSVisualEffectMaterialSidebar);
        QCOMPARE(effect.blendingMode, NSVisualEffectBlendingModeBehindWindow);
        QCOMPARE(effect.state, NSVisualEffectStateFollowsWindowActiveState);
        QVERIFY(!native.opaque);
        QCOMPARE(native.styleMask, originalMask);
        QVERIFY(!window.windowFlags().testFlag(Qt::FramelessWindowHint));
        QVERIFY([host.subviews indexOfObject:effect] < [host.subviews indexOfObject:content]);

        QVERIFY(backdrop->apply(true, 2, false));
        QCOMPARE(effect.material, NSVisualEffectMaterialUnderWindowBackground);
        QVERIFY([effect.appearance.name isEqualToString:NSAppearanceNameAqua]);
        window.resize(800, 600);
        QCoreApplication::processEvents();
        QVERIFY(NSEqualRects(effect.frame, host.bounds));
        QVERIFY(!backdrop->apply(false, 0, false));
        QVERIFY(effect.hidden);
        // Qt keeps its alpha-capable backing store for toggling at runtime.
        // With the effect hidden, the native background must be fully solid.
        QCOMPARE(native.backgroundColor.alphaComponent, 1.0);
        QVERIFY(!native.titlebarAppearsTransparent);
        QVERIFY(backdrop->apply(true, 1, true));
        QVERIFY(!effect.hidden);
        QCOMPARE(native.styleMask, originalMask);
        delete backdrop;
        QVERIFY(![host.subviews containsObject:effect]);
    }

    void accessibilityNotificationAndCleanup() {
        QWidget window;
        int callbacks = 0;
        auto* backdrop = new MacWindowBackdrop(&window, [&] { ++callbacks; });
        NSNotificationCenter* center = [NSWorkspace sharedWorkspace].notificationCenter;
        [center postNotificationName:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification object:nil];
        QTRY_COMPARE(callbacks, 1);
        // An already queued notification must not call the destroyed controller.
        [center postNotificationName:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification object:nil];
        delete backdrop;
        QCoreApplication::processEvents();
        QCOMPARE(callbacks, 1);
        [center postNotificationName:NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification object:nil];
        QCoreApplication::processEvents();
        QCOMPARE(callbacks, 1);
        QCOMPARE(MacWindowBackdrop::effectsEnabled(),
            ![NSWorkspace sharedWorkspace].accessibilityDisplayShouldReduceTransparency
                && ![NSWorkspace sharedWorkspace].accessibilityDisplayShouldIncreaseContrast);
    }
};

QTEST_MAIN(MacBackdropTest)
#include "mac_backdrop_test.moc"
