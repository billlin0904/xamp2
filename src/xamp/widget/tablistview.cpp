#include <widget/str_utilts.h>

#include "thememanager.h"
#include "tablistview.h"

TabListView::TabListView(QWidget *parent)
    : QListView(parent)
    , model_(this) {
    setModel(&model_);
    setFrameStyle(QFrame::StyledPanel);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(2);
    setIconSize(qTheme.tabIconSize());

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

QString TabListView::getTabName(int table_id) const {
	if (!names_.contains(table_id)) {
        return qEmptyString;
	}
    return names_[table_id];
}

int32_t TabListView::getTabId(const QString& name) const {
    if (!ids_.contains(name)) {
        return -1;
    }
    return ids_[name];
}

void TabListView::addTab(const QString& name, int table_id, const QIcon& icon) {
    auto *item = new QStandardItem(name);
    item->setData(table_id);
    item->setIcon(icon);
    item->setSizeHint(QSize(18, 35));
    
    auto f = item->font();
#ifdef XAMP_OS_MAC
    f.setPointSize(15);
#else
    f.setPointSize(8);
#endif
    item->setFont(f);
    model_.appendRow(item);
    names_[table_id] = name;
    ids_[name] = table_id;
    item->setData(name, Qt::ToolTipRole);
}

void TabListView::addSeparator() {
    auto* item = new QStandardItem();
    item->setFlags(Qt::NoItemFlags);
    auto* hline = new QFrame(this);
    item->setSizeHint(QSize(80, 2));
    hline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    model_.appendRow(item);
    auto index = model_.indexFromItem(item);
    setIndexWidget(index, hline);
}
