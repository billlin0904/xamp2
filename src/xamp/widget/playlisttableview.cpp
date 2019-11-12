#include <QHeaderView>
#include <QMenu>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QtWidgets/QApplication>
#include <QClipboard>
#include <QScrollBar>

#include <base/rng.h>
#include <metadata/taglibmetareader.h>

#include "str_utilts.h"
#include "actionmap.h"
#include "playlisttableview.h"

PlayListEntity PlayListTableView::fromMetadata(const xamp::base::Metadata& metadata) {
	PlayListEntity item;
	item.track = metadata.track;
	item.duration = metadata.duration;
	item.samplerate = metadata.samplerate;
	item.title = QString::fromStdWString(metadata.title);
	item.file_ext = QString::fromStdWString(metadata.file_ext);
	item.album = QString::fromStdWString(metadata.album);
	item.artist = QString::fromStdWString(metadata.artist);
	item.file_path = QString::fromStdWString(metadata.file_path);
	item.bitrate = metadata.bitrate;
	item.cover_id = QLatin1String(metadata.cover_id.c_str(), metadata.cover_id.length());
	item.file_name = QString::fromStdWString(metadata.file_name);
	return item;
}

PlayListTableView::PlayListTableView(QWidget* parent, int32_t playlist_id)
	: QTableView(parent)
	, playlist_id_(playlist_id)
	, model_(this)
	, proxy_model_(this) {
	initial();
}

PlayListTableView::~PlayListTableView() {
	adapter_.Cancel();
}

