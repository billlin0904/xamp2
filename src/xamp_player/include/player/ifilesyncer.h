//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/memory.h>

#include <player/player.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

class XAMP_PLAYER_API IFileSyncer {
public:
	XAMP_BASE_CLASS(IFileSyncer)
	/*
	* Add music to library.
	*
	* @param[in] file_path The file path.
	*/
	virtual void AddMusicToLibrary(const std::wstring& file_path) = 0;

	/*
	 * Sync to device.
	 */
	virtual void SyncToDevice() = 0;
protected:
	IFileSyncer() = default;
};

class XAMP_PLAYER_API ITunesFileSyncer : public IFileSyncer {
public:
	ITunesFileSyncer();

	XAMP_PIMPL(ITunesFileSyncer)

	void AddMusicToLibrary(const std::wstring& file_path) override;

	void SyncToDevice() override;
private:
	class ITunesFileSyncerImpl;
	ScopedPtr<ITunesFileSyncerImpl> impl_;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END

