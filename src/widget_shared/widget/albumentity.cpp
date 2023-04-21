#include <widget/albumentity.h>

QVariant GetIndexValue(const QModelIndex& index, const QModelIndex& src, int i) {
    return index.model()->data(index.model()->index(src.row(), i));
}

QVariant GetIndexValue(const QModelIndex& index, int i) {
    return index.model()->data(index.model()->index(index.row(), i));
}