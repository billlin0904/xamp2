//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <thememanager.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>

#include <QAbstractButton>
#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QListView>
#include <QPalette>
#include <QSize>
#include <QSlider>
#include <QToolButton>

void ThemeManager::setMenuStyle(QWidget* menu) {
	/*menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setAttribute(Qt::WA_StyledBackground);
    menu->setStyle(new IconSizeStyle(14));
    auto f = defaultFont();
    f.setPointSize(10);
    menu->setFont(f);*/
}

void ThemeManager::setBackgroundColor(QWidget* widget) {
    auto color = palette().color(QPalette::WindowText);
    widget->setStyleSheet(backgroundColorToString(color));
}

void ThemeManager::setTitleBarButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) {
    const QColor hover_color = hoverColor();
    const QColor color_hover_color("#dc3545"_str);

    close_button->setStyleSheet(qFormat(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         background-color: transparent;
										 border-radius: 0px;
                                         }

										 QToolButton#closeButton:hover {
										 background-color: %1;
										 border-radius: 0px;
                                         }
                                         )").arg(colorToString(color_hover_color)));
    close_button->setIconSize(QSize(titleBarIconHeight(), titleBarIconHeight()));
    close_button->setIcon(fontIcon(Glyphs::ICON_CLOSE_WINDOW));

    min_win_button->setStyleSheet(qFormat(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#minWinButton:hover {
										  background-color: %1;
										  border-radius: 0px;
                                          }
                                          )").arg(colorToString(hover_color)));
    min_win_button->setIconSize(QSize(titleBarIconHeight(), titleBarIconHeight()));
    min_win_button->setIcon(fontIcon(Glyphs::ICON_MINIMIZE_WINDOW));

    max_win_button->setStyleSheet(qFormat(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#maxWinButton:hover {
										  background-color: %1;
										  border-radius: 0px;
                                          }
                                          )").arg(colorToString(hover_color)));
    max_win_button->setIconSize(QSize(titleBarIconHeight(), titleBarIconHeight()));
    max_win_button->setIcon(fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
}

void ThemeManager::setFrameBackgroundColor(QFrame* frame) {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        frame->setStyleSheet(qFormat("QFrame#%1 { background-color: #1c1c1e; }").arg(frame->objectName()));
        break;
    case ThemeColor::LIGHT_THEME:
        frame->setStyleSheet(qFormat("QFrame#%1 { background-color: #CED1D4; }").arg(frame->objectName()));
        break;
    }
}

void ThemeManager::setComboBoxStyle(QComboBox* combo_box, const QString& object_name) {
    QString border_color;
    QString selection_background_color;
    QString on_selection_background_color;

    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        border_color = "#C9CDD0"_str;
        selection_background_color = "#FAFAFA"_str;
        on_selection_background_color = "#1e1d23"_str;
        break;
    case ThemeColor::DARK_THEME:
        border_color = "#455364"_str;
        selection_background_color = "#1e1d23"_str;
        on_selection_background_color = "#9FCBFF"_str;
        break;
    }

    combo_box->setStyleSheet(qFormat(R"(
    QComboBox#%4 {
		background-color: %2;
		border: 1px solid %1;
	}
	QComboBox QAbstractItemView#%4 {
		background-color: %2;
	}
	QComboBox#%4:on {
		selection-background-color: %3;
	}
    )").arg(border_color)
       .arg(selection_background_color)
       .arg(on_selection_background_color)
       .arg(object_name)
    );
}

void ThemeManager::setLineEditStyle(QLineEdit* line_edit, const QString& object_name) {
    switch (themeColor()) {
        case ThemeColor::DARK_THEME:
            line_edit->setStyleSheet(qFormat(R"(
                                            QLineEdit#%1 {
                                            background-color: %2;
                                            border: 1px solid #4d4d4d;
                                            color: white;
                                            border-radius: 12px;
                                            }
                                            )").arg(object_name).arg("#121212"_str));
			break;
        case ThemeColor::LIGHT_THEME:
            line_edit->setStyleSheet(qFormat(R"(
                                            QLineEdit#%1 {
                                            background-color: %2;
                                            border: 1px solid gray;
                                            color: black;
                                            border-radius: 12px;
                                            }
                                            )").arg(object_name).arg(colorToString(Qt::white)));
            break;
    }
}

