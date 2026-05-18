#include <widget/playlisttableproxymodel.h>

#include <widget/playlisttablemodel.h>

PlayListTableFilterProxyModel::PlayListTableFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
}

void PlayListTableFilterProxyModel::addFilterByColumn(int32_t column) {
    filters_.push_back(column);
}

void PlayListTableFilterProxyModel::setAlbumFilterId(std::optional<int32_t> album_id) {
    album_filter_id_ = album_id;
    invalidateRowsFilter();
}

bool PlayListTableFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    if (album_filter_id_) {
        const auto album_index = sourceModel()->index(source_row, PLAYLIST_ALBUM_ID, source_parent);
        if (!album_index.isValid()
            || sourceModel()->data(album_index).toInt() != album_filter_id_.value()) {
            return false;
        }
    }

    auto pattern = filterRegularExpression().pattern();
    auto cs = filterCaseSensitivity();
	return std::any_of(filters_.begin(), filters_.end(), [source_row, source_parent, this, cs, pattern](auto filter) {
        auto index = sourceModel()->index(source_row, filter, source_parent);
        if (index.isValid()) {
            auto text = sourceModel()->data(index).toString();
            if (text.contains(pattern, cs)) {
                return true;
            }
        }
        return false;
        });
}
