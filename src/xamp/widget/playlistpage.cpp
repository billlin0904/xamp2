#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QLabel>

#include "playlisttableview.h"
#include "playlistpage.h"

PlyalistPage::PlyalistPage(QWidget* parent)
	: QFrame(parent) {
	initial();
}

void PlyalistPage::initial() {
	setStyleSheet("background: transparent;");

	auto verticalLayout_4 = new QVBoxLayout(this);
	verticalLayout_4->setSpacing(0);
	verticalLayout_4->setContentsMargins(11, 11, 11, 11);
	verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
	verticalLayout_4->setContentsMargins(0, 20, 0, 0);
	auto horizontalLayout_7 = new QHBoxLayout();
	horizontalLayout_7->setSpacing(0);
	horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	horizontalLayout_7->setContentsMargins(20, -1, 20, -1);
	auto verticalLayout_3 = new QVBoxLayout();
	verticalLayout_3->setSpacing(0);
	verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
	auto verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	verticalLayout_3->addItem(verticalSpacer_2);

	cover_ = new QLabel(this);
	cover_->setObjectName(QString::fromUtf8("label"));
	cover_->setMinimumSize(QSize(150, 150));
	cover_->setMaximumSize(QSize(150, 150));

	verticalLayout_3->addWidget(cover_);


	horizontalLayout_7->addLayout(verticalLayout_3);

	auto horizontalSpacer_15 = new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

	horizontalLayout_7->addItem(horizontalSpacer_15);

	auto verticalLayout_2 = new QVBoxLayout();
	verticalLayout_2->setSpacing(0);
	verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
	verticalLayout_2->setContentsMargins(-1, 5, -1, -1);
	title_ = new QLabel(this);
	auto f = font();
	f.setPixelSize(25);
	title_->setFont(f);
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setMinimumSize(QSize(0, 30));
	title_->setMaximumSize(QSize(16777215, 30));

	verticalLayout_2->addWidget(title_);

	auto verticalSpacer = new QSpacerItem(20, 108, QSizePolicy::Minimum, QSizePolicy::Expanding);

	verticalLayout_2->addItem(verticalSpacer);


	horizontalLayout_7->addLayout(verticalLayout_2);

	horizontalLayout_7->setStretch(2, 1);

	verticalLayout_4->addLayout(horizontalLayout_7);

	auto verticalSpacer_3 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	verticalLayout_4->addItem(verticalSpacer_3);

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

	verticalLayout_4->addLayout(horizontalLayout_8);

	verticalLayout_4->setStretch(2, 1);
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
