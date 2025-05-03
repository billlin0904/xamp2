#include <widget/artistview.h>
#include <widget/albumview.h>
#include <widget/database.h>
#include <widget/playlistentity.h>
#include <widget/taglistview.h>

#include <thememanager.h>

#include <QSqlError>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QLabel>
#include <QPropertyAnimation>

enum {
	ARTIST_INDEX_ARTIST,
	ARTIST_INDEX_ARTIST_ID,
	ARTIST_INDEX_COVER_ID,
	ARTIST_INDEX_FIRST_CHAR,
};

ArtistStyledItemDelegate::ArtistStyledItemDelegate(QObject* parent)
	: QStyledItemDelegate(parent) {
}

void ArtistStyledItemDelegate::setTextColor(QColor color) {
	text_color_ = color;
}

void ArtistStyledItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto* style = option.widget ? option.widget->style() : QApplication::style();

	const auto artist = index.model()->data(index.model()->index(index.row(),          ARTIST_INDEX_ARTIST)).toString();
	auto artist_id = index.model()->data(index.model()->index(index.row(),             ARTIST_INDEX_ARTIST_ID)).toString();
	const auto artist_cover_id = index.model()->data(index.model()->index(index.row(), ARTIST_INDEX_COVER_ID)).toString();
	const auto first_char = index.model()->data(index.model()->index(index.row(),      ARTIST_INDEX_FIRST_CHAR)).toString();

	constexpr auto kPaddingSize = 2;

	const auto default_cover_size = qTheme.defaultCoverSize();
	const QRect cover_rect(option.rect.left() + kPaddingSize,
		option.rect.top() + kPaddingSize,
		default_cover_size.width(),
		default_cover_size.height());

	painter->setRenderHints(QPainter::Antialiasing, true);
	painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter->setRenderHints(QPainter::TextAntialiasing, true);

	const auto size = cover_rect.height() - 4;
	const QRect rect(cover_rect.x() + 2, cover_rect.y() + 2, size, size);
	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::gray);
	painter->drawEllipse(rect);

	auto font = painter->font();

	if (!artist_cover_id.isEmpty()) {
		const auto artist_cover = qImageCache.getOrDefault(ArtistStyledItemDelegate::kArtistCacheTag, artist_cover_id);
		painter->drawPixmap(rect, image_util::roundImage(artist_cover, size / 2));
	}
	else {
		painter->setPen(Qt::white);
		font.setPointSize(qTheme.fontSize(70));
		font.setBold(true);
		painter->setFont(font);
		painter->drawText(rect, Qt::AlignCenter, artist.left(1));
	}

	painter->setPen(text_color_);
	font.setBold(false);
	font.setPointSize(qTheme.fontSize(9));
	painter->setFont(font);
	const auto album_artist_text_width = default_cover_size.width();
	const QRect artist_text_rect(option.rect.left() + 10,
	                             option.rect.top() + default_cover_size.height() + 15,
	                             album_artist_text_width,
	                             20);
	const QFontMetrics album_metrics(font);
	painter->drawText(artist_text_rect, Qt::AlignVCenter,
		album_metrics.elidedText(artist, Qt::ElideRight, album_artist_text_width));
}

QSize ArtistStyledItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto result = QStyledItemDelegate::sizeHint(option, index);
	const auto default_cover = qTheme.defaultCoverSize();
	result.setWidth(default_cover.width() + 30);
	result.setHeight(default_cover.height() + 80);
	return result;
}

bool ArtistStyledItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
	if (event->type() == QEvent::MouseButtonPress) {
		const auto* mouse_event = dynamic_cast<QMouseEvent*>(event);
		if (mouse_event->button() == Qt::LeftButton) {
			emit enterAlbumView(index);
		}
	}
	return true;
}

const ConstexprQString ArtistStyledItemDelegate::kArtistCacheTag("artist_thumbnail_"_str);

ArtistViewPage::ArtistViewPage(QWidget* parent)
	: QFrame(parent) {
	setObjectName(QString::fromUtf8("artistViewPage"));
	setFrameStyle(QFrame::StyledPanel);

	close_button_ = new QPushButton(this);
	close_button_->setObjectName("albumViewPageCloseButton"_str);
	close_button_->setCursor(Qt::PointingHandCursor);
	close_button_->setAttribute(Qt::WA_TranslucentBackground);
	close_button_->setFixedSize(qTheme.titleButtonIconSize() * 1.5);
	close_button_->setIconSize(qTheme.titleButtonIconSize() * 1.5);
	close_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW, qTheme.themeColor()));

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
	f.setPointSize(qTheme.fontSize(15));
	artist->setFont(f);
	artist->setText(tr("ARTIST"));

	artist_name_ = new QLabel(this);
	f.setWeight(QFont::DemiBold);
	f.setPointSize(qTheme.fontSize(30));
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
	f.setPointSize(qTheme.fontSize(20));
	all_album->setFont(f);
	all_album->setText(tr("ALL ALBUMS"));

	album_view_ = new AlbumView(this);
	album_view_->setObjectName(QString::fromUtf8("albumView"));
	album_view_->styledDelegate()->enableAlbumView(false);

	all_album_layout->addWidget(all_album);
	all_album_layout->addWidget(album_view_);
	all_album_layout->setStretch(1, 1);

	default_layout->addLayout(close_button_hbox_layout);
	default_layout->addLayout(hbox_layout);
	default_layout->addLayout(all_album_layout);
	default_layout->setStretch(2, 1);

	//auto* fade_effect = new QGraphicsOpacityEffect(this);
	//setGraphicsEffect(fade_effect);

	artist_image_->hide();
	artist_name_->hide();
	artist->hide();
}

