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
	auto default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 20, 0, 0);

	auto child_layout = new QHBoxLayout();
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
		ActionMap<ArtistInfoPage, std::function<void()>> action_map(this);
		action_map.addAction(tr("Change artist image"), [=]() {
			auto file_name = QFileDialog::getOpenFileName(this,
				tr("Open file"),
				Qt::EmptyString,
				tr("Music Files *.jpg *.jpeg *.png"));
			cover_id_ = Singleton<PixmapCache>::GetInstance().addOrUpdate(QPixmap(file_name));
			Singleton<Database>::GetInstance().updateArtistCoverId(artist_id_, cover_id_);
			setArtistId(artist_->text(), cover_id_, artist_id_);
			});		
		action_map.exec(pt);
		});

	auto f = font();
	f.setPointSize(15);
	f.setBold(true);
	artist_ = new QLabel(this);
	artist_->setFont(f);
	artist_->setStyleSheet(Q_UTF8("background-color: transparent"));

	auto title = new QLabel();
	title->setStyleSheet(Q_UTF8("background-color: transparent"));
	title->setObjectName(QString::fromUtf8("label_2"));
	title->setText(tr("Artist Information"));
	f.setPointSize(18);
	title->setFont(f);
	default_layout->addWidget(title);

	auto cover_spacer2 = new QSpacerItem(50, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	child_layout->addWidget(cover_);
	child_layout->addItem(cover_spacer2);
	child_layout->addWidget(artist_);

	album_view_ = new AlbumView(this);

	default_layout->addLayout(child_layout);
	default_layout->addWidget(album_view_);

	album_view_->hideWidget();
	setArtistId(Qt::EmptyString, Qt::EmptyString, -1);

	setStyleSheet(Q_UTF8("background-color: transparent"));
}

QPixmap ArtistInfoPage::getArtistImage(QPixmap const* cover) const {
	return  Pixmap::roundImage(Pixmap::resizeImage(*cover, cover_->size()), Pixmap::kSmallImageRadius);
}

void ArtistInfoPage::setArtistId(const QString& artist, const QString& cover_id, int32_t artist_id) {
	artist_->setText(artist);
	album_view_->setFilterByArtistId(artist_id);

	auto cover = &ThemeManager::instance().pixmap().defaultSizeUnknownCover();
	if (auto cache_small_cover = Singleton<PixmapCache>::GetInstance().find(cover_id)) {
		cover = cache_small_cover.value();
	}

	const auto small_cover = getArtistImage(cover);
	cover_->setPixmap(small_cover);

	artist_id_ = artist_id;
	cover_id_ = cover_id;
}

void ArtistInfoPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
    artist_->setStyleSheet(Q_UTF8("QLabel { color: ") + colorToString(color) + Q_UTF8(";}"));
    album_view_->OnThemeColorChanged(backgroundColor, color);
}
