#include "ui_xamp.h"
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#if defined(Q_OS_WIN)
#include <widget/win32/fluentstyle.h>
#endif
#include <widget/appsettings.h>
#include "thememanager.h"

QColor ThemeManager::tableTextColor(Qt::black);
QColor ThemeManager::backgroundColor(228, 233, 237, 230);
QColor ThemeManager::controlBackgroundColor(228, 233, 237, 220);
QSize ThemeManager::defaultAlbumCoverSize(250, 250);
QSize ThemeManager::defaultCoverSize(150, 150);
QColor ThemeManager::menuColor(228, 233, 237, 150);
QColor ThemeManager::menuTextColor(Qt::black);

DefaultStylePixmapManager::DefaultStylePixmapManager()
	: unknown_cover_(Q_UTF8(":/xamp/Resource/White/unknown_album.png"))
	, default_size_unknown_cover_(Pixmap::resizeImage(unknown_cover_, ThemeManager::getDefaultCoverSize())) {
}

const QPixmap& DefaultStylePixmapManager::defaultSizeUnknownCover() const noexcept {
	return default_size_unknown_cover_;
}

const QPixmap& DefaultStylePixmapManager::unknownCover() const noexcept {
	return unknown_cover_;
}

const StylePixmapManager& ThemeManager::pixmap() noexcept {
	static const DefaultStylePixmapManager manager;
	return manager;
}

QIcon ThemeManager::volumeUp() {
	auto color = Q_UTF8("Black");
	return QIcon(QString(Q_UTF8(":/xamp/Resource/%1/volume_up.png")).arg(color));
}

QIcon ThemeManager::volumeOff() {
	auto color = Q_UTF8("Black");
	return QIcon(QString(Q_UTF8(":/xamp/Resource/%1/volume_off.png")).arg(color));
}

void ThemeManager::setPlayOrPauseButton(Ui::XampWindow& ui, bool is_playing) {
	auto color = Q_UTF8("Black");

	if (is_playing) {
		ui.playButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#playButton {
                                            image: url(:/xamp/Resource/%1/pause.png);
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )")).arg(color));
	}
	else {
		ui.playButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#playButton {
                                            image: url(:/xamp/Resource/%1/play.png);
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )")).arg(color));
	}
}

QString ThemeManager::getMenuStyle() noexcept {
	return Q_UTF8(R"(
                  QMenu {
                  background-color: rgba(228, 233, 237, 150);
                  }
                  QMenu::item:selected {
                  background-color: black;
                  }
                  )");
}

QSize ThemeManager::getDefaultCoverSize() noexcept {
	return defaultCoverSize;
}

QSize ThemeManager::getCacheCoverSize() noexcept {
	return getDefaultCoverSize() * 2;
}

QSize ThemeManager::getAlbumCoverSize() noexcept {
	return defaultAlbumCoverSize;
}

QColor ThemeManager::getBackgroundColor() noexcept {
	return backgroundColor;
}

void ThemeManager::enableBlur(const QWidget* widget, bool enable) {
#if defined(Q_OS_WIN)
	FluentStyle::setBlurMaterial(widget, enable);
	AppSettings::setValue(APP_SETTING_ENABLE_BLUR, enable);
#endif
}

void ThemeManager::setBackgroundColor(Ui::XampWindow& ui, QColor color) {
	ui.currentView->setStyleSheet(backgroundColorToString(color));
	ui.titleFrame->setStyleSheet(backgroundColorToString(color));

	QColor bottomColor = color.lighter(50);
	ui.playingFrame->setStyleSheet(backgroundColorToString(bottomColor));
	ui.volumeFrame->setStyleSheet(backgroundColorToString(bottomColor));

	QColor alphaColor = color;
	alphaColor.setAlpha(150);
	ui.sliderFrame->setStyleSheet(backgroundColorToString(alphaColor));
	ui.controlFrame->setStyleSheet(backgroundColorToString(alphaColor));

	AppSettings::setValue(APP_SETTING_BACKGROUND_COLOR, color);
	backgroundColor = color;
}

QIcon ThemeManager::playArrow() noexcept {
	auto color = Q_UTF8("Black");
	return QIcon(QString(Q_UTF8(":/xamp/Resource/%1/play_arrow.png")).arg(color));
}

