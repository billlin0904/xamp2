#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>

#include "thememanager.h"

#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/playbackhistorypage.h>

PlaybackHistoryTableView::PlaybackHistoryTableView(QWidget* parent)
	: QTableView(parent) {
	setModel(&model_);
	auto f = font();
#ifdef Q_OS_WIN
	f.setPointSize(9);
#else
	f.setPointSize(12);
#endif
	setFont(f);

	setUpdatesEnabled(true);
	setAcceptDrops(true);
	setDragEnabled(true);
	setShowGrid(false);

	setDragDropMode(InternalMove);
	setFrameShape(NoFrame);
	setFocusPolicy(Qt::NoFocus);

	setHorizontalScrollMode(ScrollPerPixel);
	setVerticalScrollMode(ScrollPerPixel);
	setSelectionMode(ExtendedSelection);
	setSelectionBehavior(SelectRows);

	verticalHeader()->setVisible(false);

	setColumnWidth(0, 5);
	horizontalHeader()->hide();
	horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	verticalScrollBar()->setStyleSheet(Q_UTF8(R"(
    QScrollBar:vertical {
        background: #FFFFFF;
		width: 9px;
    }
	QScrollBar::handle:vertical {
		background: #dbdbdb;
		border-radius: 3px;
		min-height: 20px;
		border: none;
	}
	QScrollBar::handle:vertical:hover {
		background: #d0d0d0;
	}
	)"));

	setStyleSheet(Q_UTF8("background-color: transparent"));
	refreshOnece();
}

void PlaybackHistoryTableView::resizeColumn() {
	auto header = horizontalHeader();

	for (auto column = 0; column < header->count(); ++column) {
		switch (column) {
		case PlaybackHistoryTableView::PLAYLIST_TRACK:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
			header->resizeSection(column, 12);
			break;
		case PlaybackHistoryTableView::PLAYLIST_TITLE:
			header->setSectionResizeMode(column, QHeaderView::Fixed);
			header->resizeSection(column, size().width() - ThemeManager::getAlbumCoverSize().width() - 100);
			break;
		case PlaybackHistoryTableView::PLAYLIST_DURATION:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
			header->resizeSection(column, 60);
			break;
		default:
			header->setSectionResizeMode(column, QHeaderView::Stretch);
			break;
		}
	}
}

void PlaybackHistoryTableView::refreshOnece() {
	QString query = Q_UTF8(R"(
SELECT
	musics.track,
	musics.title,
	musics.durationStr,
	musics.musicId,
	artists.artist,
	musics.fileExt,
	musics.path,
	albums.coverId,
	albums.album
FROM
	playbackHistory
	LEFT JOIN albums ON albums.albumId = playbackHistory.albumId
	LEFT JOIN artists ON artists.artistId = playbackHistory.artistId
	LEFT JOIN musics ON musics.musicId = playbackHistory.musicId;
)");
	model_.setQuery(query);
	setColumnHidden(3, true);
	setColumnHidden(4, true);
	setColumnHidden(5, true);
	setColumnHidden(6, true);
	setColumnHidden(7, true);
	setColumnHidden(8, true);
}

PlaybackHistoryPage::PlaybackHistoryPage(QWidget* parent)
	: QFrame(parent) {
	setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 255)"));

	auto default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setContentsMargins(10, 10, 10, 0);

	auto close_button = new QPushButton(tr("X"), this);
	close_button->setFixedSize(QSize(24, 24));
	close_button->setStyleSheet(Q_UTF8("border: none"));

	auto button_layout = new QHBoxLayout();

	auto select_all_button = new QPushButton(tr("Select All"), this);
	select_all_button->setStyleSheet(Q_UTF8("border: none"));
	button_layout->addWidget(select_all_button);

	auto clear_button = new QPushButton(tr("Clear"), this);
	clear_button->setStyleSheet(Q_UTF8("border: none"));
	button_layout->addWidget(clear_button);

	auto save_playlist_button = new QPushButton(tr("Save"), this);
	save_playlist_button->setStyleSheet(Q_UTF8("border: none"));
	button_layout->addWidget(save_playlist_button);

	playlist_ = new PlaybackHistoryTableView();
	default_layout->addWidget(close_button);
	default_layout->addLayout(button_layout);
	default_layout->addWidget(playlist_);

	(void)QObject::connect(close_button, &QPushButton::clicked, [this]() {
		hide();
		});

	(void)QObject::connect(clear_button, &QPushButton::clicked, [this]() {
		Database::Instance().deleteOldestHistory();
		playlist_->refreshOnece();
		});
}

void PlaybackHistoryPage::refreshOnece() {
	playlist_->refreshOnece();
}