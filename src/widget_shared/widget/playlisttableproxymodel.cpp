#include <widget/playlisttableproxymodel.h>

#include <widget/starrating.h>
#include <widget/playlisttablemodel.h>

PlayListTableFilterProxyModel::PlayListTableFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
}

void PlayListTableFilterProxyModel::SetFilterByColumn(int32_t column) {
    filters_.insert(column);
}

bool PlayListTableFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
	const auto left_data = sourceModel()->data(left);
	const auto right_data = sourceModel()->data(right);
    if (left.column() == PlayListColumn::PLAYLIST_RATING) {
        const auto left_rating = qvariant_cast<StarRating>(left_data);
        const auto right_rating = qvariant_cast<StarRating>(right_data);
        return left_rating.starCount() > right_rating.starCount();
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

bool PlayListTableFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    auto found = false;

    for (const auto& filter : filters_) {
        auto index = sourceModel()->index(source_row, filter, source_parent);
        if (index.isValid()) {
            auto text = sourceModel()->data(index).toString();
            if (text.contains(filterRegExp())) {
                found = true;
                break;
            }
        }
    }
    return found;
}
