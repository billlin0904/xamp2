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

void AlbumItem::setCover(const QPixmap& cover) {
    QPixmap scaled_cover = cover.scaled(120, 120,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation);
    cover_label_->setPixmap(scaled_cover);
}

QString AlbumItem::albumTitle() const {
    return album_title_label_->text();
}

SongItem::SongItem(const QPixmap& album_cover,
    const QString& song_title,
    const QString& artist,
    const QString& album_title,
    const QString& video_id,
    QWidget* parent)
    : QWidget(parent)
	, artist_(artist)
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

QString SongItem::albumTitle() const {
    return album_label_->text();
}

QString SongItem::songTitle() const {
    return song_label_->text();
}

ArtistInfoPage::ArtistInfoPage(QWidget* parent)
    : QFrame(parent) {
    artist_id_ = 0;

    toggle_btn_ = new QPushButton(tr(" Show More"), this);
    toggle_btn_->setVisible(false);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(50, 30, 50, 0);
    main_layout->setSpacing(0);

    top_widget_ = new QWidget(this);

    auto* top_layout = new QVBoxLayout(top_widget_);
    top_layout->setContentsMargins(50, 0, 50, 0);
    top_layout->setSpacing(0);

    auto f = font();

    title_label_ = new QLabel(top_widget_);
    f.setPointSize(24);
    f.setBold(true);
    title_label_->setFont(f);
    top_layout->addWidget(title_label_);

    auto* desc_layout = new QVBoxLayout();
    desc_layout->setContentsMargins(0, 0, 0, 0);
    desc_layout->setSpacing(2);

    desc_label_ = new QLabel(top_widget_);
    f.setBold(false);
    f.setPointSize(8);
	desc_label_->setFont(f);
    desc_label_->setMaximumWidth(1000);
    desc_label_->setWordWrap(true);
    desc_label_->setAlignment(Qt::AlignLeft);

    toggle_btn_->setFont(f);

    desc_layout->addWidget(desc_label_);
    desc_layout->addWidget(toggle_btn_, 0, Qt::AlignLeft);

	//top_layout->addWidget(desc_label_);
    //top_layout->addWidget(toggle_btn_);
    top_layout->addLayout(desc_layout);

    auto* top_spacer = new QSpacerItem(20, 280, QSizePolicy::Expanding, QSizePolicy::Fixed);
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

    scroll_area->setWidget(scroll_content_widget);

    main_layout->addWidget(scroll_area);

    (void)QObject::connect(album_list_, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
		auto* album_item = qobject_cast<AlbumItem*>(album_list_->itemWidget(item));
        auto title = album_item->albumTitle();
        auto video_id = album_item->videoId();
		emit browseAlbum(title, video_id);
        });

    (void)QObject::connect(toggle_btn_, &QPushButton::clicked, [this]() {
        if (!is_expanded_) {
            desc_label_->setText(full_description_);
            toggle_btn_->setText(tr(" Show Less"));
            is_expanded_ = true;
        } else {
            desc_label_->setText(short_description_);
            toggle_btn_->setText(tr(" Show More"));
            is_expanded_ = false;
        }
        });

    song_section_label->hide();
    song_list_->hide();

    setFrameShape(QFrame::StyledPanel);    
    setLayout(main_layout);
}

void ArtistInfoPage::setTitle(const QString& title) {
    title_label_->setText(title);
}

void ArtistInfoPage::setDescription(const QString& info) {
    full_description_ = info;
    constexpr auto kMaxTextLimit = 200;

    if (full_description_.size() > kMaxTextLimit) {
        short_description_ = full_description_.left(kMaxTextLimit) + "..."_str;
        desc_label_->setText(short_description_);
        toggle_btn_->setVisible(true);
        is_expanded_ = false;
        toggle_btn_->setText(tr(" Show More"));
    } else {
        short_description_ = info;
        desc_label_->setText(info);
        toggle_btn_->setVisible(false);
        is_expanded_ = false;
    }
}

void ArtistInfoPage::setArtistImage(const QPixmap& image) {
    background_ = image;
}

void ArtistInfoPage::setArtistId(int32_t artist_id) {
    artist_id_ = artist_id;
}

void ArtistInfoPage::insertSong(const QString& album,
    const QString& title,
    const QString& artist,
    const QString& video_id,
    const QPixmap& image) {
    auto* item_widget = new SongItem(image,
        title,
        artist,
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
    setArtistImage(image);
    for (auto i = 0; i < album_list_->count(); ++i) {
        auto *widget = dynamic_cast<AlbumItem*>(album_list_->itemWidget(album_list_->item(i)));
        if (widget) {
            auto image = qImageCache.getOrDefault(kEmptyString, widget->videoId());
            widget->setCover(image);
		}
    }
}

void ArtistInfoPage::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (background_.isNull()) {
        return;
    }

    constexpr auto kBackgroundHeight = 450;

    // 建立一個繪製範圍 topAreaRect
    QRect topAreaRect(0, 0, width(), kBackgroundHeight);

    // 將圖縮放到這個 topAreaRect (或 KeepAspectRatioByExpanding 之類)
    auto scaledBg = background_.scaled(
        topAreaRect.size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
    );

    // 若想只取前面 280px 高度，可做 copy，但要確保 scaledBg 足夠大
    scaledBg = scaledBg.copy(0, 0, scaledBg.width(), kBackgroundHeight);

    // 水平置中: 讓 x 往左 or 往右偏移
    int x = (width() - scaledBg.width()) / 2;
    painter.drawPixmap(QPoint(x, 0), scaledBg);

    constexpr int kGradientHeight = kBackgroundHeight;
    int startY = kBackgroundHeight - kGradientHeight;

    QLinearGradient grad(
        QPointF(0, startY),
        QPointF(0, kBackgroundHeight)
    );
    grad.setColorAt(0.0, QColor(0, 0, 0, 0));   // 從透明
    grad.setColorAt(1.0, QColor(0, 0, 0, 255)); // 漸變到黑

    QRect gradientRect(0, startY, width(), kGradientHeight);
    painter.fillRect(gradientRect, grad);
}
