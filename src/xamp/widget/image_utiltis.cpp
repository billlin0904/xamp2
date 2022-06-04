#include <base/threadpool.h>

#include <QBuffer>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>

#include <widget/image_utiltis.h>

namespace Pixmap {

QPixmap resizeImage(const QPixmap& source, const QSize& size, bool is_aspect_ratio) {
	return source.scaled(size,
		is_aspect_ratio ? Qt::KeepAspectRatioByExpanding
		: Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation);
}

std::vector<uint8_t> getImageDate(const QPixmap& source) {
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	source.save(&buffer, "JPG");
	return { bytes.constData(), bytes.constData() + bytes.size() };
}

QPixmap roundImage(const QPixmap& src, int32_t radius) {
	return roundImage(src, src.size(), radius);
}

QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius) {
	QPixmap result(size);
	const QPixmap pixmap(src);

	result.fill(Qt::transparent);

	QPainter painter(&result);
	QPainterPath painter_path;
	const QRect rect(0, 0, size.width(), size.height());

	painter_path.addRoundedRect(rect, radius, radius);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.setClipPath(painter_path);
	painter.setBrush(QBrush(QColor(249, 249, 249)));
	painter.drawPixmap(rect, resizeImage(pixmap, size, true));
	return result;
}

}
