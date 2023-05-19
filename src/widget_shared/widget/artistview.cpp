#include <widget/artistview.h>
#include <widget/albumview.h>
#include <widget/database.h>
#include <widget/albumentity.h>
#include <widget/taglistview.h>

#include <thememanager.h>

#include <QSqlError>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QLabel>
#include <QPropertyAnimation>

enum {
	INDEX_ARTIST,
	INDEX_ARTIST_ID,
	INDEX_COVER_ID,
	INDEX_FIRST_CHAR,
};

ArtistStyledItemDelegate::ArtistStyledItemDelegate(QObject* parent)
	: QStyledItemDelegate(parent) {
}

void ArtistStyledItemDelegate::SetTextColor(QColor color) {
	text_color_ = color;
}

void ArtistStyledItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto* style = option.widget ? option.widget->style() : QApplication::style();

	auto artist = index.model()->data(index.model()->index(index.row(), INDEX_ARTIST)).toString();
	auto artist_id = index.model()->data(index.model()->index(index.row(), INDEX_ARTIST_ID)).toString();
	auto artist_cover_id = index.model()->data(index.model()->index(index.row(), INDEX_COVER_ID)).toString();
	auto first_char = index.model()->data(index.model()->index(index.row(), INDEX_FIRST_CHAR)).toString();

	const auto default_cover_size = qTheme.GetDefaultCoverSize();
	const QRect cover_rect(option.rect.left() + 10,
		option.rect.top() + 10,
		default_cover_size.width(),
		default_cover_size.height());

	painter->setRenderHints(QPainter::Antialiasing, true);
	painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter->setRenderHints(QPainter::TextAntialiasing, true);

	int size = cover_rect.height() - 4;
	QRect rect(cover_rect.x() + 2, cover_rect.y() + 2, size, size);
	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::gray);
	painter->drawEllipse(rect);

	auto font = painter->font();

	if (!artist_cover_id.isEmpty()) {
		auto artist_cover = AlbumViewStyledDelegate::GetCover(qTEXT("thumbnail_"), artist_cover_id);
		painter->drawPixmap(rect, image_utils::RoundImage(artist_cover, size / 2));
	}
	else {
		painter->setPen(Qt::white);
		font.setPointSize(qTheme.GetFontSize(70));
		font.setBold(true);
		painter->setFont(font);
		painter->drawText(rect, Qt::AlignCenter, first_char);
	}

	painter->setPen(text_color_);
	font.setBold(false);
	font.setPointSize(qTheme.GetFontSize(8));
	painter->setFont(font);
	auto album_artist_text_width = default_cover_size.width();
	QRect artist_text_rect(option.rect.left() + 10,
		option.rect.top() + default_cover_size.height() + 15,
		album_artist_text_width,
		15);
	QFontMetrics album_metrics(font);
	painter->drawText(artist_text_rect, Qt::AlignVCenter,
		album_metrics.elidedText(artist, Qt::ElideRight, album_artist_text_width));
}

QSize ArtistStyledItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto result = QStyledItemDelegate::sizeHint(option, index);
	const auto default_cover = qTheme.GetDefaultCoverSize();
	result.setWidth(default_cover.width() + 30);
	result.setHeight(default_cover.height() + 80);
	return result;
}

bool ArtistStyledItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
	if (event->type() == QEvent::MouseButtonPress) {
		auto* mouse_event = static_cast<QMouseEvent*>(event);
		if (mouse_event->button() == Qt::LeftButton) {
			emit EnterAlbumView(index);
		}
	}
	return true;
}

const ConstLatin1String ArtistStyledItemDelegate::kArtistCacheTag(qTEXT("artist_thumbnail"));

