//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/playerorder.h>
#include <ui_xamp.h>

class TabListView;

void setShufflePlayOrder(Ui::XampWindow& ui);

void setRepeatOnePlayOrder(Ui::XampWindow& ui);

void setRepeatOncePlayOrder(Ui::XampWindow& ui);

void setAuthButton(Ui::XampWindow& ui, bool auth);

void setThemeIcon(Ui::XampWindow& ui);

void setRepeatButtonIcon(Ui::XampWindow& ui, PlayerOrder order);

void setNaviBarTheme(TabListView* navi_bar);

void setWidgetStyle(Ui::XampWindow& ui);

void updateButtonState(QToolButton* playButton, PlayerState state);

void setNaviBarMenuButton(Ui::XampWindow& ui);
