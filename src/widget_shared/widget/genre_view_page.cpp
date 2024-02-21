#include <widget/genre_view_page.h>

#include <widget/genre_view.h>
#include <widget/albumartistpage.h>
#include <widget/clickablelabel.h>

#include <QScrollBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QScrollArea>

#include <thememanager.h>

GenrePage::GenrePage(QWidget* parent)
	: QFrame(parent) {
	auto genre_container_layout = new QVBoxLayout(this);

	auto f = font();
	genre_label_ = new ClickableLabel();
	f.setPointSize(qTheme.fontSize(15));
	f.setBold(true);
	genre_label_->setFont(f);

	auto* cevron_right = new QToolButton();
	cevron_right->setIcon(qTheme.fontIcon(Glyphs::ICON_CHEVRON_LEFT));

	auto* genre_combox_layout = new QHBoxLayout();
	auto horizontalSpacer_3 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	genre_combox_layout->addWidget(genre_label_);
	genre_combox_layout->addWidget(cevron_right);
	genre_combox_layout->addSpacerItem(horizontalSpacer_3);

	genre_view_ = new GenreView(this);

	genre_view_->showAll();
	genre_view_->reload();

	genre_container_layout->addLayout(genre_combox_layout);
	genre_container_layout->addWidget(genre_view_, 1);

	(void)QObject::connect(genre_label_, &ClickableLabel::clicked, [this]() {
		emit goBackPage();
		});

	(void)QObject::connect(cevron_right, &QToolButton::clicked, [this]() {
		emit goBackPage();
		});
}

void GenrePage::setGenre(const QString& genre) {
	genre_view_->setGenre(genre);
	auto genre_label_text = genre;
	genre_label_text = genre_label_text.replace(0, 1, genre_label_text.at(0).toUpper());
	genre_label_->setText(genre_label_text);
	genre_view_->showAll();
	genre_view_->setShowMode(SHOW_NORMAL);
	genre_view_->reload();
}

GenreViewPage::GenreViewPage(QWidget* parent)
	: QStackedWidget(parent) {
	auto* container_frame = new QFrame();
	auto* genre_container_layout = new QVBoxLayout(container_frame);

	genre_page_ = new GenrePage();
	(void)QObject::connect(genre_page_, &GenrePage::goBackPage, [this]() {
		setCurrentIndex(0);
		});

	addWidget(container_frame);
	addWidget(genre_page_);

	auto* scroll_area = new QScrollArea();
	scroll_area->setWidgetResizable(true);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	genre_frame_ = new QFrame();
	genre_frame_->setObjectName(QString::fromUtf8("currentGenreViewFrame"));
	genre_frame_->setFrameShape(QFrame::StyledPanel);

	genre_frame_layout_ = new QVBoxLayout(genre_frame_);
	genre_frame_layout_->setSpacing(0);
	genre_frame_layout_->setObjectName(QString::fromUtf8("currentGenreViewFrameLayout"));
	genre_frame_layout_->setContentsMargins(0, 0, 0, 0);

	scroll_area->setWidget(genre_frame_);

	auto* vertical_spacer = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	genre_frame_layout_->addItem(vertical_spacer);

	genre_container_layout->addWidget(scroll_area);
}

void GenreViewPage::onThemeChangedFinished(ThemeColor theme_color) {
	Q_FOREACH(auto page, genre_view_) {
		page.first->view()->onThemeChangedFinished(theme_color);
		page.second->onThemeChangedFinished(theme_color);
	}
}

void GenreViewPage::onThemeColorChanged(QColor backgroundColor, QColor color) {
	Q_FOREACH(auto page, genre_view_) {
		page.first->view()->onThemeColorChanged(backgroundColor, color);
		page.second->onThemeColorChanged(backgroundColor, color);
	}
}

void GenreViewPage::reload() {
	Q_FOREACH(auto page, genre_view_) {
		page.first->view()->reload();
		page.second->reload();
	}
}

void GenreViewPage::clear() {
	
}

void GenreViewPage::removeGenre(const QString& genre) {
	genre_view_.remove(genre);
}

void GenreViewPage::addGenre(const QString& genre) {
	if (genre_view_.contains(genre)) {
		return;
	}

	auto f = font();

	auto genre_label_text = genre;
	genre_label_text = genre_label_text.replace(0, 1, genre_label_text.at(0).toUpper());
	auto* genre_label = new ClickableLabel(genre_label_text);
	f.setPointSize(qTheme.fontSize(15));
	f.setBold(true);
	genre_label->setFont(f);

	auto* cevron_right = new QToolButton();
	cevron_right->setIcon(qTheme.fontIcon(Glyphs::ICON_CHEVRON_RIGHT));

	auto* genre_combox_layout = new QHBoxLayout();
	auto horizontalSpacer_3 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	genre_combox_layout->addWidget(genre_label);
	genre_combox_layout->addWidget(cevron_right);
	genre_combox_layout->addSpacerItem(horizontalSpacer_3);

	auto* genre_view = new GenreView(this);
	genre_view->setGenre(genre);
	genre_view->showAll();
	genre_view->reload();
	genre_view->verticalScrollBar()->hide();
	genre_view->setFixedHeight(275);
	genre_view->setShowMode(SHOW_NORMAL);

	(void)QObject::connect(genre_label, &ClickableLabel::clicked, [genre, this]() {
		genre_page_->setGenre(genre);
		setCurrentIndex(1);
		});

	(void)QObject::connect(cevron_right, &QToolButton::clicked, [genre, this]() {
		genre_page_->setGenre(genre);
		setCurrentIndex(1);
		});

	genre_view_[genre] = qMakePair(genre_page_, genre_view);

	genre_frame_layout_->addLayout(genre_combox_layout);
	genre_frame_layout_->addWidget(genre_view, 0);
}
