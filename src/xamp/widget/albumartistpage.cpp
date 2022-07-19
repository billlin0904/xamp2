#include <QVBoxLayout>
#include <QLabel>

#include <widget/str_utilts.h>
#include <widget/albumview.h>
#include <thememanager.h>
#include <widget/albumartistpage.h>

AlbumArtistPage::AlbumArtistPage(QWidget* parent)
	: QFrame(parent)
	, album_view_(new AlbumView(this)) {
	auto* verticalLayout_2 = new QVBoxLayout(this);

	verticalLayout_2->setSpacing(0);
	verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

	auto f = font();
	f.setPointSize(35);
	f.setBold(true);

	auto* title = new QLabel();
	title->setStyleSheet(Q_TEXT("background-color: transparent"));
	title->setObjectName(QString::fromUtf8("label_2"));
	title->setText(tr("Albums"));
	title->setFont(f);
	verticalLayout_2->addWidget(title);
	
	auto* default_layout = new QHBoxLayout();
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);

	default_layout->addWidget(album_view_);

	default_layout->setStretch(1, 2);
	verticalLayout_2->addLayout(default_layout);

	(void)QObject::connect(album_view_, &AlbumView::removeAll,
		this, &AlbumArtistPage::refreshOnece);

	(void)QObject::connect(album_view_, &AlbumView::loadCompleted,
		this, &AlbumArtistPage::refreshOnece);

	setStyleSheet(Q_TEXT("background-color: transparent"));
}

void AlbumArtistPage::refreshOnece() {
	album_view_->refreshOnece();
}

void AlbumArtistPage::onThemeChanged(QColor backgroundColor, QColor color) {
	//setStyleSheet(backgroundColorToString(backgroundColor));
}