void ThemeManager::shuffle(Ui::XampWindow& ui) {
	auto color = Q_UTF8("Black");
	ui.repeatButton->setStyleSheet(QString(Q_UTF8(R"(
                                              QToolButton#repeatButton {
                                              image: url(:/xamp/Resource/White/shuffle.png);
                                              background: transparent;
                                              }
                                              )")).arg(color));
}

void ThemeManager::repeatOne(Ui::XampWindow& ui) {
	auto color = Q_UTF8("Black");
	ui.repeatButton->setStyleSheet(QString(Q_UTF8(R"(
                                              QToolButton#repeatButton {
                                              image: url(:/xamp/Resource/White/repeat_one.png);
                                              background: transparent;
                                              }
                                              )")).arg(color));
}

void ThemeManager::repeatOnce(Ui::XampWindow& ui) {
	auto color = Q_UTF8("Black");
	ui.repeatButton->setStyleSheet(QString(Q_UTF8(R"(
                                              QToolButton#repeatButton {
                                              image: url(:/xamp/Resource/White/repeat.png);
                                              background: transparent;
                                              }
                                              )")).arg(color));
}

void ThemeManager::setWhiteIcon(Ui::XampWindow& ui) {
	auto color = Q_UTF8("Black");

	ui.titleLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

	ui.nextPageButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#nextPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/%1/right_black.png);
                                            background-color: transparent;
                                            }
                                            )")).arg(color));

	ui.backPageButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#backPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/%1/left_black.png);
                                            background-color: transparent;
                                            }
                                            )")).arg(color));

	ui.closeButton->setStyleSheet(QString(Q_UTF8(R"(
                                         QToolButton#closeButton {
                                         image: url(:/xamp/Resource/%1/close.png);
                                         background-color: transparent;
                                         }
                                         )")).arg(color));

	ui.minWinButton->setStyleSheet(QString(Q_UTF8(R"(
                                          QToolButton#minWinButton {
                                          image: url(:/xamp/Resource/%1/minimize.png);
                                          background-color: transparent;
                                          }
                                          )")).arg(color));

	ui.maxWinButton->setStyleSheet(QString(Q_UTF8(R"(
                                          QToolButton#maxWinButton {
                                          image: url(:/xamp/Resource/%1/maximize.png);
                                          background-color: transparent;
                                          }
                                          )")).arg(color));

	ui.settingsButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#settingsButton {
                                            image: url(:/xamp/Resource/%1/settings.png);
                                            background-color: transparent;
                                            }
                                            )")).arg(color));

	ui.stopButton->setStyleSheet(QString(Q_UTF8(R"(
                                         QToolButton#stopButton {
                                         image: url(:/xamp/Resource/%1/stop.png);
                                         background-color: transparent;
                                         }
                                         )")).arg(color));

	ui.nextButton->setStyleSheet(QString(Q_UTF8(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        image: url(:/xamp/Resource/%1/next.png);
                                        background-color: transparent;
                                        }
                                        )")).arg(color));

	ui.prevButton->setStyleSheet(QString(Q_UTF8(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        image: url(:/xamp/Resource/%1/previous.png);
                                        background-color: transparent;
                                        }
                                        )")).arg(color));

	ui.selectDeviceButton->setStyleSheet(QString(Q_UTF8(R"(
                                                QToolButton#selectDeviceButton {
                                                image: url(:/xamp/Resource/%1/speaker.png);
                                                border: none;
                                                background-color: transparent;
                                                }
                                                )")).arg(color));

	ui.mutedButton->setStyleSheet(QString(Q_UTF8(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/%1/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )")).arg(color));

	ui.repeatButton->setStyleSheet(QString(Q_UTF8(R"(
                                          QToolButton#repeatButton {
                                          image: url(:/xamp/Resource/%1/repeat.png);
                                          background-color: transparent;
                                          border: none;
                                          }
                                          )")).arg(color));

	ui.addPlaylistButton->setStyleSheet(QString(Q_UTF8(R"(
                                               QToolButton#addPlaylistButton {
                                               image: url(:/xamp/Resource/%1/create_new_folder.png);
                                               border: none;
                                               background-color: transparent;
                                               }
                                               )")).arg(color));

}

void ThemeManager::setDefaultStyle(Ui::XampWindow& ui) {
	if (!AppSettings::getValueAsString(APP_SETTING_BACKGROUND_COLOR).isEmpty()) {
		setBackgroundColor(ui, AppSettings::getValueAsString(APP_SETTING_BACKGROUND_COLOR));
	}
	else {
		setBackgroundColor(ui, backgroundColor);
	}

	ui.controlFrame->setStyleSheet(backgroundColorToString(controlBackgroundColor));
	ui.volumeFrame->setStyleSheet(backgroundColorToString(controlBackgroundColor));
	ui.playingFrame->setStyleSheet(backgroundColorToString(controlBackgroundColor));

	ui.searchLineEdit->setStyleSheet(Q_UTF8(""));
	ui.sliderBar->setStyleSheet(Q_UTF8("QListView#sliderBar { background-color: transparent; border: none; }"));

	setWhiteIcon(ui);

	ui.searchLineEdit->setStyleSheet(Q_UTF8(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: white;
                                            border: none;
                                            }
                                            )"));

	ui.playlistButton->setStyleSheet(Q_UTF8(R"(
                                            QToolButton#playlistButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));
	ui.volumeSlider->setStyleSheet(Q_UTF8(R"(
                                          QSlider#volumeSlider {
                                          border: none;
                                          background-color: transparent;
                                          }
                                          )"));

	ui.artistLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#artistLabel {
                                         }
                                         )"));

	ui.startPosLabel->setStyleSheet(Q_UTF8(R"(
                                           QLabel#startPosLabel {
                                           color: gray;
                                           background-color: transparent;
                                           }
                                           )"));

	ui.endPosLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#endPosLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));

	ui.seekSlider->setStyleSheet(Q_UTF8(R"(
                                        QSlider#seekSlider {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));

	ui.artistLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#artistLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));

	auto slider_style = Q_UTF8(R"(
                                         QSlider::handle:horizontal {
                                         width: 12px;
                                         background-color: rgb(255, 255, 255);
                                         margin: -5px 0px -5px 0px;
                                         border-radius: 6px;
                                         }

                                         QSlider::groove:horizontal {
                                         height: 2px;
                                         background-color: rgb(219, 219, 219);
                                         }

                                         QSlider::add-page:horizontal { 
                                         background-color: rgb(219, 219, 219);
                                         }

                                         QSlider::sub-page:horizontal {
                                         background-color: rgb(187, 134, 252);
                                         }
                                         )");
	ui.volumeSlider->setStyleSheet(slider_style);
	ui.seekSlider->setStyleSheet(slider_style);
}
