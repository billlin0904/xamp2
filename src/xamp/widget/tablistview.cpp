#include <widget/str_utilts.h>
#include "tablistview.h"

TabListView::TabListView(QWidget *parent)
    : QListView(parent)
    , model_(this) {
    setModel(&model_);
    setFrameStyle(QFrame::NoFrame);    
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    (void)QObject::connect(this, &QListView::clicked, [this](auto index) {
        auto table_id = index.data(Qt::UserRole + 1).toInt();
        emit clickedTable(table_id);
    });

    (void)QObject::connect(&model_, &QStandardItemModel::itemChanged, [this](auto item) {
        auto table_id = item->data(Qt::UserRole + 1).toInt();
        auto table_name = item->data(Qt::DisplayRole).toString();
        emit tableNameChanged(table_id, table_name);
    });
}

void TabListView::addTab(const QString& name, int table_id, const QIcon& icon) {
    auto item = new QStandardItem(name);
    item->setData(table_id);
    item->setIcon(icon);
    model_.appendRow(item);
}
