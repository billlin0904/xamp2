#include <widget/vinylwidget.h>

#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QTest>
#include <limits>

namespace {
QPixmap testCover() {
    QPixmap cover(500, 500);
    cover.fill(QColor(22, 44, 63));
    QPainter painter(&cover);
    QLinearGradient sky(0, 0, 500, 500);
    sky.setColorAt(0, QColor(92, 134, 182));
    sky.setColorAt(0.48, QColor(194, 131, 154));
    sky.setColorAt(1, QColor(26, 57, 73));
    painter.fillRect(cover.rect(), sky);
    painter.setBrush(QColor(244, 206, 174));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(350, 165), 38, 38);
    QPainterPath ridge;
    ridge.moveTo(0, 350);
    ridge.lineTo(160, 225);
    ridge.lineTo(260, 325);
    ridge.lineTo(390, 265);
    ridge.lineTo(500, 365);
    ridge.lineTo(500, 500);
    ridge.lineTo(0, 500);
    painter.fillPath(ridge, QColor(29, 59, 72));
    painter.setPen(QColor(239, 233, 222));
    painter.setFont(QFont(QStringLiteral("Helvetica"), 26));
    painter.drawText(QRect(0, 350, 500, 65), Qt::AlignCenter, QStringLiteral("AFTER HOURS"));
    return cover;
}

QImage render(VinylWidget& widget) {
    QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(21, 26, 29));
    widget.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
    return image;
}
}

class VinylWidgetTest : public QObject {
    Q_OBJECT
private slots:
    void playbackAndVisibility() {
        VinylWidget widget;
        widget.resize(411, 411);
        widget.setCover(testCover());
        widget.show();
        QTest::qWait(30);
        auto* timer = widget.findChild<QTimer*>(QStringLiteral("vinylAnimationTimer"));
        QVERIFY(timer);
        QVERIFY(!timer->isActive());
        widget.setPlaying(true);
        QVERIFY(timer->isActive());
        const auto before = render(widget);
        QTest::qWait(150);
        QVERIFY(render(widget) != before);
        widget.setPlaying(false);
        QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 1200);
        const auto paused = render(widget);
        QTest::qWait(120);
        QCOMPARE(render(widget), paused);
        widget.setPlaying(true);
        widget.hide();
        QVERIFY(!timer->isActive());
        widget.show();
        QVERIFY(timer->isActive());
        widget.hide();
        widget.setPlaying(false);
        widget.show();
        QVERIFY(!timer->isActive());
    }

    void armLandsAndReturns() {
        VinylWidget widget;
        widget.resize(500, 500);
        widget.show();
        QTest::qWait(30);
        const QRect arm_area(0, 130, 195, 350);
        const auto parked = render(widget).copy(arm_area);
        widget.setPlaying(true);
        QCOMPARE(render(widget).copy(arm_area), parked);
        QTest::qWait(350);
        QVERIFY(render(widget).copy(arm_area) != parked);
        QTest::qWait(650);
        widget.setProgress(0.8);
        widget.setPlaying(false);
        const auto returning = render(widget);
        widget.setProgress(0.0);
        QCOMPARE(render(widget), returning);
        auto* timer = widget.findChild<QTimer*>(QStringLiteral("vinylAnimationTimer"));
        QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 1400);
        // The strip left of the disc contains the arm/rest without the rotated label.
        QCOMPARE(render(widget).copy(QRect(0, 130, 75, 350)), parked.copy(QRect(0, 0, 75, 350)));
        widget.setPlaying(true);
        widget.hide();
        widget.show();
        // Switching into vinyl mode during playback also starts from the arm rest.
        QCOMPARE(render(widget).copy(QRect(0, 130, 75, 350)), parked.copy(QRect(0, 0, 75, 350)));
    }

    void coverAndProgress() {
        VinylWidget widget;
        widget.resize(411, 411);
        widget.show();
        widget.setPlaying(true);
        QTest::qWait(1000);
        // Render without advancing the event loop: only the tonearm changes with progress.
        widget.setProgress(0.0);
        const auto lead_in = render(widget);
        widget.setProgress(1.0);
        QVERIFY(render(widget) != lead_in);
        widget.setProgress(std::numeric_limits<double>::quiet_NaN());
        QCOMPARE(render(widget), lead_in);
        widget.setProgress(-1.0);
        QCOMPARE(render(widget), lead_in);
        const auto original = render(widget);
        widget.setCover(testCover());
        QVERIFY(render(widget) != original);
        widget.setCover(QPixmap());
        QCOMPARE(render(widget), original);
        widget.resize(260, 180);
        QVERIFY(!render(widget).isNull());
    }

    void savePreview() {
        const auto directory = qEnvironmentVariable("XAMP_VINYL_PREVIEW_DIR");
        if (directory.isEmpty()) QSKIP("Set XAMP_VINYL_PREVIEW_DIR to save the visual fixtures");
        QVERIFY(QDir().mkpath(directory));
        VinylWidget widget;
        widget.resize(640, 640);
        widget.setCover(testCover());
        widget.show();
        QVERIFY(render(widget).save(directory + QStringLiteral("/vinyl-arm-stopped.png")));
        widget.setPlaying(true);
        QTest::qWait(1000);
        QVERIFY(render(widget).save(directory + QStringLiteral("/vinyl-arm-playing.png")));
        for (const bool dark : {true, false}) {
            widget.setDarkTheme(dark);
            QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
            image.fill(dark ? QColor(21, 26, 29) : QColor(239, 241, 241));
            widget.render(&image, QPoint(), QRegion(), QWidget::DrawChildren);
            QVERIFY(image.save(directory + (dark ? QStringLiteral("/vinyl-dark.png") : QStringLiteral("/vinyl-light.png"))));
        }
    }
};
QTEST_MAIN(VinylWidgetTest)
#include "vinyl_widget_test.moc"
