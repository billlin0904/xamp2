//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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
			: playlist_dao(qGuiDb.getDatabase())
			, album_dao(qGuiDb.getDatabase())
			, artist_dao(qGuiDb.getDatabase())
			, music_dao(qGuiDb.getDatabase()) {
		}

		explicit DatabaseFacade(Database *database)
			: playlist_dao(database->database())
			, album_dao(database->database())
			, artist_dao(database->database())
			, music_dao(database->database()) {
		}

		PlaylistDao playlist_dao;
		AlbumDao album_dao;
		ArtistDao artist_dao;
		MusicDao music_dao;
	};

#define qDaoFacade SharedSingleton<dao::DatabaseFacade>::GetInstance()
}

