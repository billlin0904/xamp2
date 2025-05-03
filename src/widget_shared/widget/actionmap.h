//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPoint>
#include <QScopedPointer>
#include <QActionGroup>
#include <QProxyStyle>

#include <widget/widget_shared_global.h>
#include <widget/util/str_util.h>
#include <thememanager.h>

class XAMP_WIDGET_SHARED_EXPORT XMenu : public QMenu {
public:
	explicit XMenu(QWidget* object = nullptr)
		: QMenu(object) {
		qTheme.setMenuStyle(this);		
	}
};

class XAMP_WIDGET_SHARED_EXPORT XAction : public QAction {
	Q_OBJECT
public:
	explicit XAction(Glyphs glyphs, const QString& text, QObject* parent = nullptr)
		: QAction(qTheme.fontIcon(glyphs), text, parent)
		, glyphs_(glyphs){
	}

public slots:
	void onThemeChangedFinished(ThemeColor theme_color) {
		setIcon(qTheme.fontIcon(glyphs_));
	}

private:
	Glyphs glyphs_;
};

template <typename Type, typename F = std::function<void()>>
class ActionMap {
public:
	using MapType = OrderedMap<QAction*, F>;

	class SubMenu {
	public:
		SubMenu(const QString& menu_name, XMenu* menu, MapType& action_map)
			: action_group_(new QActionGroup(submenu_.get()))
			, action_map_(action_map) {
			submenu_.reset(menu->addMenu(menu_name));
            qTheme.setMenuStyle(submenu_.get());
		}

		void addSeparator() {
			submenu_->addSeparator();
		}

		template <typename Callable>
		QAction* addAction(const QString& menu_name,
			Callable&& callback,
			const bool checked = false,
			const bool add_separator = false) {
			const auto action = new QAction(menu_name, nullptr);
			action->setCheckable(true);

			action->setChecked(checked);

			action_map_[action] = callback;
			submenu_->addAction(action);

			if (add_separator)
				submenu_->addSeparator();

			submenu_->addAction(action);
			action_group_->addAction(action);

			return action;
		}
	private:
		QScopedPointer<QMenu> submenu_;
		QScopedPointer<QActionGroup> action_group_;
		MapType& action_map_;
	};

	explicit ActionMap(Type* object)
		: object_(object)
		, menu_(nullptr) {
	}

	QAction* addAction(const QString& menu_name) {
		auto action = addAction(menu_name, []() {});
		action->setEnabled(false);
		return action;
	}

	template <typename Callable>
	void setCallback(QAction* action, Callable&& callback) {
		map_[action] = callback;
		action->setEnabled(true);
	}

	template <typename Callable>
	QAction* addAction(const QString& menu_name,
		Callable&& callback,
		const bool add_separator = false,
		const bool checked = false) {
		auto* action = new QAction(menu_name, nullptr);

		map_[action] = callback;
		menu_.addAction(action);

        if (add_separator) {
            menu_.addSeparator();
        }

        if (checked) {
            action->setCheckable(true);
            action->setChecked(checked);
        } else {
            action->setCheckable(false);
        }

		return action;
	}

	SubMenu* addSubMenu(const QString& menu_name) {
		return new SubMenu(menu_name, &menu_, map_);
	}

	void addSeparator() {
		menu_.addSeparator();
	}

	template <typename... Args>
	void exec(const QPoint& point, Args... args) {
		auto globalpos = object_->mapToGlobal(point);

		auto itr = map_.find(menu_.exec(globalpos));
		if (itr != map_.end()) {
			(*itr).second(std::forward<Args>(args)...);
		}
	}

private:	
	Type* object_;
	XMenu menu_;
	MapType map_;
};
