#include <widget/artistinfopage.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QListWidget>
#include <QPainter>
#include <QScrollArea>
#include <algorithm>

#include <widget/dao/artistdao.h>
#include <thememanager.h>

#include <widget/albumview.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/util/image_util.h>
#include <widget/imagecache.h>

AlbumItem::AlbumItem(const QPixmap& cover,
    const QString& album_title,
    const QString& year,
    const QString& video_id,
    QWidget* parent)
	: QWidget(parent)
	, video_id_(video_id) {
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    auto f = font();
    f.setPointSize(8);

    cover_label_ = new QLabel(this);
    if (!cover.isNull()) {
        QPixmap scaled_cover = cover.scaled(120, 120,
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation);
        cover_label_->setPixmap(scaled_cover);
    }
    main_layout->addWidget(cover_label_, 0, Qt::AlignCenter);

    album_title_label_ = new QLabel(album_title, this);
    album_title_label_->setAlignment(Qt::AlignLeft);
    album_title_label_->setFont(f);
    album_title_label_->setFixedHeight(18);
    main_layout->addWidget(album_title_label_);

    year_label_ = new QLabel(year, this);
    year_label_->setAlignment(Qt::AlignLeft);
    year_label_->setFont(f);
    year_label_->setFixedHeight(18);
    main_layout->addWidget(year_label_);
    setLayout(main_layout);
}

