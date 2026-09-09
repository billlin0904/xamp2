#include <QScrollArea>
#include <widget/navbarlistview.h>
#include <widget/util/str_util.h>
#include <thememanager.h>

NavBarListView::NavBarListView(QWidget* parent)
    : QFrame(parent) {
    setStyleSheet("border: none"_str);
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 5, 0, 5);
    auto* scroll_widget = new QWidget(this);
    auto* scroll_area = new QScrollArea(this);
    scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area->setWidget(scroll_widget);
    scroll_area->setWidgetResizable(true);
    scroll_layout_ = new QVBoxLayout(scroll_widget);
    scroll_layout_->setContentsMargins(4, 0, 4, 0);
    scroll_layout_->setSpacing(6);
    scroll_layout_->setAlignment(Qt::AlignTop);
    main_layout->addWidget(scroll_area, 1, Qt::AlignTop);
    connect(&qTheme, &ThemeManager::themeChangedFinished,
        this, &NavBarListView::onThemeChangedFinished);
}

void NavBarListView::setCurrentIndex(int32_t tab_id) {
    for (auto it = widgets_.cbegin(); it != widgets_.cend(); ++it)
        it.value()->setSelected(it.key() == tab_id);
}

void NavBarListView::onThemeChangedFinished(ThemeColor theme_color) {
    Q_UNUSED(theme_color);
    if (widgets_.empty()) {
        return;
    }
    QHashIterator<int32_t, NavWidget*> itr(widgets_);
    while (itr.hasNext()) {
        itr.next();
        switch (itr.key()) {
        case TAB_RICH_PLAYLIST:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
            break;
        case TAB_FILE_EXPLORER:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_DESKTOP));
            break;
        case TAB_LYRICS:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
            break;
        case TAB_CD:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_CD));
            break;
        }
    }
}

void NavBarListView::addTab(const QString& name, int tab_id, const QIcon& icon) {
    auto* button = new NavPushButton(icon, name, true, this);
    button->setProperty("navigationId", tab_id);
    widgets_[tab_id] = button;
    connect(button, &NavPushButton::clicked, this, [this, tab_id] {
        setCurrentIndex(tab_id);
        emit clickedTable(tab_id);
    });
    scroll_layout_->addWidget(button, 0, Qt::AlignTop);
}

void NavBarListView::setTabText(int id, const QString& text) {
    if (auto* button = qobject_cast<NavPushButton*>(widgets_.value(id))) button->setText(text);
}
