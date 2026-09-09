#include <style_util.h>
#include <thememanager.h>

#include <QTableView>
#include <QHeaderView>

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

void setThemeIcon(Ui::XampWindow& ui) {
    ui.nextButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_FORWARD));
    ui.prevButton->setIcon(qTheme.fontIcon(Glyphs::ICON_PLAY_BACKWARD));
    ui.selectDeviceButton->setIcon(qTheme.fontIcon(Glyphs::ICON_SPEAKER));
    ui.selectDeviceButton->setStyleSheet(
        "QToolButton#selectDeviceButton::menu-indicator { image: none; }"_str);
    ui.eqButton->setIcon(qTheme.fontIcon(Glyphs::ICON_EQUALIZER));
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
}

void setWidgetStyle(Ui::XampWindow& ui) {
    ui.playButton->setStyleSheet(QString());
    ui.artistLabel->setStyleSheet(QString());
    ui.bottomFrame->setStyleSheet(QString());
    ui.currentView->setStyleSheet(QString());
    ui.currentView->setProperty("topLeftCornerOutsideColor", qTheme.backgroundColorString());
    ui.selectDeviceButton->setIconSize(QSize(20, 20));
    ui.startPosLabel->setFont(qTheme.defaultFont());
    ui.endPosLabel->setFont(qTheme.defaultFont());
    ui.seekSlider->setProperty("compactSeek", true);
    setThemeIcon(ui);
}

void updateButtonState(QToolButton* playButton, PlayerState state) {
    qTheme.setPlayOrPauseButton(playButton, state == PlayerState::PLAYER_STATE_RUNNING || state == PlayerState::PLAYER_STATE_RESUME);
}
