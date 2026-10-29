#include <widget/lrcpage.h>
#include <widget/vinylwidget.h>
#include <widget/lyricsshowwidget.h>
#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <thememanager.h>

#include <QDir>
#include <QPainter>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTranslator>

class VinylPageTest : public QObject {
    Q_OBJECT
    QTemporaryDir settings_;
    QTranslator translator_;
private slots:
    void initTestCase() {
        QVERIFY(settings_.isValid());
        qAppSettings.loadIniFile(settings_.filePath(QStringLiteral("vinyl-test.ini")));
        qAppSettings.setDefaultValue(kLyricsFontSize, 16);
        qAppSettings.setEnumValue(kAppSettingTheme, ThemeColor::DARK_THEME);
        qAppSettings.setValue(QStringLiteral("lyricsVinylMode"), false);
        const auto catalogs = qEnvironmentVariable("XAMP_VINYL_TEST_CATALOGS");
        if (!catalogs.isEmpty() && translator_.load(catalogs + QStringLiteral("/widget_shared_zh_TW.qm"))) {
            qApp->installTranslator(&translator_);
        }
        qTheme.setThemeColor(ThemeColor::DARK_THEME);
    }

    void switchAndRememberMode() {
        LrcPage page;
        page.resize(1050, 700);
        page.show();
        auto* button = page.findChild<QToolButton*>(QStringLiteral("vinylModeButton"));
        auto* stack = page.findChild<QStackedWidget*>(QStringLiteral("lrcCoverStack"));
        QVERIFY(button);
        QVERIFY(stack);
        QVERIFY(button->isVisible());
        QVERIFY(!page.vinylMode());
        QCOMPARE(stack->currentWidget(), page.cover());
        page.lyrics()->setFullLrc(QStringLiteral("A song for the evening\nKeep the music playing"), 120);
        const auto text = page.lyrics()->parsedLyrics();
        QTest::mouseClick(button, Qt::LeftButton);
        QVERIFY(page.vinylMode());
        QCOMPARE(stack->currentWidget()->objectName(), QStringLiteral("vinylWidget"));
        QVERIFY(!page.lyrics()->isVisible());
        QVERIFY(!page.format()->isVisible());
        QCOMPARE(page.lyrics()->parsedLyrics(), text);
        QVERIFY(qAppSettings.valueAsBool(QStringLiteral("lyricsVinylMode")));
        LrcPage reopened;
        QVERIFY(reopened.vinylMode());
        button->setFocus();
        QTest::keyClick(button, Qt::Key_Space);
        QVERIFY(!page.vinylMode());
        QCOMPARE(stack->currentWidget(), page.cover());
        QVERIFY(page.lyrics()->isVisible());
        QVERIFY(page.format()->isVisible());
        QCOMPARE(page.lyrics()->parsedLyrics(), text);
        QVERIFY(!qAppSettings.valueAsBool(QStringLiteral("lyricsVinylMode")));
    }

    void playbackOnPage() {
        LrcPage page;
        page.resize(1050, 700);
        page.show();
        page.setVinylMode(true);
        auto* timer = page.findChild<QTimer*>(QStringLiteral("vinylAnimationTimer"));
        QVERIFY(timer);
        QVERIFY(!timer->isActive());
        page.setPlaying(true);
        QVERIFY(timer->isActive());
        page.setVinylMode(false);
        QVERIFY(!timer->isActive());
        page.setVinylMode(true);
        QVERIFY(timer->isActive());
        page.setPlaying(false);
        QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 1200);
        page.setPlaybackProgress(30.0, 120.0);
        page.setPlaybackProgress(0.0, 0.0);
        page.hide();
        QVERIFY(!timer->isActive());
    }

    void pagePreview() {
        const auto directory = qEnvironmentVariable("XAMP_VINYL_PREVIEW_DIR");
        if (directory.isEmpty()) QSKIP("Set XAMP_VINYL_PREVIEW_DIR to save the lyrics-page previews");
        QVERIFY(QDir().mkpath(directory));
        LrcPage page;
        page.resize(1100, 680);
        QPixmap cover(500, 500);
        QPainter painter(&cover);
        QLinearGradient gradient(0, 0, 500, 500);
        gradient.setColorAt(0, QColor(107, 140, 190));
        gradient.setColorAt(0.5, QColor(207, 143, 168));
        gradient.setColorAt(1, QColor(28, 55, 80));
        painter.fillRect(cover.rect(), gradient);
        painter.setBrush(QColor(245, 215, 179));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(345, 150), 36, 36);
        painter.end();
        page.setCover(cover);
        page.lyrics()->setFullLrc(QStringLiteral("A song for the evening\nLet the record turn\nKeep the music playing\nUntil the morning comes"), 120);
        page.setVinylMode(true);
        page.setPlaying(true);
        page.show();
        for (const auto theme : {ThemeColor::DARK_THEME, ThemeColor::LIGHT_THEME}) {
            qTheme.setThemeColor(theme);
            QTest::qWait(50);
            page.setAutoFillBackground(true);
            auto palette = page.palette();
            palette.setColor(QPalette::Window, qTheme.backgroundColor());
            page.setPalette(palette);
            QVERIFY(page.grab().save(directory + (theme == ThemeColor::DARK_THEME
                ? QStringLiteral("/lyrics-vinyl-dark.png") : QStringLiteral("/lyrics-vinyl-light.png"))));
        }
    }
};
QTEST_MAIN(VinylPageTest)
#include "vinyl_page_test.moc"