void ArtistViewPage::paintEvent(QPaintEvent* event) {
	QPainter painter(this);
}

void ArtistViewPage::onThemeChangedFinished(ThemeColor theme_color) {
	close_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_CLOSE_WINDOW, ThemeColor::DARK_THEME));
	album_view_->onThemeChangedFinished(theme_color);
}

void ArtistViewPage::setArtist(const QString& artist, int32_t artist_id, const QString& artist_cover_id) {
	setStyleSheet(qFormat(
		R"(
           QFrame#artistViewPage {
		        background-color: %1;
                border-radius: 8px;
           }
        )"
	).arg(qTheme.linearGradientStyle()));

	const auto artist_cover = qImageCache.getOrDefault(ArtistStyledItemDelegate::kArtistCacheTag, artist_cover_id);
	const auto round_image = image_util::roundImage(artist_cover, artist_cover.width() / 2);	
	artist_name_->setText(artist);
	artist_image_->setPixmap(round_image);
	album_view_->filterByArtistId(artist_id);
	album_view_->reload();
	cover_ = qImageCache.getOrAddDefault(artist_cover_id, false);
	/*if (!cover_.isNull()) {
		cover_ = QPixmap::fromImage(image_util::blurImage(cover_, size()));
	}*/
}

ArtistView::ArtistView(QWidget* parent)
	: QListView(parent)
	, page_(new ArtistViewPage(this))
	, model_(this)
	, proxy_model_(new PlayListTableFilterProxyModel(this)) {
	proxy_model_->addFilterByColumn(ALBUM_INDEX_ARTIST);
	proxy_model_->setSourceModel(&model_);
	setModel(proxy_model_);

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
    styled_delegate_ = new ArtistStyledItemDelegate(this);
    setItemDelegate(styled_delegate_);
	verticalScrollBar()->setStyleSheet(
		"QScrollBar:vertical { width: 6px; }"_str
	);
	reload();
	page_->hide();

    (void)QObject::connect(styled_delegate_, &ArtistStyledItemDelegate::enterAlbumView, [this](auto index) {
		const auto list_view_rect = this->rect();
		auto artist = indexValue(index,          ARTIST_INDEX_ARTIST).toString();
		auto artist_id = indexValue(index,       ARTIST_INDEX_ARTIST_ID).toInt();
		auto artist_cover_id = indexValue(index, ARTIST_INDEX_COVER_ID).toString();
		
		if (enable_page_) {
			showPageAnimation();
			page_->show();
		}

		emit getArtist(artist);
		page_->setArtist(artist, artist_id, artist_cover_id);
		page_->setFixedSize(QSize(list_view_rect.size().width() - 2, list_view_rect.height()));
		page_->show();
		});
}

void ArtistView::search(const QString& keyword) {
	const QRegularExpression reg_exp(keyword, QRegularExpression::CaseInsensitiveOption);
	proxy_model_->addFilterByColumn(ALBUM_INDEX_ARTIST);
	proxy_model_->setFilterRegularExpression(reg_exp);
}

void ArtistView::reload() {	
	if (last_query_.isEmpty()) {
		showAll();
	}
	model_.setQuery(last_query_, qGuiDb.database());
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

void ArtistView::showPageAnimation() {
	hide_page_ = false;
}

void ArtistView::hidePageAnimation() {
	hide_page_ = true;
}

void ArtistView::onThemeChangedFinished(ThemeColor theme_color) {
	page_->onThemeChangedFinished(theme_color);
    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        styled_delegate_->setTextColor(Qt::white);
        break;
    }
}

void ArtistView::filterArtistName(const QSet<QString>& name) {
	QStringList names;
	Q_FOREACH(auto & c, name) {
		names.append(qFormat("'%1'").arg(c));
	}

	last_query_ = qFormat(R"(
SELECT
    artists.artist,
    artists.artistId,
    artists.coverId,
	artists.firstChar,
	artists.firstCharEn
FROM
    artists
WHERE
	firstChar IN (%1)
    )").arg(names.join(","_str));
	page_->hide();
}

void ArtistView::showAll() {
	last_query_ = R"(
SELECT
    artists.artist,
    artists.artistId,
    artists.coverId,
	artists.firstChar
FROM
    artists
    )"_str;
	page_->hide();
}
