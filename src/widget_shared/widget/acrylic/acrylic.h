//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once
#include <QColor>
#include <QLabel>
#include <QPainterPath>

#include <widget/widget_shared_global.h>

class QWidget;

class XAMP_WIDGET_SHARED_EXPORT AcrylicRender {
public:
    AcrylicRender(QWidget* device,         
        int blur_radius = 13,
        const QColor& tint_color = QColor(242, 242, 242, 150),
        const QColor& luminosity_color = QColor(255, 255, 255, 10),
        double noise_opacity = 0.03);

    void setBlurRadius(int radius);

    void setLuminosityColor(const QColor& color);

    void setTintColor(const QColor& color);

    void grabImage(const QRect& rect);

    void setImage(const QPixmap& image);

    void setClipPath(const QPainterPath& path);

    QImage textureImage() const;

    void paint() const;
private:
    int blur_radius_;
    double noise_opacity_;
    QWidget* widget_;
    QColor tint_color_;
    QColor luminosity_color_;
    QImage noise_image_;
    QPixmap original_image_;
    QPixmap image_;
    QPainterPath clip_path_;
};

class AcrylicTextureLabel : public QLabel {
public:
	explicit AcrylicTextureLabel(const QColor& tint_color = QColor(242, 242, 242, 150),
	                             const QColor& luminosity_color = QColor(255, 255, 255, 10),
	                             double noise_opacity = 0.03,
	                             QWidget* parent = nullptr);

    void setTintColor(const QColor& color);

    void paintEvent(QPaintEvent* event) override;

private:
    QColor tint_color_;
    QColor luminosity_color_;
    double noise_opacity_;
    QImage noise_image_;
};

class AcrylicLabel : public QLabel {
public:
    explicit AcrylicLabel(int blur_radius = 13,
        const QColor& tint_color = QColor(242, 242, 242, 150),
        const QColor& luminosity_color = QColor(255, 255, 255, 10),
        QWidget* parent = nullptr);

    explicit AcrylicLabel(QWidget* parent = nullptr);

    void setImage(const QPixmap& image);

    void setTintColor(const QColor& color);

    void resizeEvent(QResizeEvent* event) override;
private:
    int blur_radius_;
    QPixmap blur_pixmap_;
    AcrylicTextureLabel acrylic_texture_label_;
};