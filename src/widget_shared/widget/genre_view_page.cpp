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
	f.setPointSize(qTheme.GetFontSize(15));
	f.setBold(true);
	genre_label_->setFont(f);

	auto* cevron_right = new QToolButton();
	cevron_right->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CHEVRON_LEFT));

	auto* genre_combox_layout = new QHBoxLayout();
	auto horizontalSpacer_3 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	genre_combox_layout->addWidget(genre_label_);
	genre_combox_layout->addWidget(cevron_right);
	genre_combox_layout->addSpacerItem(horizontalSpacer_3);

	genre_view_ = new GenreView(this);

	genre_view_->ShowAll();
	genre_view_->Refresh();

	genre_container_layout->addLayout(genre_combox_layout);
	genre_container_layout->addWidget(genre_view_, 1);

	(void)QObject::connect(genre_label_, &ClickableLabel::clicked, [this]() {
		emit goBackPage();
		});

	(void)QObject::connect(cevron_right, &QToolButton::clicked, [this]() {
		emit goBackPage();
		});
}

void GenrePage::SetGenre(const QString& genre) {
	genre_view_->SetGenre(genre);
	auto genre_label_text = genre;
	genre_label_text = genre_label_text.replace(0, 1, genre_label_text.at(0).toUpper());
	genre_label_->setText(genre_label_text);
	genre_view_->ShowAllAlbum();
	genre_view_->SetShowMode(SHOW_NORMAL);
	genre_view_->Refresh();
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

void GenreViewPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	Q_FOREACH(auto page, genre_view_) {
		page.first->view()->OnCurrentThemeChanged(theme_color);
		page.second->OnCurrentThemeChanged(theme_color);
	}
}

void GenreViewPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
	Q_FOREACH(auto page, genre_view_) {
		page.first->view()->OnThemeChanged(backgroundColor, color);
		page.second->OnThemeChanged(backgroundColor, color);
	}
}

void GenreViewPage::Refresh() {
	Q_FOREACH(auto page, genre_view_) {
		page.first->view()->Refresh();
		page.second->Refresh();
	}
}

void GenreViewPage::Clear() {
	
}

void GenreViewPage::RemoveGenre(const QString& genre) {
	genre_view_.remove(genre);
}

void GenreViewPage::AddGenre(const QString& genre) {
	if (genre_view_.contains(genre)) {
		return;
	}

	auto f = font();

	auto genre_label_text = genre;
	genre_label_text = genre_label_text.replace(0, 1, genre_label_text.at(0).toUpper());
	auto* genre_label = new ClickableLabel(genre_label_text);
	f.setPointSize(qTheme.GetFontSize(15));
	f.setBold(true);
	genre_label->setFont(f);

	auto* cevron_right = new QToolButton();
	cevron_right->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CHEVRON_RIGHT));

	auto* genre_combox_layout = new QHBoxLayout();
	auto horizontalSpacer_3 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	genre_combox_layout->addWidget(genre_label);
	genre_combox_layout->addWidget(cevron_right);
	genre_combox_layout->addSpacerItem(horizontalSpacer_3);

	auto* genre_view = new GenreView(this);
	genre_view->SetGenre(genre);
	genre_view->ShowAll();
	genre_view->Refresh();
	genre_view->verticalScrollBar()->hide();
	genre_view->setFixedHeight(275);
	genre_view->SetShowMode(SHOW_NORMAL);

	(void)QObject::connect(genre_label, &ClickableLabel::clicked, [genre, this]() {
		genre_page_->SetGenre(genre);
		setCurrentIndex(1);
		});

	(void)QObject::connect(cevron_right, &QToolButton::clicked, [genre, this]() {
		genre_page_->SetGenre(genre);
		setCurrentIndex(1);
		});

	genre_view_[genre] = qMakePair(genre_page_, genre_view);

	genre_frame_layout_->addLayout(genre_combox_layout);
	genre_frame_layout_->addWidget(genre_view, 0);
}
