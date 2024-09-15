//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>

class MusicPlayerModel : public QObject {
	Q_OBJECT
public:
	MusicPlayerModel(const std::shared_ptr<IAudioPlayer> &player, QObject* parent = nullptr);

	~MusicPlayerModel();

	void play(const QString& path);

	void stop();

	const AlignPtr<IAudioDeviceManager>& GetAudioDeviceManager();

	void seek(double seconds);

	DsdModes dsdMode() const;

	uint32_t volume() const;

	PlayerState state() const;

	void pause();

	void resume();
private:
	std::shared_ptr<IAudioPlayer> player_;
};