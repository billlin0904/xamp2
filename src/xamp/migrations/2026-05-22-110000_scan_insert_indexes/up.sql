DELETE FROM playlistMusics
WHERE playlistMusicsId NOT IN (
    SELECT MIN(playlistMusicsId)
    FROM playlistMusics
    GROUP BY playlistId, musicId
);

DELETE FROM albumArtist
WHERE albumArtistId NOT IN (
    SELECT MIN(albumArtistId)
    FROM albumArtist
    GROUP BY albumId, artistId
);

DELETE FROM albumCategories
WHERE albumCategoryId NOT IN (
    SELECT MIN(albumCategoryId)
    FROM albumCategories
    GROUP BY albumId, category
);

CREATE UNIQUE INDEX IF NOT EXISTS playlist_music_unique_index ON playlistMusics (playlistId, musicId);
CREATE UNIQUE INDEX IF NOT EXISTS album_artist_unique_index ON albumArtist (albumId, artistId);
CREATE UNIQUE INDEX IF NOT EXISTS album_category_unique_index ON albumCategories (albumId, category);
CREATE INDEX IF NOT EXISTS album_music_id_index ON albumMusic (musicId);
CREATE INDEX IF NOT EXISTS album_store_index ON albums (album, storeType);
