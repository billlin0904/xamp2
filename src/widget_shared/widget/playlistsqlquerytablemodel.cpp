#include <QSqlDriver>

#include <widget/playlistsqlquerytablemodel.h>

#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlistentity.h>

PlayListSqlQueryTableModel::PlayListSqlQueryTableModel(QObject *parent)
    : QSqlQueryModel(parent) {
}

int PlayListSqlQueryTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    if (rich_album_header_enabled_ && !rich_display_rows_.isEmpty()) {
        return rich_display_rows_.size();
    }
    return QSqlQueryModel::rowCount(parent);
}

QVariant PlayListSqlQueryTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }

    if (!rich_album_header_enabled_ || rich_display_rows_.isEmpty()) {
        if (role == PLAYLIST_ROW_TYPE_ROLE) {
            return PLAYLIST_ROW_TRACK;
        }
        return QSqlQueryModel::data(index, role);
    }

    if (index.row() < 0 || index.row() >= rich_display_rows_.size()) {
        return {};
    }

    const auto& row = rich_display_rows_[index.row()];
    if (role == PLAYLIST_ROW_TYPE_ROLE) {
        return row.is_album_header ? PLAYLIST_ROW_ALBUM_HEADER : PLAYLIST_ROW_TRACK;
    }
    if (role == PLAYLIST_ALBUM_TRACK_COUNT_ROLE) {
        return row.track_count;
    }
    if (role == PLAYLIST_ALBUM_DURATION_ROLE) {
        return row.duration;
    }

    const auto mapped_index = sourceIndex(index);
    if (!mapped_index.isValid()) {
        return {};
    }

    if (row.is_album_header && role == Qt::DisplayRole) {
        switch (index.column()) {
        case PLAYLIST_TRACK:
            return {};
        case PLAYLIST_TITLE:
            return QSqlQueryModel::data(QSqlQueryModel::index(row.source_row, PLAYLIST_ALBUM), role);
        default:
            break;
        }
    }

    return QSqlQueryModel::data(mapped_index, role);
}

Qt::ItemFlags PlayListSqlQueryTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return QAbstractTableModel::flags(index);
    }
    if (data(index, PLAYLIST_ROW_TYPE_ROLE).toInt() == PLAYLIST_ROW_ALBUM_HEADER) {
        return Qt::ItemIsEnabled;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant PlayListSqlQueryTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal) {
        if (role == Qt::TextAlignmentRole) {
            if (section == PLAYLIST_TRACK) {
                return { Qt::AlignCenter };
            }
        }
    }
    return QSqlQueryModel::headerData(section, orientation, role);
}

void PlayListSqlQueryTableModel::setRichAlbumHeaderEnabled(bool enabled) {
    if (rich_album_header_enabled_ == enabled) {
        return;
    }

    beginResetModel();
    rich_album_header_enabled_ = enabled;
    rebuildRichAlbumHeaderRows();
    endResetModel();
}

bool PlayListSqlQueryTableModel::isRichAlbumHeaderEnabled() const {
    return rich_album_header_enabled_;
}

void PlayListSqlQueryTableModel::refreshRichAlbumHeaderRows() {
    if (!rich_album_header_enabled_) {
        rich_display_rows_.clear();
        return;
    }

    beginResetModel();
    rebuildRichAlbumHeaderRows();
    endResetModel();
}

void PlayListSqlQueryTableModel::rebuildRichAlbumHeaderRows() {
    rich_display_rows_.clear();
    if (!rich_album_header_enabled_) {
        return;
    }

    const auto source_row_count = QSqlQueryModel::rowCount();
    rich_display_rows_.reserve(source_row_count + 16);

    auto source_row = 0;
    while (source_row < source_row_count) {
        const auto album_id = QSqlQueryModel::data(QSqlQueryModel::index(source_row, PLAYLIST_ALBUM_ID)).toInt();
        auto track_count = 0;
        auto duration = 0.0;
        auto next_row = source_row;

        while (next_row < source_row_count) {
            const auto next_album_id = QSqlQueryModel::data(QSqlQueryModel::index(next_row, PLAYLIST_ALBUM_ID)).toInt();
            if (next_album_id != album_id) {
                break;
            }
            ++track_count;
            duration += QSqlQueryModel::data(QSqlQueryModel::index(next_row, PLAYLIST_DURATION)).toDouble();
            ++next_row;
        }

        rich_display_rows_.push_back({ source_row, true, track_count, duration });
        for (auto track_row = source_row; track_row < next_row; ++track_row) {
            rich_display_rows_.push_back({ track_row, false, track_count, duration });
        }
        source_row = next_row;
    }
}

QModelIndex PlayListSqlQueryTableModel::sourceIndex(const QModelIndex& index) const {
    if (!index.isValid()) {
        return {};
    }
    if (!rich_album_header_enabled_ || rich_display_rows_.isEmpty()) {
        return QSqlQueryModel::index(index.row(), index.column());
    }
    if (index.row() < 0 || index.row() >= rich_display_rows_.size()) {
        return {};
    }
    return QSqlQueryModel::index(rich_display_rows_[index.row()].source_row, index.column());
}
