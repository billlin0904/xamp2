#include <thememanager.h>
#include <widget/image_utiltis.h>
#include <widget/str_utilts.h>
#include <widget/vinylwidget.h>

static constexpr auto kBackgroundSize = 350;

VinylWidget::VinylWidget(QWidget *parent)
    : QWidget(parent)
    , timer_(this) {
    angle_ = 0.0;

    QObject::connect(&timer_, &QTimer::timeout, [=]() {
        angle_  = angle_ + 1 % 360;
        update();
    });

    background_ = Pixmap::resizeImage(QPixmap(Q_UTF8(":/xamp/Resource/vinyl_blackground.png")),
                                      QSize(kBackgroundSize, kBackgroundSize),
                                      true);
    vinly_ = Pixmap::resizeImage(QPixmap(Q_UTF8(":/xamp/Resource/vinyl.png")),
                                 QSize(kBackgroundSize, kBackgroundSize),
                                 true);
    cover_ = Pixmap::resizeImage(ThemeManager::instance().pixmap().defaultSizeUnknownCover(),
                                 QSize(kBackgroundSize, kBackgroundSize),
                                 true);

    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(45);
}

void VinylWidget::writeBackground() {
    QPixmap temp(kBackgroundSize, kBackgroundSize);
    temp.fill(Qt::transparent);

    QPainter painter(&temp);

    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);

    painter.drawPixmap(0, 0, background_);
    painter.drawPixmap(0, 0, vinly_);
    painter.drawPixmap(0, 0, cover_);
    image_ = temp;
}

void VinylWidget::setPixmap(QPixmap const &image) {
    auto resize_image = Pixmap::resizeImage(image,
                                            QSize(kBackgroundSize, kBackgroundSize),
                                            true);
    QPixmap temp(resize_image.width(), resize_image.height());
    temp.fill(Qt::transparent);

    QPainter painter(&temp);

    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);

    auto cover_size = 235;
    const QRect image_react(
        QPoint{ 59, 59 },
        QSize(cover_size, cover_size));

    QPainterPath path;
    path.addEllipse(image_react);
    painter.setClipPath(path);
    painter.drawPixmap(image_react, resize_image);
    painter.end();

    cover_ = temp;

    writeBackground();

    update();
}

void VinylWidget::start() {
    timer_.start();
}

void VinylWidget::stop() {
    timer_.stop();
    angle_ = 0;
    update();
}

void VinylWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);

    painter.translate(width() / 2, height() / 2);
    painter.rotate(angle_);
    painter.translate(-width() / 2, -height() / 2);
    painter.drawPixmap(0, 0, image_);
}
