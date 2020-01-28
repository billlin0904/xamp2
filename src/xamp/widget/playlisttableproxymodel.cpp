#include <widget/playlisttableproxymodel.h>

PlayListTableFilterProxyModel::PlayListTableFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent) {
}

void PlayListTableFilterProxyModel::setFilterByColumn(int32_t column) {
    filters_.insert(column);
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