SongItem::SongItem(const QPixmap& album_cover,
    const QString& song_title,
    const QString& album_title,
    const QString& video_id,
    QWidget* parent)
    : QWidget(parent)
    , video_id_(video_id) {
    auto* main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(5, 5, 5, 5);
    main_layout->setSpacing(10);

    cover_label_ = new QLabel(this);
    if (!album_cover.isNull()) {
        auto scaled_cover = album_cover.scaled(35, 35,
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
        cover_label_->setPixmap(scaled_cover);
    }
    main_layout->addWidget(cover_label_);

    auto* text_layout = new QHBoxLayout();
    text_layout->setContentsMargins(0, 0, 0, 0);
    text_layout->setSpacing(2);

    song_label_ = new QLabel(song_title, this);
    song_label_->setStyleSheet("font-size: 10px; color: #dddddd;"_str);

    album_label_ = new QLabel(album_title, this);
    album_label_->setStyleSheet("font-size: 10px; color: #aaaaaa;"_str);

    text_layout->addWidget(song_label_);
    text_layout->addWidget(album_label_);

    auto* spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    text_layout->addSpacerItem(spacer);

    main_layout->addLayout(text_layout);

    setLayout(main_layout);
}

ArtistInfoPage::ArtistInfoPage(QWidget* parent)
    : QFrame(parent) {
    artist_id_ = 0;

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(50, 30, 50, 0);
    main_layout->setSpacing(0);

    top_widget_ = new QWidget(this);
    top_widget_->setFixedHeight(100);

    auto* top_layout = new QVBoxLayout(top_widget_);
    top_layout->setContentsMargins(50, 0, 50, 0);
    top_layout->setSpacing(0);

    auto f = font();

    title_label_ = new QLabel(top_widget_);
    f.setPointSize(24);
    f.setBold(true);
    title_label_->setFont(f);
    top_layout->addWidget(title_label_);

    desc_label_ = new QLabel(top_widget_);
    f.setBold(false);
    f.setPointSize(8);
	desc_label_->setFont(f);
    desc_label_->setMaximumWidth(1000);
    desc_label_->setWordWrap(true);
    desc_label_->setAlignment(Qt::AlignLeft);
    top_layout->addWidget(desc_label_);

    auto* top_spacer = new QSpacerItem(20, 150, QSizePolicy::Expanding, QSizePolicy::Fixed);
	main_layout->addSpacerItem(top_spacer);
    main_layout->addWidget(top_widget_);

    auto* scroll_area = new QScrollArea(this);
    scroll_area->setWidgetResizable(true);
    auto* scroll_content_widget = new QWidget(scroll_area);
    auto* scroll_content_layout = new QVBoxLayout(scroll_content_widget);
    scroll_content_layout->setContentsMargins(50, 0, 0, 0);

    f.setPointSize(16);
	f.setBold(true);

    auto* song_section_label = new QLabel(tr("Song"), scroll_content_widget);
    song_section_label->setFixedHeight(30);
    song_section_label->setFont(f);
    scroll_content_layout->addWidget(song_section_label);

    song_list_ = new QListWidget(scroll_content_widget);
    song_list_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    song_list_->setMaximumHeight(1000);
    scroll_content_layout->addWidget(song_list_);

    auto* album_section_label = new QLabel(tr("Album"), scroll_content_widget);
    album_section_label->setFixedHeight(30);
    album_section_label->setFont(f);
    scroll_content_layout->addWidget(album_section_label);

    album_list_ = new QListWidget(scroll_content_widget);
    album_list_->setObjectName("albumList");
    album_list_->setViewMode(QListView::IconMode);
    album_list_->setFlow(QListView::LeftToRight);
    album_list_->setWrapping(true);
    album_list_->setSpacing(10);
    album_list_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll_content_layout->addWidget(album_list_);

    scroll_content_widget->setLayout(scroll_content_layout);

    //scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_area->setWidget(scroll_content_widget);

    main_layout->addWidget(scroll_area);

    setFrameShape(QFrame::StyledPanel);    
    setLayout(main_layout);
}

void ArtistInfoPage::setTitle(const QString& title) {
    title_label_->setText(title);
}

void ArtistInfoPage::setDescription(const QString& info) {
    desc_label_->setText(info);
}

void ArtistInfoPage::setAristImage(const QPixmap& image) {
    background_ = image;
}

void ArtistInfoPage::setArtistId(int32_t artist_id) {
    artist_id_ = artist_id;
}

void ArtistInfoPage::insertSong(const QString& album,
    const QString& title,
    const QString& video_id,
    const QPixmap& image) {
    auto* item_widget = new SongItem(image,
        title, 
        album, 
        video_id,
        this);
    auto* list_item = new QListWidgetItem(song_list_);
    list_item->setSizeHint(QSize(0, 50));
	song_list_->setItemWidget(list_item, item_widget);
}

void ArtistInfoPage::insertAlbum(const QString& album_title,
    const QString& year,
    const QString& video_id,
    const QPixmap& image) {
    auto* album_item = new AlbumItem(
        image,
        album_title,
        year,
        video_id,
        this);
    auto* list_item = new QListWidgetItem(album_list_);
    list_item->setSizeHint(QSize(150, 180));
    album_list_->setItemWidget(list_item, album_item);
}

void ArtistInfoPage::reload() {
    auto cover_id = qDaoFacade.artist_dao.getArtistCoverId(artist_id_);
    auto image = qImageCache.getOrDefault(kEmptyString, cover_id);
    setAristImage(image);
}

void ArtistInfoPage::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (background_.isNull()) {
        return;
    }

    QRect top_rect = top_widget_->geometry();

    auto w = top_rect.width();
    auto h = top_rect.height();

    QSize resize(w, h);

    auto scaled_bg = background_.scaled(resize,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation);

    while (scaled_bg.width() > 1000) {        
        scaled_bg = background_.scaled(resize,
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation);
        resize.setWidth(resize.width() - 10);
    }	

    scaled_bg = scaled_bg.copy(0, 0, scaled_bg.width(), 280);

    auto bw = scaled_bg.width();
    auto bh = scaled_bg.height();

    int x = top_rect.x() + (top_rect.width() - scaled_bg.width()) / 2;
    int y = top_rect.y() + (top_rect.height() - scaled_bg.height()) / 2;
    painter.drawPixmap(QPoint(x, 0), scaled_bg);

    constexpr auto kGradientHeight = 10;

    int start_y = top_rect.bottom() - 250 - kGradientHeight;

    QLinearGradient grad(
        QPointF(0, start_y),
        QPointF(0, top_rect.bottom())
    );
    grad.setColorAt(0.0, QColor(0, 0, 0, 0));
    grad.setColorAt(1.0, QColor(0, 0, 0, 255));

    QRect gradient_rect(
        top_rect.x(),
        start_y,
        top_rect.width(),
        top_rect.bottom()
    );

    painter.fillRect(gradient_rect, grad);
}
