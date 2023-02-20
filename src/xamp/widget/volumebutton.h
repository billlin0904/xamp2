#pragma once

#include <QToolButton>
#include <widget/themecolor.h>
#include <widget/widget_shared.h>

namespace xamp {
	namespace player {
		class IAudioPlayer;
	}
}

class VolumeControlDialog;

class VolumeButton : public QToolButton {
	Q_OBJECT
public:
	explicit VolumeButton(QWidget *parent = nullptr);

    virtual ~VolumeButton();

	void SetPlayer(std::shared_ptr<IAudioPlayer> player);

public slots:
	void OnVolumeChanged(uint32_t volume);

	void OnCurrentThemeChanged(ThemeColor theme_color);

private:
	void enterEvent(QEvent *event) override;

	void leaveEvent(QEvent* event) override;

	QScopedPointer<VolumeControlDialog> dialog_;
};
