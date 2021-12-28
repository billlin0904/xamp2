#include <QImageReader>
#include <QDirIterator>
#include <QThreadPool>
#include <QPaintEvent>
#include <QPainter>

#include <thememanager.h>
#include <widget/coverflowwidget.h>

constexpr auto COVERFLOW_IMAGE_WIDTH = 250;
constexpr auto COVERFLOW_IMAGE_HEIGHT = 250;

static QImage* mirrorImage(const QImage* image) {
	QImage* tmpImage = new QImage(image->mirrored(false, true));

	QPoint p1, p2;
	p2.setY(tmpImage->height());

	QLinearGradient gradient(p1, p2);
	gradient.setColorAt(0, QColor(0, 0, 0, 100));
	gradient.setColorAt(1, Qt::transparent);

	QPainter p(tmpImage);
	p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	p.fillRect(0, 0, tmpImage->width(), tmpImage->height(), gradient);
	p.end();

	return(tmpImage);
}

CoverFlowImageLoader::CoverFlowImageLoader(const QImage& image)
	: image_(image) {
}

CoverFlowImageLoader::CoverFlowImageLoader(const QString& path)
	: path_(path) {
}

CoverFlowImageLoader::~CoverFlowImageLoader() {
}

void CoverFlowImageLoader::run() {
	auto cover_size = ThemeManager::instance().getCacheCoverSize();
	if (!path_.isEmpty()) {
		QImageReader reader(path_);
		reader.setScaledSize(cover_size);
		image_ = reader.read();
	}
	else if (!image_.isNull()) {
		image_ = image_.scaled(cover_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
	}
	emit completed(image_);
}

CoverFlowItemsLoader::CoverFlowItemsLoader(const QDir& directory)
	: dir_(directory) {
}

CoverFlowItemsLoader::~CoverFlowItemsLoader() {
}

void CoverFlowItemsLoader::run() {
	QDirIterator itr(dir_);
	while (itr.hasNext()) {
		emit completedItem(itr.next());
	}
}

CoverFlowItem::CoverFlowItem(const QString& title, const QString& path, QObject* parent)
	: QObject(parent)
	, title_(title)
	, path_(path) {
}

CoverFlowItem::~CoverFlowItem() {
}

const QImage* CoverFlowItem::image() const {
	return image_.isNull() ? nullptr : &image_;
}

QString CoverFlowItem::path() const {
	return path_;
}

QString CoverFlowItem::title() const {
	return title_;
}

void CoverFlowItem::setImage(const QImage& image) {
	image_ = image;
	emit imageChanged();
}

CoverFlowWidget::CoverFlowWidget(QWidget* parent)
	: QWidget(parent)
	, selected_(-1) {
	init();
	setAttribute(Qt::WA_OpaquePaintEvent);	
}

void CoverFlowWidget::init() {
	empty_item_ = QImage(COVERFLOW_IMAGE_WIDTH, COVERFLOW_IMAGE_HEIGHT,
		QImage::Format_ARGB32_Premultiplied);
	QLinearGradient gradient(0, 0, 0, COVERFLOW_IMAGE_HEIGHT);
	gradient.setColorAt(0, Qt::black);
	gradient.setColorAt(1, QColor(0x55, 0x55, 0x55));
	QPainter p(&empty_item_);
	p.setBrush(gradient);
	p.setPen(QPen(QColor(0x40, 0x40, 0x40), 4));
	p.drawRect(empty_item_.rect());
	p.end();
}

void CoverFlowWidget::addItem(const QString& path) {
	QFileInfo fileInfo(path);
	addItem(fileInfo.baseName(), path);
}

void CoverFlowWidget::addItem(const QString& title, const QString& path) {
	auto loader = new CoverFlowImageLoader(path);
	auto item = new CoverFlowItem(title, path, this);
	(void) QObject::connect(loader, SIGNAL(completed(const QImage&)),
		item, SLOT(setImage(const QImage&)));
	(void) QObject::connect(item, SIGNAL(imageChanged()), this, SLOT(update()));
	items_.append(item);
	QThreadPool::globalInstance()->start(loader);
}

void CoverFlowWidget::addItems(const QString& path) {
	addItems(QDir(path));
}

void CoverFlowWidget::addItems(const QDir& directory) {
	auto loader = new CoverFlowItemsLoader(directory);
	(void) QObject::connect(loader, SIGNAL(completedItem(const QString&)),
		this, SLOT(addItem(const QString&)));
	QThreadPool::globalInstance()->start(loader);
}

void CoverFlowWidget::showNext() {
	if ((selected_ + 1) < items_.size()) {
		selected_++;
		update();
	}
}

void CoverFlowWidget::showPrevious() {
	if (selected_ > 0) {
		selected_--;
		update();
	}
}

void CoverFlowWidget::showAtIndex(int index) {
	if (index >= items_.size()) {
		index = items_.size() - 1;
	}
	if (index < 0) {
		index = 0;
	}
	selected_ = index;
	update();
}

void CoverFlowWidget::paintEvent(QPaintEvent* event) {
	QPainter painter(this);

	painter.setClipRect(event->rect());
	painter.fillRect(rect(), Qt::black);

	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	painter.setRenderHint(QPainter::Antialiasing);

	if (items_.size() > 0) {
		drawItemsBeforeSelected(&painter);
		drawItemsAfterSelected(&painter);
		drawSelectedItem(&painter);
	}
}

const QImage* CoverFlowWidget::itemImage(int index) const {
	auto img = items_.at(index)->image();
	return(img != NULL ? img : &empty_item_);
}

void CoverFlowWidget::mouseReleaseEvent(QMouseEvent* event) {
	auto img = itemImage(selected_);

	int x = (width() / 2) - img->width() / 2;
	if (event->x() >= x && event->x() <= (x + img->width())) {
		// You've Clicked on the Central Item
		qDebug("Central Item Clicked");
	}
	else if (event->x() < x) {
		showPrevious();
	}
	else {
		showNext();
	}
}

void CoverFlowWidget::keyPressEvent(QKeyEvent* event) {
	if (event->key() == Qt::Key_Left) {
		if (event->modifiers() == Qt::ControlModifier)
			showAtIndex(selected_ - 5);
		else
			showPrevious();
	}
	else if (event->key() == Qt::Key_Right) {
		if (event->modifiers() == Qt::ControlModifier)
			showAtIndex(selected_ + 5);
		else
			showNext();
	}
}

void CoverFlowWidget::drawSelectedItem(QPainter* painter) {
	auto item = items_.at(selected_);
	auto img = itemImage(selected_);

	auto cw = width() / 2;
	auto wh = height();
	auto h = img->height();
	auto w = img->width();

	// Draw Image
	drawItemAt(painter, cw, ((wh / 2) - (h / 4)), img, w, h, 0);

	painter->save();
	painter->setPen(Qt::white);

	// Draw Title
	painter->setFont(QFont(painter->font().family(), 12, QFont::Bold));
	auto titleWidth = painter->fontMetrics().width(item->title());
	painter->drawText(cw - titleWidth / 2, wh - 30, item->title());

	// Draw Notes
	painter->setFont(QFont(painter->font().family(), 10, QFont::Normal));
	auto notesWidth = painter->fontMetrics().width(item->path());
	painter->drawText(cw - notesWidth / 2, wh - 15, item->path());

	painter->restore();
}

void CoverFlowWidget::drawItemsAfterSelected(QPainter* painter) {
	auto imgSelected = itemImage(selected_);

	auto winWidth = width();
	QPoint ptSelected((winWidth / 2) + (imgSelected->width() / 2),
		(height() / 2) - (imgSelected->height() / 4));

	int widthAvail = winWidth - ptSelected.x();
	int endIdx = (widthAvail / 40);
	int availItems = items_.size() - (selected_ + 1);
	if (endIdx > availItems) endIdx = availItems;

	for (int i = 0, idx = selected_ + endIdx; i < endIdx; ++i, --idx) {
		auto img = itemImage(idx);

		auto x = ptSelected.x() + ((endIdx - i) * 40);
		drawItemAt(painter, x, ptSelected.y(), img,
			img->width() - 32, img->height() - 32, 55);
	}
}

void CoverFlowWidget::drawItemsBeforeSelected(QPainter* painter) {
	auto imgSelected = itemImage(selected_);

	QPoint ptSelected((width() / 2) - (imgSelected->width() / 2),
		(height() / 2) - (imgSelected->height() / 4));

	auto widthAvail = ptSelected.x();
	auto startIdx = selected_ - (widthAvail / 40);
	if (startIdx < 0) startIdx = 0;

	for (int i = 0, idx = startIdx; idx < selected_; ++i, ++idx) {
		auto img = itemImage(idx);

		auto x = widthAvail - ((selected_ - idx) * 40);
		drawItemAt(painter, x, ptSelected.y(), img,
			img->width() - 32, img->height() - 32, -55);
	}
}

void CoverFlowWidget::drawItemAt(QPainter* painter, int x, int y,
	const QImage* img, int w, int h, int angle) {
	painter->save();

	// Get Image Reflection
	auto imageKey = img->cacheKey();
	QImage* mirrorImg;

	if (mirror_cache_.contains(imageKey)) {
		mirrorImg = mirror_cache_[imageKey];
	}
	else {
		mirrorImg = mirrorImage(img);
		mirror_cache_.insert(imageKey, mirrorImg);
	}

	// Setup Image Transform
	QTransform transform;
	transform.scale(qreal(w) / img->width(), qreal(h) / img->height());
	transform.rotate(angle, Qt::YAxis);
	painter->setTransform(transform * QTransform().translate(x, y), true);

	QPointF pt(-img->width() / 2, -img->height() / 2);
	QPointF pt2(pt.x(), img->height() / 2);

	// Draw Image Reflection
	painter->save();
	painter->setCompositionMode(QPainter::CompositionMode_Source);
	painter->drawImage(pt2, *mirrorImg);
	painter->restore();

	// Draw Image
	painter->drawImage(pt, *img);

	painter->restore();
}