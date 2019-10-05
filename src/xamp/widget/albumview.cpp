#include <QPainter>

#include "str_utilts.h"
#include "albumview.h"

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
	: QStyledItemDelegate(parent) {
}

void AlbumViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	if (!index.isValid()) {
		return;
	}

	auto rect = option.rect;
	auto x = rect.x() + 10;
	auto y = rect.y() + 10;

	QStyledItemDelegate::paint(painter, option, index);
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	return QStyledItemDelegate::sizeHint(option, index);	
}

AlbumView::AlbumView(QWidget* parent)
	: QListView(parent)
	, model_(this) {
	model_.setRootPath(QDir::homePath());
	model_.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
	model_.setReadOnly(true);

	setModel(&model_);

	setUniformItemSizes(true);
	setFlow(QListView::LeftToRight);
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setFrameStyle(QFrame::NoFrame);
	setIconSize(QSize(120, 120));
	setStyleSheet(Q_UTF8("background-color: transparent"));
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setItemDelegate(new AlbumViewStyledDelegate());

	(void)QObject::connect(this, &QListView::clicked, [this](const QModelIndex& index) {
		auto file_path = model_.fileInfo(index).absoluteFilePath();
		setRootIndex(model_.setRootPath(file_path));
		});
}