ArtistViewPage::ArtistViewPage(QWidget* parent)
	: QFrame(parent) {
	setObjectName(QString::fromUtf8("artistView"));
	setFrameStyle(QFrame::StyledPanel);

	close_button_ = new QPushButton(this);
	close_button_->setObjectName(qTEXT("albumViewPageCloseButton"));
	close_button_->setStyleSheet(qTEXT(R"(
                                         QPushButton#albumViewPageCloseButton {
                                         border-radius: 5px;        
                                         background-color: gray;
                                         }
										 QPushButton#albumViewPageCloseButton:hover {
                                         color: white;
                                         background-color: darkgray;                                         
                                         }
                                         )"));
	close_button_->setFixedSize(qTheme.GetTitleButtonIconSize() * 2);
	close_button_->setIconSize(qTheme.GetTitleButtonIconSize());
	close_button_->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CLOSE_WINDOW, ThemeColor::DARK_THEME));

	(void)QObject::connect(close_button_, &QPushButton::clicked, [this]() {
		hide();
		});

	auto* close_button_hbox_layout = new QHBoxLayout();
	close_button_hbox_layout->setSpacing(0);
	close_button_hbox_layout->setContentsMargins(15, 15, 0, 0);

	auto* button_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Fixed);
	close_button_hbox_layout->addWidget(close_button_);
	close_button_hbox_layout->addSpacerItem(button_spacer);

	auto* default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setContentsMargins(0, 5, 0, 0);

	artist_image_ = new QLabel(this);
	artist_image_->setObjectName(QString::fromUtf8("artistImage"));
	artist_image_->setFixedHeight(300);
	artist_image_->setAlignment(Qt::AlignCenter);

	auto* album_title_layout = new QVBoxLayout();
	album_title_layout->setSpacing(0);
	album_title_layout->setObjectName(QString::fromUtf8("verticalLayout_2"));
	album_title_layout->setContentsMargins(0, 5, -1, -1);

	auto artist = new QLabel(this);
	auto f = font();
	f.setWeight(QFont::DemiBold);
	f.setPointSize(qTheme.GetFontSize(15));
	artist->setFont(f);
	artist->setText(tr("ARTIST"));

	artist_name_ = new QLabel(this);
	f.setWeight(QFont::DemiBold);
	f.setPointSize(qTheme.GetFontSize(30));
	artist_name_->setFont(f);
	artist_name_->setObjectName(QString::fromUtf8("artistNameLabel"));
	artist_name_->setMinimumSize(QSize(0, 80));
	artist_name_->setMaximumSize(QSize(16777215, 80));

	auto* top_spacer = new QSpacerItem(20, 40, QSizePolicy::Expanding, QSizePolicy::Fixed);
	auto* artist_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);

	album_title_layout->addSpacerItem(top_spacer);
	album_title_layout->addWidget(artist);
	album_title_layout->addWidget(artist_name_);
	album_title_layout->addSpacerItem(artist_spacer);
	album_title_layout->setStretch(2, 1);

	auto* hbox_layout = new QHBoxLayout();
	hbox_layout->addWidget(artist_image_);
	hbox_layout->addLayout(album_title_layout);

	auto* all_album_layout = new QVBoxLayout();

	auto all_album = new QLabel(this);
	f.setWeight(QFont::DemiBold);
	f.setPointSize(qTheme.GetFontSize(20));
	all_album->setFont(f);
	all_album->setText(tr("ALL ALBUMS"));

	album_view_ = new AlbumView(this);
	album_view_->setObjectName(QString::fromUtf8("albumView"));

	all_album_layout->addWidget(all_album);
	all_album_layout->addWidget(album_view_);
	all_album_layout->setStretch(1, 1);

	default_layout->addLayout(close_button_hbox_layout);
	default_layout->addLayout(hbox_layout);
	default_layout->addLayout(all_album_layout);
	default_layout->setStretch(2, 1);

	auto* fade_effect = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(fade_effect);
}

void ArtistViewPage::paintEvent(QPaintEvent* event) {
	QPainter painter(this);
	QLinearGradient gradient(0, 0, 0, height());
	qTheme.SetLinearGradient(gradient);
	painter.fillRect(rect(), gradient);

	painter.setCompositionMode(QPainter::CompositionMode_Overlay);

	if (!cover_.isNull()) {
		auto cover = image_utils::ResizeImage(cover_, size(), false);
		QPoint center = rect().center() - cover.rect().center();
		painter.drawPixmap(center, cover);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	}	
}

void ArtistViewPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	close_button_->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CLOSE_WINDOW, ThemeColor::DARK_THEME));
}

void ArtistViewPage::SetArtist(const QString& artist, int32_t artist_id, const QString& artist_cover_id) {
	auto artist_cover = AlbumViewStyledDelegate::GetCover(ArtistStyledItemDelegate::kArtistCacheTag, artist_cover_id);
	auto round_image = image_utils::RoundImage(artist_cover, artist_cover.width() / 2);	
	artist_name_->setText(artist);
	artist_image_->setPixmap(round_image);
	album_view_->FilterByArtistId(artist_id);
	album_view_->Update();
	cover_ = qPixmapCache.GetOrDefault(artist_cover_id, false);
	if (!cover_.isNull()) {
		cover_ = QPixmap::fromImage(image_utils::BlurImage(cover_, size()));
	}
}

