#include <QVBoxLayout>
#include <QLabel>

#include "thememanager.h"

#include <widget/albumview.h>
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
	child_layout->setContentsMargins(20, 0, 20, 0);

	cover_ = new QLabel(this);
	cover_->setMinimumSize(QSize(150, 150));
	cover_->setMaximumSize(QSize(150, 150));

	auto f = font();
	f.setPointSize(15);
	f.setBold(true);
	artist_ = new QLabel(this);
	artist_->setFont(f);
	artist_->setStyleSheet(Q_UTF8("background-color: transparent"));	

	auto cover_spacer2 = new QSpacerItem(50, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	child_layout->addWidget(cover_);
	child_layout->addItem(cover_spacer2);
	child_layout->addWidget(artist_);

	album_view_ = new AlbumView(this);

	default_layout->addLayout(child_layout);
	default_layout->addWidget(album_view_);

	album_view_->hideWidget();
	setArtistId(Qt::EmptyStr, Qt::EmptyStr, -1);

	setStyleSheet(Q_UTF8("background-color: transparent"));
}

void ArtistInfoPage::setArtistId(const QString& artist, const QString& cover_id, int32_t artist_id) {
	artist_->setText(artist);
	album_view_->setFilterByArtistId(artist_id);

	auto cover = &ThemeManager::instance().pixmap().defaultSizeUnknownCover();
	if (auto cache_small_cover = Singleton<PixmapCache>::Get().find(cover_id)) {
		cover = cache_small_cover.value();
	}

	auto small_cover = Pixmap::resizeImage(*cover, QSize(120, 120));
	cover_->setPixmap(small_cover);
}

void ArtistInfoPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
    artist_->setStyleSheet(Q_UTF8("QLabel { color: ") + colorToString(color) + Q_UTF8(";}"));
    album_view_->OnThemeColorChanged(backgroundColor, color);
}
