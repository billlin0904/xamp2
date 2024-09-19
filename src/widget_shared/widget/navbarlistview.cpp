#include <widget/navbarlistview.h>
#include <widget/util/str_util.h>

#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <thememanager.h>

NavBarListView::NavBarListView(QWidget *parent)
    : QListView(parent)
    , model_(this) {
    setModel(&model_);
    setFrameStyle(QFrame::StyledPanel);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(2);

#ifdef Q_OS_WIN
    setIconSize(QSize(28, 28));
#else
    setIconSize(QSize(18, 18));
#endif    

    (void)QObject::connect(this, &QListView::clicked, [this](auto index) {
        auto table_id = index.data(Qt::UserRole + 1).toInt();
        emit clickedTable(table_id);
    });

    (void)QObject::connect(&model_, &QStandardItemModel::itemChanged, [this](auto item) {
        auto table_id = item->data(Qt::UserRole + 1).toInt();
        auto table_name = item->data(Qt::DisplayRole).toString();
        emit tableNameChanged(table_id, table_name);
    });

    setStyleSheet("border: none"_str);
    tooltip_.hide();
}

QString NavBarListView::tabName(int table_id) const {
	if (!names_.contains(table_id)) {
        return kEmptyString;
	}
    return names_[table_id];
}

int32_t NavBarListView::tabId(const QString& name) const {
    if (!ids_.contains(name)) {
        return -1;
    }
    return ids_[name];
}

int32_t NavBarListView::currentTabId() const {
    auto index = currentIndex();
    if (!index.isValid()) {
        return -1;
    }
    auto table_id = index.data(Qt::UserRole + 1).toInt();
    return table_id;
}

void NavBarListView::onRetranslateUi() {
    tooltip_.setText(kEmptyString);
}

void NavBarListView::toolTipMove(const QPoint& pos) {
    auto index = indexAt(pos);
    if (index.isValid()) {
        auto* item = model_.item(index.row(), index.column());
        auto tooltip_text = item->text();
        if (!tooltip_text.isEmpty() && qAppSettings.valueAsBool(kAppSettingHideNaviBar)) {
            if (tooltip_text != tooltip_.text() || elapsed_timer_.elapsed() > 500) {
                const auto item_rect = visualRect(index);
                const auto global_pos = viewport()->mapToGlobal(item_rect.topRight());
                tooltip_.setText(tooltip_text);
                tooltip_.move(global_pos + QPoint(5, -5));
                tooltip_.showAndStart();
                elapsed_timer_.restart();
            }
        }
    }
}

void NavBarListView::mouseMoveEvent(QMouseEvent* event) {
	toolTipMove(event->pos());
    QListView::mouseMoveEvent(event);
}

void NavBarListView::onThemeChangedFinished(ThemeColor theme_color) {
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
        case TAB_YT_MUSIC_SEARCH:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_YOUTUBE));
            break;
        case TAB_YT_MUSIC_PLAYLIST:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_YOUTUBE_PLAYLIST));
            break;
        case TAB_AI:
            item->setIcon(qTheme.fontIcon(Glyphs::ICON_AI));
            break;
        }
    }
	tooltip_.onThemeChangedFinished(theme_color);
}

void NavBarListView::setTabText(const QString& name, int table_id) {
    for (auto column_index = 0; column_index < model()->rowCount(); ++column_index) {
        auto* item = model_.item(column_index);
        if (column_index == table_id) {
			item->setText(name);
            break;
        }
    }

    names_[table_id] = name;
    ids_[name] = table_id;
    tooltip_.setText(name);
}

void NavBarListView::addTab(const QString& name, int table_id, const QIcon& icon) {
    auto *item = new QStandardItem(name);
    item->setData(table_id);
    item->setIcon(icon);
    item->setSizeHint(QSize(36, 36));      
    item->setData(name, Qt::UserRole);
    auto f = item->font();
    f.setPointSize(qTheme.fontSize(12));
    item->setFont(f);
    model_.appendRow(item);
    names_[table_id] = name;
    ids_[name] = table_id;	
	// Prepare tooltip text width
    tooltip_.setText(name);
}

void NavBarListView::addSeparator() {
    auto* item = new QStandardItem();
    item->setFlags(Qt::NoItemFlags);
    auto* hline = new QFrame(this);
    item->setSizeHint(QSize(80, 2));
    hline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    model_.appendRow(item);
    const auto index = model_.indexFromItem(item);
    setIndexWidget(index, hline);
}
