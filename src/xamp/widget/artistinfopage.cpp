#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>

#include <widget/database.h>
#include "thememanager.h"

#include <widget/albumview.h>
#include <widget/actionmap.h>
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/artistinfopage.h>

ArtistInfoPage::ArtistInfoPage(QWidget* parent)
	: QFrame(parent) {
	auto* default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 20, 0, 0);

	auto* child_layout = new QHBoxLayout();
	child_layout->setSpacing(0);
	child_layout->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	child_layout->setContentsMargins(20, 0, 20, 0);

	cover_ = new QLabel(this);
	cover_->setMinimumSize(QSize(120, 120));
	cover_->setMaximumSize(QSize(120, 120));

	cover_->setContextMenuPolicy(Qt::CustomContextMenu);	
	(void)QObject::connect(cover_, &QLabel::customContextMenuRequested, [this](auto pt) {
		if (artist_id_ == -1) {
			return;
		}
        ActionMap<ArtistInfoPage> action_map(this);
		action_map.addAction(tr("Change artist image"), [=]() {
			const auto file_name = QFileDialog::getOpenFileName(this,
			                                                    tr("Open file"),
			                                                    Qt::EmptyString,
			                                                    tr("Music Files *.jpg *.jpeg *.png"),
                nullptr);
			cover_id_ = qPixmapCache.savePixamp(QPixmap(file_name));
			qDatabase.updateArtistCoverId(artist_id_, cover_id_);
			setArtistId(artist_->text(), cover_id_, artist_id_);
			});		
		action_map.exec(pt);
		});

	auto f = font();
	f.setPointSize(30);
	f.setBold(true);
	artist_ = new QLabel(this);
	artist_->setFont(f);
	artist_->setStyleSheet(Q_TEXT("background-color: transparent"));

	auto* title = new QLabel();
	title->setStyleSheet(Q_TEXT("background-color: transparent"));
	title->setObjectName(QString::fromUtf8("label_2"));
	title->setText(tr("Artists"));
	f.setPointSize(35);
	title->setFont(f);
	default_layout->addWidget(title);

	f.setBold(false);
    f.setPointSize(20);

	albums_ = new QLabel(this);
	albums_->setFixedSize(QSize(16777215, 64));
	albums_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	albums_->setFont(f);
	albums_->setStyleSheet(Q_TEXT("background-color: transparent;"));

	tracks_ = new QLabel(this);
	tracks_->setFixedSize(QSize(16777215, 64));
	tracks_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	tracks_->setFont(f);
	tracks_->setStyleSheet(Q_TEXT("background-color: transparent;"));

	durtions_ = new QLabel(this);
	durtions_->setFixedSize(QSize(16777215, 64));
	durtions_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	durtions_->setFont(f);
	durtions_->setStyleSheet(Q_TEXT("background-color: transparent;"));

	auto* artist_layout = new QHBoxLayout();
	artist_layout->setSpacing(0);
	artist_layout->setContentsMargins(0, 0, 0, 0);

	artist_layout->addWidget(albums_);
	artist_layout->addWidget(tracks_);
	artist_layout->addWidget(durtions_);
	artist_layout->setStretch(0, 0);
	artist_layout->setStretch(1, 2);
	artist_layout->setStretch(2, 2);	

	auto* cover_spacer2 = new QSpacerItem(50, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	child_layout->addWidget(cover_);
	child_layout->addItem(cover_spacer2);
	child_layout->addWidget(artist_);

	album_view_ = new AlbumView(this);

	default_layout->addLayout(child_layout);
	default_layout->addLayout(artist_layout);
	default_layout->addWidget(album_view_);	

	album_view_->hideWidget();
	setArtistId(Qt::EmptyString, Qt::EmptyString, -1);

	setStyleSheet(Q_TEXT("background-color: transparent"));

	setAlbumCount(0);
	setTracks(0);
	setTotalDuration(0);
}

QPixmap ArtistInfoPage::getArtistImage(QPixmap const* cover) const {
	return  Pixmap::roundImage(Pixmap::resizeImage(*cover, cover_->size()), Pixmap::kPlaylistImageRadius);
}

void ArtistInfoPage::setArtistId(const QString& artist, const QString& cover_id, int32_t artist_id) {
	artist_->setText(artist);
	album_view_->setFilterByArtistId(artist_id);

    const auto* cover = &qTheme.pixmap().defaultSizeUnknownCover();
	if (auto const * cache_small_cover = qPixmapCache.find(cover_id)) {
		cover = cache_small_cover;
	}

	const auto small_cover = getArtistImage(cover);
	cover_->setPixmap(small_cover);

	artist_id_ = artist_id;
	cover_id_ = cover_id;

	if (auto artist_stats = qDatabase.getArtistStats(artist_id)) {
		setAlbumCount(artist_stats.value().albums);
		setTracks(artist_stats.value().tracks);
		setTotalDuration(artist_stats.value().durations);
	}
}

void ArtistInfoPage::onThemeChanged(QColor backgroundColor, QColor color) {
    artist_->setStyleSheet(Q_TEXT("QLabel { color: ") + colorToString(color) + Q_TEXT(";}"));
	album_view_->onThemeChanged(backgroundColor, color);
}

void ArtistInfoPage::setAlbumCount(int32_t album_count) {
	albums_->setText(tr("%1 Albums").arg(album_count));
}

void ArtistInfoPage::setTracks(int32_t tracks) {
	tracks_->setText(tr("%1 Songs").arg(tracks));
}

void ArtistInfoPage::setTotalDuration(double durations) {
	durtions_->setText(msToString(durations));
}