void PlayListTableView::initial() {
	proxy_model_.setSourceModel(&model_);
	proxy_model_.setFilterByColumn(PLAYLIST_ALBUM);
	proxy_model_.setFilterByColumn(PLAYLIST_ARTIST);
	proxy_model_.setFilterByColumn(PLAYLIST_TITLE);
	proxy_model_.setDynamicSortFilter(true);
	setModel(&proxy_model_);

	setUpdatesEnabled(true);
	setAcceptDrops(true);
	setDragEnabled(true);
	setShowGrid(false);

	setDragDropMode(InternalMove);
	setFrameShape(NoFrame);
	setFocusPolicy(Qt::NoFocus);

	hideColumn(PLAYLIST_MUSIC_ID);
	hideColumn(PLAYLIST_FILEPATH);
	hideColumn(PLAYLIST_FILE_NAME);
	hideColumn(PLAYLIST_BITRATE);
	hideColumn(PLAYLIST_SAMPLE_RATE);

	setHorizontalScrollMode(ScrollPerPixel);
	setVerticalScrollMode(ScrollPerPixel);
	setSelectionMode(ExtendedSelection);
	setSelectionBehavior(SelectRows);

	verticalHeader()->setVisible(false);
	verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	verticalHeader()->setDefaultSectionSize(40);

	horizontalScrollBar()->setDisabled(true);

	horizontalHeader()->setStyleSheet(Q_UTF8(R"(
		QHeaderView::section {
			background-color: transparent;
		}
		QHeaderView {
			color: gray;
			background-color: transparent;
		}
	 )"));

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

	horizontalHeader()->setHighlightSections(false);
	horizontalHeader()->setStretchLastSection(true);
	horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	setEditTriggers(DoubleClicked | SelectedClicked);
	verticalHeader()->setSectionsMovable(false);
	horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

	(void)QObject::connect(horizontalHeader(), &QHeaderView::sectionClicked, [this](int logicalIndex) {
		sortByColumn(logicalIndex, Qt::AscendingOrder);
		});

	(void)QObject::connect(this, &QTableView::doubleClicked, [this](const QModelIndex& index) {
		emit playMusic(index, model_.item(proxy_model_.mapToSource(index)));
		setNowPlaying(index);
		});

	(void)QObject::connect(&adapter_, &MetadataExtractAdapter::finish, [this]() {
		resizeColumn();
		});

    auto f = font();
#ifdef Q_OS_WIN
    f.setPointSize(9);
#else
	f.setPointSize(11);
#endif
    setFont(f);

	adapter_.playlist = this;
}

void PlayListTableView::append(const QString& file_name) {
	const xamp::metadata::Path path(file_name.toStdWString());
	xamp::metadata::TaglibMetadataReader reader;
    reader.ExtractFromPath(path, &adapter_);
}

void PlayListTableView::resizeEvent(QResizeEvent* event) {
	QTableView::resizeEvent(event);
	resizeColumn();
}

void PlayListTableView::resizeColumn() const {
	auto header = horizontalHeader();

	for (auto column = 0; column < header->count(); ++column) {
		const auto width = header->sectionSize(column);

		switch (column) {
		case PLAYLIST_TRACK:
		case PLAYLIST_PLAYING:
			header->setSectionResizeMode(column, QHeaderView::Fixed);
			header->resizeSection(column, 8);
			break;
		case PLAYLIST_DURATION:
		case PLAYLIST_BITRATE:
			header->setSectionResizeMode(column, QHeaderView::Fixed);
			header->resizeSection(column, 60);
			break;
		case PLAYLIST_TITLE:
			header->resizeSection(column, 250);
			break;
		case PLAYLIST_ALBUM:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
			break;
		case PLAYLIST_ARTIST:
		default:
			header->setSectionResizeMode(column, QHeaderView::Stretch);
			break;
		}
	}
}

void PlayListTableView::appendItem(const xamp::base::Metadata& metadata) {
	appendItem(fromMetadata(metadata));
}

void PlayListTableView::appendItem(const PlayListEntity& item) {
	model_.append(item);
}

void PlayListTableView::search(const QString& sort_str, Qt::CaseSensitivity case_sensitivity, QRegExp::PatternSyntax pattern_syntax) {
	const QRegExp reg_exp(sort_str, case_sensitivity, pattern_syntax);

	proxy_model_.setFilterRegExp(reg_exp);
	proxy_model_.invalidate();
}

void PlayListTableView::setPlaylistId(const int32_t playlist_id) {
	playlist_id_ = playlist_id;
}

int32_t PlayListTableView::playlistId() const {
	return playlist_id_;
}

QModelIndex PlayListTableView::currentIndex() const {
	return play_index_;
}

QModelIndex PlayListTableView::shuffeIndex() {
	std::vector<int32_t> indexes;
	indexes.reserve(proxy_model_.rowCount());

	for (int n = 0; n < proxy_model_.rowCount(); n++) {
		if (model_.nowPlaying() != n) {
			indexes.push_back(n);
		}
	}

	xamp::base::RNG::Instance().Shuffle(indexes);
	auto selected = xamp::base::RNG::Instance()(size_t(0), indexes.size() - 1);
	return model()->index(indexes[selected], PLAYLIST_PLAYING);
}

void PlayListTableView::setNowPlaying(const QModelIndex& index, bool is_scroll_to) {
	play_index_ = index;
	model_.setNowPlaying(proxy_model_.mapToSource(index).row());
	setCurrentIndex(index);
	if (is_scroll_to) {
		// Note: 如果column index被hidden就無法使用scrollTo
		QTableView::scrollTo(index, PositionAtCenter);
	}
	proxy_model_.dataChanged(QModelIndex(), QModelIndex());
}

void PlayListTableView::scrollTo(const QModelIndex& index) {
	QTableView::scrollTo(index, PositionAtCenter);
}

std::optional<QModelIndex> PlayListTableView::selectItem() const {
	auto select_row = selectionModel()->selectedRows();
	if (select_row.isEmpty()) {
		return std::nullopt;
	}
	return select_row[0];
}

std::map<int32_t, QModelIndex> PlayListTableView::selectItemIndex() const {
	std::map<int32_t, QModelIndex> select_items;

	foreach(auto index, selectionModel()->selectedRows()) {
		auto const row = index.row();
		select_items.emplace(row, index);
	}
	return select_items;
}

PlayListEntity& PlayListTableView::item(const QModelIndex& index) {
	return model_.item(proxy_model_.mapToSource(index));
}

void PlayListTableView::play(const QModelIndex& index) {
	play_index_ = index;
	emit playMusic(index, item(index));
}

void PlayListTableView::removePlaying() {
	model_.setNowPlaying(-1);
	proxy_model_.dataChanged(QModelIndex(), QModelIndex());
}

void PlayListTableView::removeItem(const QModelIndex& index) {
	proxy_model_.removeRows(index.row(), 1, index);
}

void PlayListTableView::removeSelectItems() {
	const auto rows = selectItemIndex();
	
	QVector<int> remove_music_ids;

	auto index = 0;
	for (auto itr = rows.rbegin(); itr != rows.rend(); ++itr) {
		auto const music_id = model()->data((*itr).second);
		remove_music_ids.push_back(music_id.toInt());
		proxy_model_.removeRows((*itr).first, 1);
		if (model_.nowPlaying() == index) {
			removePlaying();
		}
		++index;
	}

	if (!remove_music_ids.empty()) {
		emit removeItems(playlist_id_, remove_music_ids);
	}

	if (model_.isEmpty()) {
		removePlaying();
	}
}
