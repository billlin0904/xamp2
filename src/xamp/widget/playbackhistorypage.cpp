#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>
#include <QApplication>
#include <QMouseEvent>

#include "thememanager.h"

#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/playbackhistorypage.h>

CheckBoxDelegate::CheckBoxDelegate(QObject* parent)
	: QItemDelegate(parent) {
}

bool CheckBoxDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
	Qt::ItemFlags flags = model->flags(index);
	if (!(flags & Qt::ItemIsUserCheckable) || !(flags & Qt::ItemIsEnabled)) {
		return false;
	}

	QVariant value = index.data(Qt::CheckStateRole);
	if (!value.isValid()) {
		return false;
	}

	if (event->type() == QEvent::MouseButtonRelease) {
		const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
		QRect checkRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
			option.decorationSize,
			QRect(option.rect.x() + (2 * textMargin), option.rect.y(),
				option.rect.width() - (2 * textMargin),
				option.rect.height()));
		QMouseEvent* mEvent = (QMouseEvent*)event;
		if (!checkRect.contains(mEvent->pos())) {
			return false;
		}
	}
	else if (event->type() == QEvent::KeyPress) {
		if (static_cast<QKeyEvent*>(event)->key() !=
			Qt::Key_Space && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select) {
			return false;
		}
	}
	else {
		return false;
	}

	Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked
		? Qt::Unchecked : Qt::Checked);

	return model->setData(index, state, Qt::CheckStateRole);
}

void CheckBoxDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	QStyleOptionViewItem viewItemOption(option);

	if (index.column() == PlaybackHistoryModel::CHECKBOX_ROW) {
		const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
		QRect newRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
			QSize(option.decorationSize.width() +
				5, option.decorationSize.height()),
			QRect(option.rect.x() + textMargin, option.rect.y(),
				option.rect.width() -
				(2 * textMargin), option.rect.height()));
		viewItemOption.rect = newRect;
	}

	QItemDelegate::paint(painter, viewItemOption, index);
}

PlaybackHistoryModel::PlaybackHistoryModel(QObject* parent)
	: QSqlQueryModel(parent) {
}

Qt::ItemFlags PlaybackHistoryModel::flags(const QModelIndex& index) const {
	auto col = index.column();
	if (col == CHECKBOX_ROW) {
		return QSqlQueryModel::flags(index) | Qt::ItemIsUserCheckable;
	}
	return QSqlQueryModel::flags(index);
}

bool PlaybackHistoryModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (index.column() == CHECKBOX_ROW && role == Qt::CheckStateRole) {
		QSqlQuery query;
		auto id = GetIndexValue(index, 12).toInt();
		auto check = value == Qt::Checked ? 1 : 0;
		query.prepare(Q_UTF8("UPDATE playbackHistory SET selected = ? WHERE playbackHistoryId = ?"));
		query.addBindValue(check);
		query.addBindValue(id);
		query.exec();
		this->query().exec();
		return QAbstractItemModel::setData(index, check);
	}
	return QSqlQueryModel::setData(index, value, role);
}

QVariant PlaybackHistoryModel::data(const QModelIndex& index, int role) const {
	auto col = index.column();
	if (col == CHECKBOX_ROW) {
		if (role == Qt::CheckStateRole) {
			int checked = QSqlQueryModel::data(index).toInt();
			if (checked) {
				return Qt::Checked;
			} else {
				return Qt::Unchecked;
			}
		} else {
			return QVariant();
		}
	}
#if 0
	else if (role == Qt::ToolTipRole)
	{
		const QString original = QSqlQueryModel::data(index, Qt::DisplayRole).toString();
		QString toolTip = breakString(original);
		return toolTip;
	}
	else if (role == Qt::DisplayRole)
	{
		const QString original = QSqlQueryModel::data(index, Qt::DisplayRole).toString();
		QString shownText = fixBreakLines(original);
		return shownText;
	}
#endif
	else {
		return QSqlQueryModel::data(index, role);
	}
	return QVariant();
}

PlaybackHistoryTableView::PlaybackHistoryTableView(QWidget* parent)
	: QTableView(parent) {
	setItemDelegateForColumn(PlaybackHistoryModel::CHECKBOX_ROW, new CheckBoxDelegate(this));
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
	albums.album,
	artists.artistId,
	albums.albumId,
	playbackHistory.selected,
	playbackHistory.playbackHistoryId	
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
	setColumnHidden(9, true);
	setColumnHidden(10, true);
	setColumnHidden(11, false);
	setColumnHidden(12, true);
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

	(void)QObject::connect(playlist_, &QTableView::doubleClicked, [this](const QModelIndex& index) {
		auto title = GetIndexValue(index, 1).toString();
		auto musicId = GetIndexValue(index, 3).toInt();
		auto artist = GetIndexValue(index, 4).toString();
		auto file_ext = GetIndexValue(index, 5).toString();
		auto file_path = GetIndexValue(index, 6).toString();
		auto cover_id = GetIndexValue(index, 7).toString();
		auto album = GetIndexValue(index, 8).toString();
		auto artistId = GetIndexValue(index, 9).toInt();
		auto albumId = GetIndexValue(index, 10).toInt();

		AlbumEntity entity;

		entity.music_id = musicId;
		entity.album = album;
		entity.title = title;
		entity.artist = artist;
		entity.cover_id = cover_id;
		entity.file_path = file_path;
		entity.file_ext = file_ext;
		entity.album_id = albumId;
		entity.artist_id = artistId;

		emit playMusic(entity);
		});
}

void PlaybackHistoryPage::refreshOnece() {
	playlist_->refreshOnece();
}