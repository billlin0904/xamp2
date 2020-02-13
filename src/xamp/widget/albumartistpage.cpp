#include <QVBoxLayout>

#include <widget/str_utilts.h>
#include <widget/artistview.h>
#include <widget/albumview.h>
#include <widget/albumartistpage.h>

AlbumArtistPage::AlbumArtistPage(QWidget* parent)
	: QFrame(parent)
	, artist_view_(new ArtistView(this))
	, album_view_(new AlbumView(this)) {
	setStyleSheet(Q_UTF8("background: transparent;"));

	auto default_layout = new QHBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);

	artist_view_->setMaximumSize(QSize(80, 16777215));

	default_layout->addWidget(artist_view_);
	default_layout->addWidget(album_view_);

	default_layout->setStretch(1, 2);

	(void) QObject::connect(artist_view_, &ArtistView::clickedArtist,
		album_view_, &AlbumView::setFilterByArtist);
}

void AlbumArtistPage::refreshOnece() {
	artist_view_->refreshOnece();
	album_view_->refreshOnece();
}