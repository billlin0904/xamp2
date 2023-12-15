#include <widget/tablistview.h>
#include <widget/str_utilts.h>
#include <thememanager.h>

TabListView::TabListView(QWidget *parent)
    : QListView(parent)
    , model_(this) {
    setModel(&model_);
    setFrameStyle(QFrame::StyledPanel);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(2);
    setIconSize(QSize(22, 22));

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

QString TabListView::tabName(int table_id) const {
	if (!names_.contains(table_id)) {
        return kEmptyString;
	}
    return names_[table_id];
}

int32_t TabListView::tabId(const QString& name) const {
    if (!ids_.contains(name)) {
        return -1;
    }
    return ids_[name];
}

void TabListView::onCurrentThemeChanged(ThemeColor theme_color) {
    for (auto column_index = 0; column_index < model()->rowCount(); ++column_index) {
        auto* item = model_.item(column_index);
        switch (column_index) {
        case TAB_PLAYLIST:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
            break;
        case TAB_FILE_EXPLORER:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_DESKTOP));
            break;
        case TAB_LYRICS:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
            break;
        case TAB_MUSIC_LIBRARY:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_MUSIC_LIBRARY));
            break;        
        case TAB_CD:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_CD));
            break;
        }
    }
}

void TabListView::addTab(const QString& name, int table_id, const QIcon& icon) {
    auto *item = new QStandardItem(name);
    item->setData(table_id);
    item->setIcon(icon);
    item->setSizeHint(QSize(35, 35));    
    auto f = item->font();
    f.setBold(true);
    f.setPointSize(qTheme.fontSize(11));
    item->setFont(f);
    model_.appendRow(item);
    names_[table_id] = name;
    ids_[name] = table_id;
}

void TabListView::addSeparator() {
    auto* item = new QStandardItem();
    item->setFlags(Qt::NoItemFlags);
    auto* hline = new QFrame(this);
    item->setSizeHint(QSize(80, 2));
    hline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    model_.appendRow(item);
    const auto index = model_.indexFromItem(item);
    setIndexWidget(index, hline);
}
