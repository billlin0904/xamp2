//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <map>
#include <memory>

#include "thememanager.h"
#include <QMenu>

template <typename Type, typename ActionFunc>
class ActionMap {
public:
    typedef std::map<QAction*, ActionFunc> MapType;

    class SubMenu {
    public:
        SubMenu(const QString &menu_name, QMenu *menu, MapType &action_map)
            : action_map_(action_map) {
            submenu_ = new QMenu(menu_name, menu);
            menu->addMenu(submenu_);
            action_group_ = new QActionGroup(submenu_);
        }

        template <typename Callable>
        QAction* addAction(const QString &menu_name, Callable &&callback, bool checked = false, bool add_eparator = false) {
            const auto action = new QAction(menu_name, nullptr);
            action->setCheckable(true);

            if (checked)
                action->setChecked(checked);

            action_map_[action] = callback;
            submenu_->addAction(action);

            if (add_eparator)
                submenu_->addSeparator();

            submenu_->addAction(action);
            action_group_->addAction(action);

            return action;
        }
    private:
        QMenu * submenu_;
        QActionGroup *action_group_;
        MapType &action_map_;
    };

	explicit ActionMap(Type *object)
		: object_(object)
		, menu_(object) {
        setThemeSetyle();
	}

    ActionMap(QWidget *app, Type *object) 
        : object_(object)
        , menu_(app) {
        setThemeSetyle();
    }

    void setThemeSetyle() {
        setStyleSheet(ThemeManager::instance().getMenuStyle());
    }

    template <typename Callable>
    QAction* addAction(const QString &menu_name, Callable &&callback, bool add_eparator = false, bool checked = false) {
        const auto action = new QAction(menu_name, nullptr);

        map_[action] = callback;
        menu_.addAction(action);
        
        if (add_eparator)
            menu_.addSeparator();

		action->setCheckable(true);
		if (checked)
			action->setChecked(checked);
        
        return action;
    }

    std::unique_ptr<SubMenu> addSubMenu(const QString &menu_name) {
        return std::make_unique<SubMenu>(menu_name, &menu_, map_);
    }

    void addSeparator() {
	    menu_.addSeparator();
	}

    template <typename... Args>
    void exec(const QPoint &point, Args... args) {
        auto globalpos = object_->mapToGlobal(point);

        auto itr = map_.find(menu_.exec(globalpos));
        if (itr != map_.end()) {
            (*itr).second(std::forward<Args>(args)...);
        }
    }

    void setStyleSheet(const QString& stylesheet) {
        menu_.setStyleSheet(stylesheet);
    }
private:
    Type * object_;
    QMenu menu_;    
    std::map<QAction*, ActionFunc> map_;
};
