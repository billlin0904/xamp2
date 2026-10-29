#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif
#include <widget/globalshortcut.h>
#include <QTest>
#include <limits>

class GlobalShortcutTest : public QObject {
    Q_OBJECT
private slots:
    void unsupportedKeys_data() {
        QTest::addColumn<int>("qt_key");
        QTest::newRow("media-play") << static_cast<int>(Qt::Key_MediaPlay);
        QTest::newRow("media-stop") << static_cast<int>(Qt::Key_MediaStop);
        QTest::newRow("media-next") << static_cast<int>(Qt::Key_MediaNext);
        QTest::newRow("media-previous") << static_cast<int>(Qt::Key_MediaPrevious);
        QTest::newRow("unknown") << static_cast<int>(Qt::Key_unknown);
        QTest::newRow("unsupported-f24") << static_cast<int>(Qt::Key_F24);
    }

    void unsupportedKeys() {
#ifdef Q_OS_MAC
        QFETCH(int, qt_key);
        GlobalShortcut shortcut;
        // Carbon keycode 0 is A, so a failed conversion must never use 0.
        const auto native = shortcut.nativeKeycode(static_cast<Qt::Key>(qt_key));
        QVERIFY2(native != static_cast<quint32>(kVK_ANSI_A), "Unsupported shortcut would intercept A / Zhuyin ㄇ");
        QCOMPARE(native, (std::numeric_limits<quint32>::max)());
#else
        QSKIP("Carbon keycode regression is macOS-specific");
#endif
    }

    void invalidKeyCannotBeRegistered() {
        GlobalShortcut shortcut;
        const auto invalid = (std::numeric_limits<quint32>::max)();
        QVERIFY(!shortcut.registerShortcut(WId(0), invalid, 0));
        QVERIFY(!shortcut.unregisterShortcut(WId(0), invalid, 0));
    }

    void supportedKeys() {
#ifdef Q_OS_MAC
        GlobalShortcut shortcut;
        QCOMPARE(static_cast<quint32>(kVK_ANSI_A), quint32(0));
        QCOMPARE(shortcut.nativeKeycode(Qt::Key_A), static_cast<quint32>(kVK_ANSI_A));
        QCOMPARE(shortcut.nativeKeycode(Qt::Key_F1), static_cast<quint32>(kVK_F1));
        QCOMPARE(shortcut.nativeKeycode(Qt::Key_F10), static_cast<quint32>(kVK_F10));
        QCOMPARE(shortcut.nativeKeycode(Qt::Key_Return), static_cast<quint32>(kVK_Return));
        QCOMPARE(shortcut.nativeKeycode(Qt::Key_Escape), static_cast<quint32>(kVK_Escape));
        QCOMPARE(shortcut.nativeKeycode(Qt::Key_VolumeMute), static_cast<quint32>(kVK_Mute));
#else
        QSKIP("Carbon keycode regression is macOS-specific");
#endif
    }
};
QTEST_MAIN(GlobalShortcutTest)
#include "globalshortcut_test.moc"