ArtistView::ArtistView(QWidget* parent)
	: QListView(parent)
	, page_(new ArtistViewPage(this))
	, model_(this) {
	setModel(&model_);
	setUniformItemSizes(true);
	setDragEnabled(false);
	setSelectionRectVisible(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setWrapping(true);
	setFlow(QListView::LeftToRight);
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setFrameStyle(QFrame::StyledPanel);
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
	setAutoScroll(false);
	viewport()->setAttribute(Qt::WA_StaticContents);
	setMouseTracking(true);
	auto* styled_delegate = new ArtistStyledItemDelegate(this);
	setItemDelegate(styled_delegate);
	verticalScrollBar()->setStyleSheet(qTEXT(
		"QScrollBar:vertical { width: 6px; }"
	));
	Refresh();
	page_->hide();

	(void)QObject::connect(styled_delegate, &ArtistStyledItemDelegate::EnterAlbumView, [this](auto index) {
		const auto list_view_rect = this->rect();
		auto artist = GetIndexValue(index, INDEX_ARTIST).toString();
		auto artist_id = GetIndexValue(index, INDEX_ARTIST_ID).toInt();
		auto artist_cover_id = GetIndexValue(index, INDEX_COVER_ID).toString();
		
		if (enable_page_) {
			ShowPageAnimation();
			page_->show();
		}

		emit GetArtist(artist);
		page_->SetArtist(artist, artist_id, artist_cover_id);
		page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));
		page_->show();
		});

	auto* fade_effect = page_->graphicsEffect();
	animation_ = new QPropertyAnimation(fade_effect, "opacity");
	QObject::connect(animation_, &QPropertyAnimation::finished, [this]() {
		if (hide_page_) {
			page_->hide();
		}
		});
}

void ArtistView::OnSearchTextChanged(const QString& text) {
	last_query_ = qSTR(R"(
SELECT
    artists.artist,
    artists.artistId,
    artists.coverId,
	artists.firstChar
FROM
    artists
WHERE
    (
    artists.artist LIKE '%%1%'
    )
    )").arg(text);
}

void ArtistView::Update() {	
	if (last_query_.isEmpty()) {
		ShowAll();
	}
	model_.setQuery(last_query_, qDatabase.database());
	if (model_.lastError().type() != QSqlError::NoError) {
		XAMP_LOG_DEBUG("SqlException: {}", model_.lastError().text().toStdString());
	}
}

void ArtistView::resizeEvent(QResizeEvent* event) {
	if (page_ != nullptr) {
		if (!page_->isHidden()) {
			const auto list_view_rect = this->rect();
			page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));
		}
	}
	QListView::resizeEvent(event);
}

void ArtistView::ShowPageAnimation() {
	animation_->setStartValue(0.01);
	animation_->setEndValue(1.0);
	animation_->setDuration(kPageAnimationDurationMs);
	animation_->setEasingCurve(QEasingCurve::OutCubic);
	animation_->start();
	hide_page_ = false;
}

void ArtistView::HidePageAnimation() {
	animation_->setStartValue(1.0);
	animation_->setEndValue(0.0);
	animation_->setDuration(kPageAnimationDurationMs);
	animation_->setEasingCurve(QEasingCurve::OutCubic);
	animation_->start();
	hide_page_ = true;
}

void ArtistView::OnThemeChanged(QColor backgroundColor, QColor color) {
	dynamic_cast<ArtistStyledItemDelegate*>(itemDelegate())->SetTextColor(color);
	page_->album()->OnThemeChanged(backgroundColor, color);
}

void ArtistView::OnCurrentThemeChanged(ThemeColor theme_color) {
	page_->OnCurrentThemeChanged(theme_color);
}

void ArtistView::FilterAritstName(const QSet<QString>& name) {
	QStringList names;
	Q_FOREACH(auto & c, name) {
		names.append(qSTR("'%1'").arg(c));
	}

	last_query_ = qSTR(R"(
SELECT
    artists.artist,
    artists.artistId,
    artists.coverId,
	artists.firstChar,
	SUBSTR(artistNameEn, 1, 1) AS enNameFirstChar
FROM
    artists
WHERE
	enNameFirstChar IN (%1)
    )").arg(names.join(","));
}

void ArtistView::ShowAll() {
	last_query_ = qTEXT(R"(
SELECT
    artists.artist,
    artists.artistId,
    artists.coverId,
	artists.firstChar
FROM
    artists
    )");
}

void ArtistView::Refresh() {
	Update();
}
