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
    setIconSize(qTheme.GetTabIconSize());

    (void)QObject::connect(this, &QListView::clicked, [this](auto index) {
        auto table_id = index.data(Qt::UserRole + 1).toInt();
        emit ClickedTable(table_id);
    });

    (void)QObject::connect(&model_, &QStandardItemModel::itemChanged, [this](auto item) {
        auto table_id = item->data(Qt::UserRole + 1).toInt();
        auto table_name = item->data(Qt::DisplayRole).toString();
        emit TableNameChanged(table_id, table_name);
    });
}

QString TabListView::GetTabName(int table_id) const {
	if (!names_.contains(table_id)) {
        return qEmptyString;
	}
    return names_[table_id];
}

int32_t TabListView::GetTabId(const QString& name) const {
    if (!ids_.contains(name)) {
        return -1;
    }
    return ids_[name];
}

void TabListView::OnCurrentThemeChanged(ThemeColor theme_color) {
    for (auto column_index = 0; column_index < model()->rowCount(); ++column_index) {
        auto* item = model_.item(column_index);
        switch (column_index) {
        case TAB_PLAYLIST:
            item->setIcon(qTheme.GetFontIcon(Glyphs::ICON_PLAYLIST));
            break;
        case TAB_FILE_EXPLORER:
            item->setIcon(qTheme.GetFontIcon(Glyphs::ICON_DESKTOP));
            break;
        case TAB_LYRICS:
            item->setIcon(qTheme.GetFontIcon(Glyphs::ICON_SUBTITLE));
            break;
        case TAB_PODCAST:
            item->setIcon(qTheme.GetFontIcon(Glyphs::ICON_PODCAST));
            break;
        case TAB_ALBUM:
            item->setIcon(qTheme.GetFontIcon(Glyphs::ICON_ALBUM));
            break;
        case TAB_ARTIST:
            item->setIcon(qTheme.GetFontIcon(Glyphs::ICON_ARTIST));
            break;
        case TAB_CD:
            item->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CD));
            break;
        }
    }
}

void TabListView::AddTab(const QString& name, int table_id, const QIcon& icon) {
    auto *item = new QStandardItem(name);
    item->setData(table_id);
    item->setIcon(icon);
    item->setSizeHint(QSize(18, 35));    
    auto f = item->font();
    f.setPointSize(qTheme.GetFontSize(10));
    item->setFont(f);
    model_.appendRow(item);
    names_[table_id] = name;
    ids_[name] = table_id;
}

void TabListView::AddSeparator() {
    auto* item = new QStandardItem();
    item->setFlags(Qt::NoItemFlags);
    auto* hline = new QFrame(this);
    item->setSizeHint(QSize(80, 2));
    hline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    model_.appendRow(item);
    const auto index = model_.indexFromItem(item);
    setIndexWidget(index, hline);
}
