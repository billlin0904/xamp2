//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/dao/albumdao.h>
#include <widget/dao/playlistdao.h>
#include <widget/dao/artistdao.h>
#include <widget/dao/musicdao.h>

namespace dao {
	class XAMP_WIDGET_SHARED_EXPORT DatabaseFacade final {
	public:
		DatabaseFacade()
			: playlist_dao_(qGuiDb.getDatabase())
			, album_dao_(qGuiDb.getDatabase())
			, artist_dao_(qGuiDb.getDatabase())
			, music_dao_(qGuiDb.getDatabase()) {
		}

		PlaylistDao playlist_dao_;
		AlbumDao album_dao_;
		ArtistDao artist_dao_;
		MusicDao music_dao_;
	};

#define qDaoFacade SharedSingleton<dao::DatabaseFacade>::GetInstance()
}

