#include <style_util.h>
#include <thememanager.h>
#include <widget/util/str_util.h>

void setShufflePlayOrder(Ui::XampWindow& ui) {
    ui.repeatButton->setIcon(qTheme.fontIcon(Glyphs::ICON_SHUFFLE_PLAY_ORDER));
}

void setRepeatOnePlayOrder(Ui::XampWindow& ui) {
    ui.repeatButton->setIcon(qTheme.fontIcon(Glyphs::ICON_REPEAT_ONE_PLAY_ORDER));
}

void setRepeatOncePlayOrder(Ui::XampWindow& ui) {
    ui.repeatButton->setIcon(qTheme.fontIcon(Glyphs::ICON_REPEAT_ONCE_PLAY_ORDER));
}

void setNaviBarMenuButton(Ui::XampWindow& ui) {
    if (qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        ui.naviBarButton->setStyleSheet(qTEXT(R"(
        QToolButton#naviBarButton {
			border: none;
			background-color: transparent;
		}

        QToolButton#naviBarButton:hover {
			background-color: #e1e3e5;
			border-radius: 4px;
		}
		)"));
    }
    else {
        ui.naviBarButton->setStyleSheet(qTEXT(R"(
        QToolButton#naviBarButton {
			border: none;
			background-color: transparent;
		}

        QToolButton#naviBarButton:hover {
			background-color: #43474e;
			border-radius: 4px;
		}
		)"));
    }
    ui.naviBarButton->setIconSize(QSize(22, 22));
    ui.naviBarButton->setIcon(qTheme.fontIcon(Glyphs::ICON_MENU));
}

void setAuthButton(Ui::XampWindow& ui, bool auth) {
    if (qTheme.themeColor() == ThemeColor::LIGHT_THEME) {
        ui.authButton->setStyleSheet(qTEXT(R"(
        QToolButton#authButton {
			border: none;
			background-color: transparent;
		}

		QToolButton#authButton:hover {
			background-color: #e1e3e5;
			border-radius: 24px;
		}
		)"));
    }
    else {
        ui.authButton->setStyleSheet(qTEXT(R"(
        QToolButton#authButton {
			border: none;
			background-color: transparent;
		}

		QToolButton#authButton:hover {
			background-color: #43474e;
			border-radius: 24px;
		}
		)"));
    }

    if (!auth) {
        ui.authButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PERSON_UNAUTHORIZATIONED));
    }
    else {
        ui.authButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PERSON));
    }
}

void setThemeIcon(Ui::XampWindow& ui) {
#if 0
    qTheme.setTitleBarButtonStyle(ui.closeButton, ui.minWinButton, ui.maxWinButton);

    const QColor hover_color = qTheme.hoverColor();

    ui.logoButton->setStyleSheet(qSTR(R"(
                                         QToolButton#logoButton {
                                         border: none;
                                         image: url(":/xamp/xamp.ico");
                                         background-color: transparent;
                                         }
										)"));

    ui.menuButton->setStyleSheet(qSTR(R"(
                                         QToolButton#menuButton {                                                
                                                border: none;
                                                background-color: transparent;                                                
                                         }
                                         QToolButton#menuButton::menu-indicator { image: none; }
										 QToolButton#menuButton:hover {												
											background-color: %1;
											border-radius: 0px;								 
									     }
                                         )").arg(colorToString(qTheme.hoverColor())));

    ui.menuButton->setIcon(qTheme.fontIcon(Glyphs::ICON_MORE));
#else
    ui.nextButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.nextButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_FORWARD));

    ui.prevButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.prevButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_BACKWARD));

    ui.selectDeviceButton->setStyleSheet(qTEXT(R"(
                                                QToolButton#selectDeviceButton {                                                
                                                border: none;
                                                background-color: transparent;                                                
                                                }
                                                QToolButton#selectDeviceButton::menu-indicator { image: none; }
                                                )"));
    ui.selectDeviceButton->setIcon(qTheme.fontIcon(Glyphs::ICON_SPEAKER));

    ui.mutedButton->setStyleSheet(qSTR(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/%1/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )").arg(qTheme.themeColorPath()));

    ui.eqButton->setStyleSheet(qTEXT(R"(
                                         QToolButton#eqButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.authButton->setIconSize(QSize(24, 24));

    ui.eqButton->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));

    ui.repeatButton->setStyleSheet(qTEXT(R"(
    QToolButton#repeatButton {
    border: none;
    background: transparent;
    }
    )"
    ));
