#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QScreen>
#include <QWidget>

#include <widget/str_utilts.h>
#include <widget/acrylic/acrylic.h>
#include <widget/image_utiltis.h>

AcrylicRender::AcrylicRender(QWidget* device, 
                             int blur_radius,
                             const QColor& tint_color,
                             const QColor& luminosity_color,
                             double noise_opacity)
	: blur_radius_(blur_radius)
	, noise_opacity_(noise_opacity)
	, widget_(device)
	, tint_color_(tint_color)
	, luminosity_color_(luminosity_color)
	, noise_image_(QImage(qTEXT(":/xamp/Resource/noise.png"))) {
}

void AcrylicRender::setBlurRadius(int radius) {
    if (radius == blur_radius_)
        return;

    blur_radius_ = radius;
    setImage(original_image_);
}

void AcrylicRender::setTintColor(const QColor& color) {
    tint_color_ = color;
    widget_->update();
}

void AcrylicRender::setLuminosityColor(const QColor& color) {
    luminosity_color_ = color;
    widget_->update();
}

void AcrylicRender::grabImage(const QRect& rect) {
    QScreen* screen = QApplication::screenAt(widget_->window()->pos());
    if (!screen)
        screen = QApplication::screens()[0];

    const int x = rect.x();
    const int w = rect.width();
    const int h = rect.height();
    const int y = rect.y();
    setImage(screen->grabWindow(0, x, y, w, h));
}

void AcrylicRender::setImage(const QPixmap& image) {
    original_image_ = image;
    if (!image.isNull()) {
        image_ = image_utils::gaussianBlur(image, blur_radius_);
    }
    widget_->update();
}

void AcrylicRender::setClipPath(const QPainterPath& path) {
    clip_path_ = path;
    widget_->update();
}

QImage AcrylicRender::textureImage() const {
    QImage texture(64, 64, QImage::Format_ARGB32_Premultiplied);
    texture.fill(luminosity_color_);

    QPainter painter(&texture);
    painter.fillRect(texture.rect(), tint_color_);
    painter.setOpacity(noise_opacity_);
    painter.drawImage(texture.rect(), noise_image_);

    return texture;
}

void AcrylicRender::paint() const {
    QPainter painter(widget_);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!clip_path_.isEmpty()) {
        painter.setClipPath(clip_path_);
    }

    const QPixmap scaled_image = image_.scaled(widget_->size(),
        Qt::KeepAspectRatioByExpanding, 
        Qt::SmoothTransformation);
    painter.drawPixmap(0, 0, scaled_image);

    painter.fillRect(widget_->rect(), QBrush(textureImage()));
}

AcrylicTextureLabel::AcrylicTextureLabel(const QColor& tint_color,
    const QColor& luminosity_color,
    double noise_opacity,
    QWidget* parent)
    : QLabel(parent)
	, tint_color_(tint_color)
	, luminosity_color_(luminosity_color)
	, noise_opacity_(noise_opacity) {
    setAttribute(Qt::WA_TranslucentBackground);
}

void AcrylicTextureLabel::setTintColor(const QColor& color) {
    tint_color_ = color;
    update();
}

void AcrylicTextureLabel::paintEvent(QPaintEvent* event) {
    auto acrylic_texture = QImage(64, 64, QImage::Format_ARGB32_Premultiplied);
    acrylic_texture.fill(luminosity_color_);

    QPainter texture_painter(&acrylic_texture);
    texture_painter.fillRect(acrylic_texture.rect(), tint_color_);
    texture_painter.setOpacity(noise_opacity_);
	texture_painter.drawImage(acrylic_texture.rect(), noise_image_);

    const QBrush acrylic_brush(acrylic_texture);
    QPainter painter(this);
    painter.fillRect(rect(), acrylic_brush);
}

AcrylicLabel::AcrylicLabel(int blur_radius, const QColor& tint_color, const QColor& luminosity_color, QWidget* parent)
	: QLabel(parent)
	, blur_radius_(blur_radius)
	, acrylic_texture_label_(tint_color, luminosity_color) {
}

AcrylicLabel::AcrylicLabel(QWidget* parent)
    : AcrylicLabel(13, QColor(242, 242, 242, 150), QColor(255, 255, 255, 10), parent) {
}

void AcrylicLabel::setImage(const QPixmap& image) {
    setPixmap(image_utils::gaussianBlur(image, blur_radius_));
    adjustSize();
}

void AcrylicLabel::setTintColor(const QColor& color) {
    acrylic_texture_label_.setTintColor(color);
}

void AcrylicLabel::resizeEvent(QResizeEvent* event) {
    QLabel::resizeEvent(event);
    acrylic_texture_label_.resize(size());
    if (blur_pixmap_.isNull() || blur_pixmap_.size() != size()) {
        setImage(image_utils::resizeImage(blur_pixmap_, size()));
    }
}
