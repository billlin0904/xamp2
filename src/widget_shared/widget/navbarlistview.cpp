#include <QScrollArea>
#include <widget/navbarlistview.h>
#include <widget/util/str_util.h>
#include <widget/xtooltip.h>
#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <thememanager.h>

NavBarListView::NavBarListView(QWidget *parent)
	: QFrame(parent) {
    setStyleSheet("border: none"_str);
	tooltip_ = new XTooltip();
    tooltip_->hide();

    main_layout_ = new NavItemLayout(this);
    main_layout_->setContentsMargins(0, 5, 0, 5);

    scroll_widget_ = new QWidget();
    scroll_area_ = new QScrollArea(this);
    scroll_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll_area_->setWidget(scroll_widget_);
    scroll_area_->setWidgetResizable(true);

	top_layout_ = new NavItemLayout();
    top_layout_->setContentsMargins(4, 0, 4, 0);
    top_layout_->setSpacing(4);
    top_layout_->setAlignment(Qt::AlignTop);

    scroll_layout_ = new NavItemLayout(scroll_widget_);
    scroll_layout_->setContentsMargins(4, 0, 4, 0);
    scroll_layout_->setSpacing(4);
    scroll_layout_->setAlignment(Qt::AlignTop);

    bottom_layout_ = new NavItemLayout();
    bottom_layout_->setContentsMargins(4, 0, 4, 0);
    bottom_layout_->setSpacing(4);
    bottom_layout_->setAlignment(Qt::AlignBottom);

    main_layout_->addLayout(bottom_layout_, 0);
    main_layout_->addWidget(scroll_area_, 1, Qt::AlignTop);
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
    return -1;
}

void NavBarListView::onRetranslateUi() {
    tooltip_->setText(kEmptyString);
}

void NavBarListView::toolTipMove(const QPoint& pos) {
    /*auto index = indexAt(pos);
    if (index.isValid()) {
        auto* item = model_.item(index.row(), index.column());
        auto tooltip_text = item->text();
        if (!tooltip_text.isEmpty() && qAppSettings.valueAsBool(kAppSettingHideNaviBar)) {
            if (tooltip_text != tooltip_->text() || elapsed_timer_.elapsed() > 500) {
                const auto item_rect = visualRect(index);
                const auto global_pos = viewport()->mapToGlobal(item_rect.topRight());
                tooltip_->setText(tooltip_text);
                tooltip_->move(global_pos + QPoint(5, -5));
                tooltip_->showAndStart();
                elapsed_timer_.restart();
            }
        }
    }*/
}

void NavBarListView::setCurrentIndex(int32_t tab_id) {
    QHashIterator<int32_t, NavWidget*> itr(widgets_);
	while (itr.hasNext()) {
		itr.next();
		if (itr.key() == tab_id) {
            itr.value()->setSelected(true);
            break;
		}
    }
}

void NavBarListView::collapse() {
    QHashIterator<int32_t, NavWidget*> itr(widgets_);
    while (itr.hasNext()) {
        itr.next();
        itr.value()->setCompacted(true);
    }
}

void NavBarListView::expand() {
    QHashIterator<int32_t, NavWidget*> itr(widgets_);
    while (itr.hasNext()) {
        itr.next();
        itr.value()->setCompacted(false);
    }
}

void NavBarListView::mouseMoveEvent(QMouseEvent* event) {
	/*toolTipMove(event->pos());
    QListView::mouseMoveEvent(event);*/
}

void NavBarListView::wheelEvent(QWheelEvent* event) {
    /*const auto count = model_.rowCount();
    const auto play_index = currentIndex();

	if (event->angleDelta().y() < 0) {
        const auto next_index = (play_index.row() + 1) % count;
        auto index = model()->index(next_index, 0);
        auto table_id = index.data(Qt::UserRole + 1).toInt();
        emit clickedTable(table_id);
        setCurrentIndex(index);
	}
	else {
        const auto next_index = (play_index.row() - 1) % count;
		if (next_index < 0) {
			return;
		}
        auto index = model()->index(next_index, 0);
        auto table_id = index.data(Qt::UserRole + 1).toInt();
        emit clickedTable(table_id);
        setCurrentIndex(index);
	}*/
}

void NavBarListView::onThemeChangedFinished(ThemeColor theme_color) {
   /* for (auto column_index = 0; column_index < model()->rowCount(); ++column_index) {
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
    tooltip_->onThemeChangedFinished(theme_color);*/
    if (widgets_.empty()) {
        return;
    }
    QHashIterator<int32_t, NavWidget*> itr(widgets_);
    while (itr.hasNext()) {
        itr.next();
        switch (itr.key()) {
        case TAB_PLAYLIST:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAYLIST));
            break;
        case TAB_FILE_EXPLORER:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_DESKTOP));
            break;
        case TAB_LYRICS:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
            break;
        case TAB_MUSIC_LIBRARY:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_MUSIC_LIBRARY));
            break;
        case TAB_CD:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_CD));
            break;
        /*case TAB_YT_MUSIC_SEARCH:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_YOUTUBE));
            break;
        case TAB_YT_MUSIC_PLAYLIST:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_YOUTUBE_PLAYLIST));
            break;
        case TAB_AI:
            itr.value()->setIcon(qTheme.fontIcon(Glyphs::ICON_AI));
            break;*/
        }
    }
    tooltip_->onThemeChangedFinished(theme_color);
}

void NavBarListView::setTabText(const QString& name, int table_id) {
    /*for (auto column_index = 0; column_index < model()->rowCount(); ++column_index) {
        auto* item = model_.item(column_index);
        if (column_index == table_id) {
			item->setText(name);
            break;
        }
    }

    names_[table_id] = name;
    ids_[name] = table_id;
    tooltip_->setText(name);*/
}

void NavBarListView::addTab(const QString& name, int table_id, const QIcon& icon) {
 //   auto *item = new QStandardItem(name);
 //   item->setData(table_id);
 //   item->setIcon(icon);
 //   item->setSizeHint(QSize(36, 36));      
 //   item->setData(name, Qt::UserRole);
 //   auto f = item->font();
 //   f.setPointSize(qTheme.fontSize(10));
 //   item->setFont(f);
 //   model_.appendRow(item);
    names_[table_id] = name;
    ids_[name] = table_id;	
	//// Prepare tooltip text width
 //   tooltip_->setText(name);
    auto* button = new NavPushButton(icon, name, true, this);
	widgets_[table_id] = button;
	(void)QObject::connect(button, &NavPushButton::clicked, [this, table_id]() mutable  {
        QHashIterator<int32_t, NavWidget*> itr(widgets_);
        while (itr.hasNext()) {
            itr.next();
			if (itr.key() == table_id) {
                itr.value()->setSelected(true);
                emit clickedTable(table_id);
			}
			else {
                itr.value()->setSelected(false);
			}
		}
		});
    button->setCompacted(false);
    scroll_layout_->addWidget(button, 0, Qt::AlignTop);
}

void NavBarListView::addSeparator() {
    /*auto* item = new QStandardItem();
    item->setFlags(Qt::NoItemFlags);
    auto* hline = new QFrame(this);
    item->setSizeHint(QSize(80, 2));
    hline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    model_.appendRow(item);
    const auto index = model_.indexFromItem(item);
    setIndexWidget(index, hline);*/
    auto* separator = new NavSeparator(this);
    top_layout_->addWidget(separator, 0, Qt::AlignTop);
}
