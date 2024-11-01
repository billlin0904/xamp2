#include <widget/artistinfopage.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>

#include <widget/dao/artistdao.h>
#include <thememanager.h>

#include <widget/albumview.h>
#include <widget/actionmap.h>
#include <widget/util/image_util.h>
#include <widget/imagecache.h>

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
	(void)connect(cover_, &QLabel::customContextMenuRequested, [this](auto pt) {
		if (artist_id_ == -1) {
			return;
		}
		ActionMap<ArtistInfoPage> action_map(this);
		action_map.addAction(tr("Change artist image"), [=]() {
			const auto file_name = QFileDialog::getOpenFileName(this,
			                                                    tr("Open file"),
			                                                    kEmptyString,
			                                                    tr("Music Files *.jpg *.jpeg *.png"),
			                                                    nullptr);
			cover_id_ = qImageCache.addImage(QPixmap(file_name));
			artist_dao_.updateArtistCoverId(artist_id_, cover_id_);
			setArtistId(artist_->text(), cover_id_, artist_id_);
		});
		action_map.exec(pt);
	});

	auto f = font();
	f.setPointSize(qTheme.fontSize(30));
	f.setBold(true);
	artist_ = new QLabel(this);
	artist_->setFont(f);
	artist_->setStyleSheet("background-color: transparent"_str);

	auto* title = new QLabel();
	title->setStyleSheet("background-color: transparent"_str);
	title->setObjectName(QString::fromUtf8("label_2"));
	title->setText(tr("Artists"));
	f.setPointSize(qTheme.fontSize(35));
	title->setFont(f);
	default_layout->addWidget(title);

	f.setBold(false);
	f.setPointSize(qTheme.fontSize(20));

	albums_ = new QLabel(this);
	albums_->setFixedSize(QSize(16777215, 64));
	albums_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	albums_->setFont(f);
	albums_->setStyleSheet("background-color: transparent;"_str);

	tracks_ = new QLabel(this);
	tracks_->setFixedSize(QSize(16777215, 64));
	tracks_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	tracks_->setFont(f);
	tracks_->setStyleSheet("background-color: transparent;"_str);

	durations_ = new QLabel(this);
	durations_->setFixedSize(QSize(16777215, 64));
	durations_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	f.setFamily("MonoFont"_str);
	durations_->setFont(f);
	durations_->setStyleSheet("background-color: transparent;"_str);

	auto* artist_layout = new QHBoxLayout();
	artist_layout->setSpacing(0);
	artist_layout->setContentsMargins(0, 0, 0, 0);

	artist_layout->addWidget(albums_);
	artist_layout->addWidget(tracks_);
	artist_layout->addWidget(durations_);
	artist_layout->setStretch(0, 0);
	artist_layout->setStretch(1, 2);
	artist_layout->setStretch(2, 2);

	auto* cover_spacer2 = new QSpacerItem(50, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	child_layout->addWidget(cover_);
	child_layout->addItem(cover_spacer2);
	child_layout->addWidget(artist_);

	album_view_ = new AlbumView(this);
	album_view_->enablePage(false);

	default_layout->addLayout(child_layout);
	default_layout->addLayout(artist_layout);
	default_layout->addWidget(album_view_);

	album_view_->hideWidget();
	setArtistId(kEmptyString, kEmptyString, -1);

	setStyleSheet("background-color: transparent"_str);

	setAlbumCount(0);
	setTracks(0);
	setTotalDuration(0);

	title->hide();
	cover_->hide();
}

QPixmap ArtistInfoPage::getArtistImage(const QPixmap* cover) const {
	return image_util::roundImage(image_util::resizeImage(*cover, cover_->size()), image_util::kPlaylistImageRadius);
}

void ArtistInfoPage::setArtistId(const QString& artist, const QString& cover_id, int32_t artist_id) {
	const QFontMetrics artist_metrics(font());

	artist_->setText(artist_metrics.elidedText(artist, Qt::ElideRight, 300));
	album_view_->filterByArtistId(artist_id);

	const auto cover = qImageCache.getOrAddDefault(cover_id);
	const auto small_cover = getArtistImage(&cover);
	cover_->setPixmap(small_cover);

	artist_id_ = artist_id;
	cover_id_ = cover_id;

	if (const auto artist_stats = artist_dao_.getArtistStats(artist_id)) {
		setAlbumCount(artist_stats.value().albums);
		setTracks(artist_stats.value().tracks);
		setTotalDuration(artist_stats.value().durations);
	}
}

void ArtistInfoPage::setAlbumCount(int32_t album_count) {
	albums_->setText(tr("%1 Albums").arg(album_count));
}

void ArtistInfoPage::setTracks(int32_t tracks) {
	tracks_->setText(tr("%1 Songs").arg(tracks));
}

void ArtistInfoPage::setTotalDuration(double durations) {
	durations_->setText(formatDuration(durations));
}
