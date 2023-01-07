#include <QPainter>
#include <QBuffer>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>

#include <widget/image_utiltis.h>

namespace ImageUtils {

QPixmap scaledImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio) {
	const auto scaled_size = source.size() * 2;
	const auto mode = is_aspect_ratio ? Qt::KeepAspectRatioByExpanding
		                  : Qt::IgnoreAspectRatio;

	return source.scaled(scaled_size, mode)
		.scaled(size, mode, Qt::SmoothTransformation);
}

QByteArray convert2ByteArray(const QPixmap& source) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	source.save(&buffer, "JPG");
	return bytes;
}

std::vector<uint8_t> convert2Vector(const QPixmap& source) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	source.save(&buffer, "JPG");
	return { bytes.constData(), bytes.constData() + bytes.size() };
}

QPixmap roundImage(const QPixmap& src, int32_t radius) {
	return roundImage(src, src.size(), radius);
}

QPixmap roundDarkImage(QSize size, int32_t alpha, int32_t radius) {
	QColor color = Qt::black;
	color.setAlpha(alpha);
	const QRect darker_rect(0, 0, size.width(), size.height());

	QPixmap result(size);
	result.fill(Qt::transparent);

	QPainter painter(&result);
	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHints(QPainter::TextAntialiasing, true);

	QPainterPath painter_path;
	painter_path.addRoundedRect(darker_rect, kImageRadius, kImageRadius);
	painter.setClipPath(painter_path);
	painter.fillPath(painter_path, QBrush(color));
	return result;
}

QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius) {
	QPixmap result(size);
	const QPixmap pixmap(src);

	result.fill(Qt::transparent);

	QPainter painter(&result);
	QPainterPath painter_path;
	const QRect rect(0, 0, size.width(), size.height());

	painter_path.addRoundedRect(rect, radius, radius);
	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHints(QPainter::TextAntialiasing, true);
	painter.setClipPath(painter_path);
	painter.setBrush(QBrush(QColor(249, 249, 249)));
	if (src.size() != size) {
		painter.drawPixmap(rect, scaledImage(pixmap, size, true));
	} else {
		painter.drawPixmap(rect, pixmap);
	}
	return result;
}

}
