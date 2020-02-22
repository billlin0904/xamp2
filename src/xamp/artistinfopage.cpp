#include <QVBoxLayout>

#include "thememanager.h"
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/artistinfopage.h>

ArtistInfoPage::ArtistInfoPage(QWidget* parent)
	: QFrame(parent) {
	auto default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 20, 0, 0);

	auto child_layout = new QHBoxLayout();
	child_layout->setSpacing(0);
	child_layout->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	child_layout->setContentsMargins(20, 60, 20, 60);

	cover_ = new QLabel(this);
	cover_->setMinimumSize(QSize(250, 250));
	cover_->setMaximumSize(QSize(250, 250));

	artist_ = new QLabel(this);

	auto cover_spacer1 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);
	auto cover_spacer2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);
	child_layout->addItem(cover_spacer1);
	child_layout->addWidget(cover_);
	child_layout->addWidget(artist_);
	child_layout->addItem(cover_spacer2);

	album_view_ = new AlbumView(this);

	default_layout->addLayout(child_layout);
	default_layout->addWidget(album_view_);

	album_view_->hideWidget();
	setArtistId(Q_UTF8(""), -1);
}

void ArtistInfoPage::setArtistId(const QString& cover_id, int32_t artist_id) {
	album_view_->setFilterByArtistId(artist_id);

	auto cover = &ThemeManager::pixmap().defaultSizeUnknownCover();
	if (auto cache_small_cover = PixmapCache::Instance().find(cover_id)) {
		cover = cache_small_cover.value();
		cover_->setPixmap(*cover);
	}
	else {
		auto small_cover = Pixmap::resizeImage(*cover, QSize(250, 250));
		cover_->setPixmap(small_cover);
	}
}

void ArtistInfoPage::onTextColorChanged(QColor backgroundColor, QColor color) {
	album_view_->onTextColorChanged(backgroundColor, color);
}