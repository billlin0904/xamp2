#include "tablistview.h"

TabListView::TabListView(QWidget *parent)
    : QListView(parent)
    , model_(this) {
    setModel(&model_);
    (void)QObject::connect(this, &QListView::clicked, [this](auto index) {
        auto table_id = index.data(Qt::UserRole + 1).toInt();
        emit clickedTable(table_id);
    });
}

void TabListView::addTab(const QString& name, int table_id) {
    auto item = new QStandardItem(name);
    item->setData(table_id);
    model_.appendRow(item);
}