#endif
    ui.naviBarButton->setIcon(qTheme.fontIcon(Glyphs::ICON_MENU));
}

void setRepeatButtonIcon(Ui::XampWindow& ui, PlayerOrder order) {
    switch (order) {
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONCE:
        setRepeatOncePlayOrder(ui);
        break;
    case PlayerOrder::PLAYER_ORDER_REPEAT_ONE:
        setRepeatOnePlayOrder(ui);
        break;
    case PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM:
        setShufflePlayOrder(ui);
        break;
    default:
        break;
    }
}

void setNaviBarTheme(NavBarListView* navi_bar) {
    QString tab_left_color;

    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        tab_left_color = qTEXT("42, 130, 218");
        break;
    case ThemeColor::LIGHT_THEME:
        tab_left_color = qTEXT("42, 130, 218");
        break;
    }

    navi_bar->setStyleSheet(qSTR(R"(
	QListView#naviBar {
		border: none; 
	}
	QListView#naviBar::item {
		border: 0px;
		padding-left: 6px;
	}
	QListView#naviBar::item:hover {
		border-radius: 2px;
	}
	QListView#naviBar::item:selected {
		padding-left: 4px;		
		border-left-width: 2px;
		border-left-style: solid;
		border-left-color: rgb(%1);
	}	
	)").arg(tab_left_color));
}

void setWidgetStyle(Ui::XampWindow& ui) {
    ui.selectDeviceButton->setIconSize(QSize(32, 32));

    ui.playButton->setStyleSheet(qTEXT(R"(
                                            QToolButton#playButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));

    QFont duration_font(qTEXT("MonoFont"));
    duration_font.setPointSize(8);
    duration_font.setWeight(QFont::Bold);

    ui.startPosLabel->setStyleSheet(qTEXT(R"(
                                           QLabel#startPosLabel {
                                           color: gray;
                                           background-color: transparent;
                                           }
                                           )"));

    ui.endPosLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#endPosLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));
    ui.startPosLabel->setFont(duration_font);
    ui.endPosLabel->setFont(duration_font);

    /*ui.titleFrameLabel->setStyleSheet(qSTR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
    }
    )"));*/

    if (qTheme.themeColor() == ThemeColor::DARK_THEME) {
        ui.titleLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

        ui.currentView->setStyleSheet(qSTR(R"(
			QStackedWidget#currentView {
				padding: 0px;
				background-color: #121212;				
				border-top-left-radius: 0px;
            }			
            )"));

        ui.bottomFrame->setStyleSheet(
            qTEXT(R"(
            QFrame#bottomFrame{
                border-top: 1px solid black;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
    }
    else {
        ui.titleLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#titleLabel {
                                         color: black;
                                         background-color: transparent;
                                         }
                                         )"));

        ui.currentView->setStyleSheet(qTEXT(R"(
			QStackedWidget#currentView {
				background-color: #f9f9f9;
				border-top-left-radius: 0px;			
            }			
            )"));

        ui.bottomFrame->setStyleSheet(
            qTEXT(R"(
            QFrame#bottomFrame {
                border-top: 1px solid #eaeaea;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
    }

    ui.artistLabel->setStyleSheet(qTEXT(R"(
                                         QLabel#artistLabel {
                                         color: rgb(250, 88, 106);
                                         background-color: transparent;
                                         }
                                         )"));

    setNaviBarTheme(ui.naviBar);
    qTheme.setSliderTheme(ui.seekSlider);

    ui.deviceDescLabel->setStyleSheet(qTEXT("background: transparent;"));

    setThemeIcon(ui);
    ui.sliderFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));
    ui.sliderFrame2->setStyleSheet(qTEXT("background: transparent; border: none;"));
	// NOTE: Setting background color to transparent has effect on child widget's background color.
    ui.currentViewFrame->setStyleSheet(qTEXT("background: transparent; border: none;"));   
}

void updateButtonState(QToolButton* playButton, PlayerState state) {
    qTheme.setPlayOrPauseButton(playButton, state != PlayerState::PLAYER_STATE_PAUSED);
}