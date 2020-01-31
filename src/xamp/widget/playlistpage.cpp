#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QLabel>

#include "str_utilts.h"
#include "playlisttableview.h"
#include "playlistpage.h"

PlyalistPage::PlyalistPage(QWidget* parent)
	: QFrame(parent) {
	initial();
}

void PlyalistPage::initial() {
	setStyleSheet(Q_UTF8("background: transparent;"));

	auto default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setContentsMargins(11, 11, 11, 11);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 20, 0, 0);

	auto child_layout = new QHBoxLayout();
	child_layout->setSpacing(0);
	child_layout->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	child_layout->setContentsMargins(20, -1, 20, -1);
	auto left_space_layout = new QVBoxLayout();
	left_space_layout->setSpacing(0);
	left_space_layout->setObjectName(QString::fromUtf8("verticalLayout_3"));
	auto vertical_spacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	left_space_layout->addItem(vertical_spacer);

	cover_ = new QLabel(this);
	cover_->setObjectName(QString::fromUtf8("label"));
	cover_->setMinimumSize(QSize(150, 150));
	cover_->setMaximumSize(QSize(200, 150));

	left_space_layout->addWidget(cover_);


	child_layout->addLayout(left_space_layout);

	auto horizontal_spacer = new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

	child_layout->addItem(horizontal_spacer);

	auto album_title_layout = new QVBoxLayout();
	album_title_layout->setSpacing(0);
	album_title_layout->setObjectName(QString::fromUtf8("verticalLayout_2"));
	album_title_layout->setContentsMargins(-1, 5, -1, -1);
	title_ = new QLabel(this);
	auto f = font();
	f.setBold(true);
	f.setPixelSize(25);
	title_->setFont(f);
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setMinimumSize(QSize(0, 30));
	title_->setMaximumSize(QSize(16777215, 30));

	format_ = new QLabel(this);
	f = font();
	f.setBold(false);
	format_->setFont(f);

	album_title_layout->addWidget(title_);

	auto middle_spacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
	album_title_layout->addItem(middle_spacer);

	album_title_layout->addWidget(format_);

	auto right_spacer = new QSpacerItem(20, 108, QSizePolicy::Minimum, QSizePolicy::Expanding);
	album_title_layout->addItem(right_spacer);


	child_layout->addLayout(album_title_layout);

	child_layout->setStretch(2, 1);

	default_layout->addLayout(child_layout);

	auto default_spacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	default_layout->addItem(default_spacer);

	auto horizontalLayout_8 = new QHBoxLayout();
	horizontalLayout_8->setSpacing(0);
	horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
	auto horizontalSpacer_4 = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	horizontalLayout_8->addItem(horizontalSpacer_4);

	playlist_ = new PlayListTableView(this);
	playlist_->setObjectName(QString::fromUtf8("tableView"));

	horizontalLayout_8->addWidget(playlist_);

	auto horizontalSpacer_5 = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	horizontalLayout_8->addItem(horizontalSpacer_5);

	horizontalLayout_8->setStretch(1, 1);

	default_layout->addLayout(horizontalLayout_8);

	default_layout->setStretch(2, 1);
}

QLabel* PlyalistPage::format() {
	return format_;
}

QLabel* PlyalistPage::title() {
	return title_;
}

QLabel* PlyalistPage::cover() {
	return cover_;
}

PlayListTableView* PlyalistPage::playlist() {
	return playlist_;
}