void ThemeManager::setMuted(QAbstractButton *button, bool is_muted) {
    if (!is_muted) {
        button->setIcon(fontIcon(Glyphs::ICON_VOLUME_UP));
        qAppSettings.setValue<bool>(kAppSettingIsMuted, false);
    }
    else {
        button->setIcon(fontIcon(Glyphs::ICON_VOLUME_OFF));
        qAppSettings.setValue(kAppSettingIsMuted, true);
    }
}

void ThemeManager::setVolume(QSlider *slider, QAbstractButton* button, uint32_t volume) {
    if (!slider->isEnabled()) {
        return;
    }
    if (volume == 0) {
        setMuted(button, true);
    }
    else {
        setMuted(button, false);
    }
    slider->setValue(volume);
}

void ThemeManager::setSliderTheme(QSlider* slider, bool enter) {
    QString slider_background_color;
    QString slider_border_color;

    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        slider_background_color = "#9FCBFF"_str;
        slider_border_color = "#C9CDD0"_str;
        break;
    case ThemeColor::DARK_THEME:
        slider_background_color = "#1A72BB"_str;
        //slider_background_color = "#E8D65A"_str;
        slider_border_color = "#43474e"_str;
        break;
    }

    // handle 預設的邊框顏色
    auto handle_border_color = slider_background_color;

    // 原先的 margin 值
    auto margin = 10;
    if (!enter) {
        handle_border_color = "transparent"_str;
        margin = 1;
    }

    // 讓 handle 更圓：假設 handle 為 12x12，border-radius 設為 6 (直徑的一半)
    // 可以依需求自行調整大小
    const int handleSize = 12;
    const int handleRadius = handleSize / 2;

    slider->setStyleSheet(qFormat(R"(
    QSlider {
        background-color: transparent;
    }

    QSlider#%1::groove:vertical {
        background: %2;
        border: 1px solid %2;
        width: 2px;
        border-radius: 2px;
        padding-top: -1px;
        padding-bottom: 0px;
    }

    QSlider#%1::sub-page:vertical {
        background: %3;
        border: 1px solid %3;
        width: 2px;
        border-radius: 2px;
    }

    QSlider#%1::add-page:vertical {
        background: %2;
        border: 0px solid %2;
        width: 2px;
        border-radius: 2px;
    }

    QSlider#%1::handle:vertical {
        /* 設為相同的寬高 + 圓角，視覺上就會是圓 */
        width: %5px;
        height: %5px;
        margin: 0px -%6px 0px -%6px;  /* 可視需求微調 */
        background-color: %2;
        border: 1px solid %2;
        border-radius: %7px;
    }

    QSlider#%1::groove:horizontal {
        background: %2;
        border: 1px solid %2;
        height: 2px;
        border-radius: 2px;
        padding-left: -1px;
        padding-right: 1px;
    }

    QSlider#%1::sub-page:horizontal {
        background: %2;
        border: 1px solid %2;
        height: 2px;
        border-radius: 2px;
    }

    QSlider#%1::add-page:horizontal {
        background: %3;
        border: 0px solid %2;
        height: 2px;
        border-radius: 2px;
    }

    QSlider#%1::handle:horizontal {
        width: %5px;
        height: %5px;
        margin: -%6px 0px -%6px 0px; /* 可視需求微調 */
        border-radius: %7px;
        background-color: %4;
        border: 1px solid %4;
    }
    )"
    ).arg(slider->objectName())
        .arg(slider_background_color)
        .arg(slider_border_color)
        .arg(handle_border_color)
        .arg(handleSize)
        // margin 以 handleSize / 2 做微調，讓整個 handle 在中線上
        .arg(handleSize / 2)
        .arg(handleRadius)
    );
}

void ThemeManager::setAlbumNaviBarTheme(QListView *tab) const {
    QString tab_left_color;

    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        tab_left_color = "42, 130, 218"_str;
        break;
    case ThemeColor::LIGHT_THEME:
        tab_left_color = "42, 130, 218"_str;
        break;
    }

    tab->setStyleSheet(qFormat(R"(
	QListView#albumTab {
		border: none;
	}
	QListView#albumTab::item {
		border: 0px;
		padding: 2px;
	}
	QListView#albumTab::item:hover {
		background-color: transparent;
		border-radius: 2px;
	}
	QListView#albumTab::item:selected {
		padding: 2px;
		background-color: transparent;
        color: rgb(%2);
		border-bottom-width: 2px;
		border-bottom-style: solid;
		border-bottom-color: rgb(%1);
	}
	)").arg(tab_left_color).arg(tab_left_color));
}

