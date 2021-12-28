#include <cmath>
#include <array>
#include <algorithm>

#include <base/stl.h>
#include <base/threadpool.h>
#include <base/align_ptr.h>

#include <QBitmap>
#include <QBuffer>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsBlurEffect>

#include <widget/widget_shared.h>
#include <widget/image_utiltis.h>

namespace Pixmap {

static QImage applyBlurEffect(QImage src, qreal radius, int extent = 0) {
	auto effect = new QGraphicsBlurEffect();
	effect->setBlurRadius(radius);

	if (src.isNull()) {
		delete effect;
		return QImage();
	}

	QGraphicsScene scene;
	QGraphicsPixmapItem item;
	item.setPixmap(QPixmap::fromImage(src));
	item.setGraphicsEffect(effect);
	scene.addItem(&item);
	QImage res(src.size() + QSize(extent * 2, extent * 2), QImage::Format_ARGB32);
	res.fill(Qt::transparent);

	QPainter ptr(&res);
	scene.render(&ptr, QRectF(), QRectF(-extent, -extent, src.width() + extent * 2, src.height() + extent * 2));
	delete effect;

	return res;
}

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
  return std::vector<unsigned char>(bytes.constData(), bytes.constData() + bytes.size());
}

QPixmap roundImage(const QPixmap& src, int radius) {
  return roundImage(src, src.size(), radius);
}

QPixmap roundImage(const QPixmap& src, QSize size, int32_t radius) {
  QBitmap mask(size);
  QPainter painter(&mask);

  painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  painter.fillRect(0, 0, size.width(), size.height(), Qt::white);
  painter.setBrush(QColor(0, 0, 0));
  painter.drawRoundedRect(0, 0, size.width(), size.height(), radius, radius);

  auto image = src.scaled(size);
  image.setMask(mask);
  return image;
}

}
