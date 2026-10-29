#include <widget/vinylwidget.h>

#include <QConicalGradient>
#include <QHideEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QShowEvent>

#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kSceneSize = 500.0;
constexpr qreal kRecordRadius = 197.0;
const QPointF kRecordCenter(278.0, 236.0);
const QPointF kArmPivot(53.0, 424.0);
constexpr qreal kRevolutionsPerMinute = 100.0 / 3.0; // Standard LP speed: 33 1/3 RPM.
constexpr qreal kDegreesPerSecond = kRevolutionsPerMinute * 360.0 / 60.0;
constexpr qreal kArmTravelSeconds = 0.9;
}

VinylWidget::VinylWidget(QWidget* parent)
    : QWidget(parent), timer_(this) {
    setObjectName(QStringLiteral("vinylWidget"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    timer_.setObjectName(QStringLiteral("vinylAnimationTimer"));
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(16);
    connect(&timer_, &QTimer::timeout, this, [this] { advanceAnimation(); });
}

QSize VinylWidget::sizeHint() const {
    return QSize(600, 600);
}

void VinylWidget::setCover(const QPixmap& cover) {
    if (cover_.cacheKey() == cover.cacheKey()) return;
    cover_ = cover;
    record_ = QPixmap();
    update();
}

void VinylWidget::setDarkTheme(bool dark) {
    if (dark_ == dark) return;
    dark_ = dark;
    update();
}

void VinylWidget::setPlaying(bool playing) {
    if (playing_ == playing) return;
    // Account for the final partial frame before changing playback state.
    if (timer_.isActive()) advanceAnimation();
    playing_ = playing;
    if (playing_) arm_progress_ = progress_;
    updateAnimation();
}

void VinylWidget::setProgress(double progress) {
    if (!std::isfinite(progress)) progress = 0.0;
    progress_ = std::clamp(progress, 0.0, 1.0);
    // A stop resets the transport position. Keep the stylus at its last groove
    // while the arm returns, rather than jumping to the lead-in first.
    if (playing_ || arm_position_ == 0.0) arm_progress_ = progress_;
    update();
}

void VinylWidget::updateAnimation() {
    const bool moving = playing_ || arm_position_ > 0.0;
    if (isVisible() && moving) {
        if (!timer_.isActive()) {
            frame_clock_.start();
            timer_.start();
        }
    } else {
        timer_.stop();
        frame_clock_.invalidate();
    }
    update();
}

void VinylWidget::advanceAnimation() {
    const auto seconds = frame_clock_.isValid()
        ? static_cast<qreal>(frame_clock_.nsecsElapsed()) / 1.0e9 : 0.0;
    frame_clock_.start();
    if (playing_) angle_ = std::fmod(angle_ + kDegreesPerSecond * seconds, 360.0);
    // Clamp only the arm step, so a delayed frame cannot overshoot the landing/rest position.
    const auto step = std::min(seconds / kArmTravelSeconds, 1.0);
    arm_position_ = playing_ ? std::min(1.0, arm_position_ + step)
                             : std::max(0.0, arm_position_ - step);
    updateAnimation();
}

void VinylWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    // Enter from the arm rest when opening vinyl mode during playback too.
    // Hidden widgets still do not run a background animation timer.
    arm_position_ = 0.0;
    updateAnimation();
}

void VinylWidget::hideEvent(QHideEvent* event) {
    timer_.stop();
    frame_clock_.invalidate();
    QWidget::hideEvent(event);
}

void VinylWidget::rebuildRecord() {
    // Cache grooves and cover at device resolution; animation only rotates this pixmap.
    const auto logical_size = std::max(1, qRound(std::min(width(), height()) * 2.0 * kRecordRadius / kSceneSize));
    const auto dpr = devicePixelRatioF();
    const QSize pixels(static_cast<int>(std::ceil(logical_size * dpr)), static_cast<int>(std::ceil(logical_size * dpr)));
    if (!record_.isNull() && record_.size() == pixels && record_.devicePixelRatio() == dpr) return;
    record_ = QPixmap(pixels);
    record_.setDevicePixelRatio(dpr);
    record_.fill(Qt::transparent);
    QPainter painter(&record_);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.scale(logical_size / (kRecordRadius * 2.0), logical_size / (kRecordRadius * 2.0));
    painter.translate(kRecordRadius, kRecordRadius);
    const QRectF disc(-kRecordRadius, -kRecordRadius, kRecordRadius * 2.0, kRecordRadius * 2.0);
    QRadialGradient vinyl(0, 0, kRecordRadius);
    vinyl.setColorAt(0.0, QColor(34, 35, 36));
    vinyl.setColorAt(0.48, QColor(12, 13, 14));
    vinyl.setColorAt(0.98, QColor(20, 21, 22));
    vinyl.setColorAt(1.0, QColor(5, 6, 7));
    painter.setPen(QPen(QColor(65, 67, 69), 1.0));
    painter.setBrush(vinyl);
    painter.drawEllipse(disc);
    painter.setBrush(Qt::NoBrush);
    for (int i = 0; i < 88; ++i) {
        const auto radius = 100.0 + i * 1.06;
        painter.setPen(QPen(i % 4 == 0 ? QColor(55, 57, 59, 115) : QColor(5, 6, 7, 190), 0.65));
        painter.drawEllipse(QPointF(), radius, radius);
    }
    // Fine lead-in/lead-out rings and a slightly raised paper label rim.
    for (const auto radius : {93.0, 95.0, 192.0, 194.0}) {
        painter.setPen(QPen(QColor(72, 74, 76, 100), 0.8));
        painter.drawEllipse(QPointF(), radius, radius);
    }
    painter.setPen(QPen(QColor(5, 6, 7), 5.0));
    painter.setBrush(QColor(43, 46, 48));
    painter.drawEllipse(QPointF(), 89.0, 89.0);
    painter.save();
    QPainterPath label_clip;
    label_clip.addEllipse(QPointF(), 84.0, 84.0);
    painter.setClipPath(label_clip);
    if (!cover_.isNull()) {
        const auto side = std::min(cover_.width(), cover_.height());
        const QRectF crop((cover_.width() - side) / 2.0, (cover_.height() - side) / 2.0, side, side);
        painter.drawPixmap(QRectF(-84, -84, 168, 168), cover_, crop);
    } else {
        QLinearGradient label(-84, -84, 84, 84);
        label.setColorAt(0, QColor(105, 147, 143));
        label.setColorAt(1, QColor(35, 64, 65));
        painter.fillRect(QRectF(-84, -84, 168, 168), label);
    }
    painter.restore();
}

void VinylWidget::paintEvent(QPaintEvent*) {
    rebuildRecord();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const auto scale = std::min(width(), height()) / kSceneSize;
    painter.translate((width() - kSceneSize * scale) / 2.0, (height() - kSceneSize * scale) / 2.0);
    painter.scale(scale, scale);

    // Soft platter shadow without an offscreen graphics effect on the translucent window.
    painter.setPen(Qt::NoPen);
    QRadialGradient shadow(kRecordCenter + QPointF(0, 8), 222);
    shadow.setColorAt(0.86, QColor(0, 0, 0, dark_ ? 100 : 48));
    shadow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.setBrush(shadow);
    painter.drawEllipse(kRecordCenter + QPointF(0, 8), 222, 222);
    QLinearGradient platter(0, 25, 0, 448);
    platter.setColorAt(0, dark_ ? QColor(100, 107, 111) : QColor(247, 248, 248));
    platter.setColorAt(0.5, dark_ ? QColor(51, 56, 60) : QColor(197, 202, 204));
    platter.setColorAt(1, dark_ ? QColor(112, 119, 122) : QColor(238, 240, 240));
    painter.setBrush(platter);
    painter.setPen(QPen(dark_ ? QColor(121, 127, 130, 100) : QColor(255, 255, 255, 190), 1));
    painter.drawEllipse(kRecordCenter, 207, 207);

    painter.save();
    painter.translate(kRecordCenter);
    painter.rotate(angle_);
    painter.drawPixmap(QRectF(-kRecordRadius, -kRecordRadius, kRecordRadius * 2.0, kRecordRadius * 2.0),
                       record_, QRectF(0, 0, record_.width(), record_.height()));
    painter.restore();

    // Light stays fixed in the room while grooves and the label rotate below it.
    QConicalGradient sheen(kRecordCenter, -30);
    sheen.setColorAt(0.0, QColor(255, 255, 255, 0));
    sheen.setColorAt(0.18, QColor(255, 255, 255, 3));
    sheen.setColorAt(0.25, QColor(255, 255, 255, 35));
    sheen.setColorAt(0.32, QColor(255, 255, 255, 2));
    sheen.setColorAt(0.68, QColor(255, 255, 255, 0));
    sheen.setColorAt(0.75, QColor(255, 255, 255, 24));
    sheen.setColorAt(0.82, QColor(255, 255, 255, 0));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
    QPainterPath groove_area;
    groove_area.addEllipse(kRecordCenter, kRecordRadius, kRecordRadius);
    groove_area.addEllipse(kRecordCenter, 90, 90);
    painter.setPen(Qt::NoPen);
    painter.fillPath(groove_area, sheen);
    QRadialGradient spindle(kRecordCenter - QPointF(2, 2), 6);
    spindle.setColorAt(0, QColor(250, 251, 252));
    spindle.setColorAt(0.5, QColor(153, 164, 169));
    spindle.setColorAt(1, QColor(45, 49, 51));
    painter.setBrush(spindle);
    painter.drawEllipse(kRecordCenter, 5, 5);

    // Fixed arm rest to the left of the platter: the parked arm sits in this cradle.
    painter.setPen(QPen(QColor(113, 128, 136), 1.0));
    painter.setBrush(dark_ ? QColor(50, 60, 65) : QColor(181, 192, 198));
    painter.drawRoundedRect(QRectF(23, 247, 20, 25), 4, 4);
    painter.setBrush(QColor(205, 216, 221));
    painter.drawRoundedRect(QRectF(19, 243, 28, 9), 3, 3);

    // The pivot stays fixed. A smooth landing moves the stylus onto the lead-in groove;
    // playback progress slowly moves it toward the label, independently of disc rotation.
    const auto eased = arm_position_ * arm_position_ * (3.0 - 2.0 * arm_position_);
    const auto arm_angle = -4.0 + (16.0 + arm_progress_ * 20.0) * eased;
    painter.setBrush(dark_ ? QColor(32, 36, 39) : QColor(204, 209, 211));
    painter.setPen(QPen(dark_ ? QColor(104, 113, 119) : QColor(183, 189, 192), 1));
    painter.drawEllipse(kArmPivot, 29, 29);
    painter.save();
    painter.translate(kArmPivot);
    painter.rotate(arm_angle);
    QPainterPath arm;
    arm.moveTo(0, 15);
    arm.lineTo(0, -88);
    arm.cubicTo(0, -112, -6, -122, -6, -145);
    arm.lineTo(-6, -201);
    painter.save();
    painter.translate(3, 4);
    painter.setPen(QPen(QColor(0, 0, 0, 65), 14, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(arm);
    painter.restore();
    painter.setPen(QPen(QColor(117, 128, 134), 13, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(arm);
    painter.setPen(QPen(QColor(234, 239, 241), 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(arm);
    painter.setPen(QPen(QColor(255, 255, 255, 220), 2, Qt::SolidLine, Qt::RoundCap));
    painter.drawPath(arm.translated(-2, 0));
    // Headshell, cartridge, tiny stylus and rear counterweight.
    painter.setPen(QPen(QColor(141, 151, 156), 1));
    painter.setBrush(QColor(227, 233, 235));
    painter.drawRoundedRect(QRectF(-18, -253, 24, 62), 4, 4);
    painter.setBrush(QColor(104, 115, 122));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(-12, -245, 4, 13), 1, 1);
    painter.drawRoundedRect(QRectF(-4, -245, 4, 13), 1, 1);
    painter.setBrush(QColor(48, 56, 60));
    painter.drawRoundedRect(QRectF(-14, -260, 16, 9), 2, 2);
    painter.setPen(QPen(QColor(181, 203, 207), 1.5));
    painter.drawLine(QPointF(-6, -260), QPointF(-6, -265));
    painter.setPen(QPen(QColor(133, 144, 150), 1));
    painter.setBrush(QColor(192, 202, 206));
    painter.drawRoundedRect(QRectF(-12, 7, 24, 38), 3, 3);
    painter.restore();
    QLinearGradient pivot(kArmPivot - QPointF(20, 20), kArmPivot + QPointF(20, 20));
    pivot.setColorAt(0, QColor(244, 247, 248));
    pivot.setColorAt(0.55, QColor(185, 196, 202));
    pivot.setColorAt(1, QColor(105, 120, 130));
    painter.setBrush(pivot);
    painter.setPen(QPen(QColor(132, 144, 151), 1));
    painter.drawEllipse(kArmPivot, 15, 15);
}
