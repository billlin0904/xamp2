#include <QPainter>
#include <QBuffer>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>

#include <widget/image_utiltis.h>

Q_WIDGETS_EXPORT void qt_blurImage(QPainter* p, QImage& blurImage, qreal radius, bool quality,
	bool alphaOnly, int transposed = 0);

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

QImage blurImage(const QPixmap& source, QSize size) {
	const int radius = qMax(20, qMin(size.width(), size.height()) / 5);

	const QSize scaled_size(size.width() + radius, size.height() + radius);
	auto pixmap = scaledImage(source, scaled_size);
	auto img = pixmap.toImage();

	QPainter painter(&pixmap);
	qt_blurImage(&painter, img, radius, true, false);

	// note: 去除模糊後的黑邊.
	auto clip_size = qMin(source.size().width(), source.size().height());
	clip_size = qMin(clip_size / 2, radius);
	auto clip = pixmap.copy(clip_size,
		clip_size, 
		pixmap.width() - clip_size * 2,
		pixmap.height() - clip_size * 2);

	return pixmap.copy(clip_size,
		clip_size, pixmap.width() - clip_size * 2,
		pixmap.height() - clip_size * 2).toImage();
}

int sampleImageBlur(const QImage& image, int blur_alpha) {
	const auto w = image.width();
	const auto h = image.height();

	qint64 rgb_sum = 0;
	constexpr int m = 16;

	for (int y = 0; y < m; y++) {
		for (int x = 0; x < m; x++) {
			QColor c = image.pixelColor(w * x / m, h * x / m);
			rgb_sum += c.red() + c.green() + c.blue();
		}
	}

	const auto addin = static_cast<int>(rgb_sum * blur_alpha / (255 * 3 * m * m));
	return qMin(255, blur_alpha + addin);
}

}
