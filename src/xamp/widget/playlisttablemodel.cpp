#include "thememanager.h"
#include <widget/str_utilts.h>
#include <widget/time_utilts.h>
#include <widget/playlisttablemodel.h>

PlayListTableModel::PlayListTableModel(QObject* parent)
	: QAbstractTableModel(parent)
	, playing_index_(-1) {
}

PlayListEntity& PlayListTableModel::item(const QModelIndex& index) {
	return data_[index.row()];
}

QVariant PlayListTableModel::data(const QModelIndex& index, int32_t role) const {
	auto const row = index.row();

	switch (role) {
	case Qt::DisplayRole:
		switch (index.column()) {
		case PLAYLIST_MUSIC_ID:
			return data_[row].music_id;
		case PLAYLIST_TRACK:
            return data_[row].track;
		case PLAYLIST_FILEPATH:
			return data_[row].file_path;
		case PLAYLIST_TITLE:
			return data_[row].title;
		case PLAYLIST_FILE_NAME:
			return data_[row].file_name;
		case PLAYLIST_ALBUM:
			return data_[row].album;
		case PLAYLIST_ARTIST:
			return data_[row].artist;
		case PLAYLIST_DURATION:
			return Time::msToString(data_[row].duration);
		case PLAYLIST_BITRATE:
			if (data_[row].bitrate > 10000) {
				return QString(Q_UTF8("%0 Mbps")).arg(data_[row].bitrate / 1000.0);
			}
			return QString(Q_UTF8("%0 kbps")).arg(data_[row].bitrate);
		case PLAYLIST_SAMPLE_RATE:
			return data_[row].samplerate;
		case PLAYLIST_RATING:
			return QVariant::fromValue(StarRating(data_[row].rating));
		default:
			break;
		}
        break;
	case Qt::DecorationRole:
		if (index.column() == PLAYLIST_PLAYING && index.row() == playing_index_)
			return ThemeManager::instance().playArrow();
		break;
	case Qt::TextAlignmentRole:
		switch (index.column()) {
		case PLAYLIST_DURATION:
			return Qt::AlignCenter;
		}
	default:
		break;
	}
	return QVariant();
}

bool PlayListTableModel::removeRows(const int32_t position, const int32_t rows, const QModelIndex&) {
	beginRemoveRows(QModelIndex(), position, position + rows - 1);
	for (auto row = 0; row < rows; ++row) {
		data_.removeAt(position);
	}
	endRemoveRows();
	return true;
}

Qt::ItemFlags PlayListTableModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return QAbstractItemModel::flags(index);
	}
	auto flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
	if (index.column() == PLAYLIST_RATING) {
		flags |= Qt::ItemIsEditable;
	}
	return flags;
}

QVariant PlayListTableModel::headerData(const int32_t section, const Qt::Orientation orientation, const int32_t role) const {
	switch (role) {
	case Qt::DisplayRole:
		switch (orientation) {
		case Qt::Horizontal:
			switch (section) {
			case PLAYLIST_MUSIC_ID:
				return tr("Music Id");
			case PLAYLIST_PLAYING:
                return tr(" ");
			case PLAYLIST_TRACK:
				return tr("#");
			case PLAYLIST_FILEPATH:
				return tr("File path");
			case PLAYLIST_TITLE:
				return tr("Title");
			case PLAYLIST_FILE_NAME:
				return tr("File name");
			case PLAYLIST_ALBUM:
				return tr("Album");
			case PLAYLIST_ARTIST:
				return tr("Artist");
			case PLAYLIST_DURATION:
				return tr("Duration");
			case PLAYLIST_BITRATE:
				return tr("Bitrate");
			case PLAYLIST_SAMPLE_RATE:
				return tr("SampleRate");
			case PLAYLIST_RATING:
				return tr("Rating");
			default:
				break;
			}
		default:
			break;
		}
		break;
	case Qt::TextAlignmentRole:
		switch (section) {
		case PLAYLIST_DURATION:
			return Qt::AlignCenter;
		}
	default:
		return QVariant();
	}
	return QVariant();
}

int32_t PlayListTableModel::rowCount(QModelIndex const&) const {
	return data_.size();
}

int32_t PlayListTableModel::columnCount(QModelIndex const&) const {
	return _PLAYLIST_MAX_COLUMN_;
}

void PlayListTableModel::clear() {
	beginResetModel();
	data_.clear();
	endResetModel();
}

void PlayListTableModel::setNowPlaying(const int32_t playing_index) noexcept {
	playing_index_ = playing_index;
}

int32_t PlayListTableModel::nowPlaying() const noexcept {
	return playing_index_;
}

PlayListEntity& PlayListTableModel::item(int32_t index) {
	return data_[index];
}

void PlayListTableModel::removeRow(const int32_t pos) {
	emit beginRemoveRows(QModelIndex(), pos, pos);
	data_.removeAt(pos);
	emit endRemoveRows();
}

bool PlayListTableModel::insertRows(int row, int count, const QModelIndex& parent) {
	beginInsertRows(QModelIndex(), row, row + count - 1);
	endInsertRows();
	return true;
}

void PlayListTableModel::append(const PlayListEntity& item) {
	emit beginInsertRows(QModelIndex(), data_.size(), data_.size());
	data_.append(item);
	emit endInsertRows();
}

bool PlayListTableModel::isEmpty() const noexcept {
	return data_.isEmpty();
}
