CREATE TABLE IF NOT EXISTS playlistAlbumStates (
    playlistId integer NOT NULL,
    albumId integer NOT NULL,
    isCollapsed integer NOT NULL DEFAULT 0,
    PRIMARY KEY (playlistId, albumId)
);
