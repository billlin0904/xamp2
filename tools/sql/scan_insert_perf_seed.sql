-- XAMP scan-to-database insert performance seed.
-- Run this after database migrations.
-- This script is repeatable: it removes the previous Perf Test rows before seeding again.
-- The expected final counts are listed at the bottom.

DROP TABLE IF EXISTS temp.seq;
CREATE TEMP TABLE seq (n INTEGER PRIMARY KEY);

INSERT INTO seq (n)
SELECT ones.n + tens.n * 10 + hundreds.n * 100 + thousands.n * 1000 + 1
FROM (
    SELECT 0 AS n UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4
    UNION ALL SELECT 5 UNION ALL SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL SELECT 9
) AS ones
CROSS JOIN (
    SELECT 0 AS n UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4
    UNION ALL SELECT 5 UNION ALL SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL SELECT 9
) AS tens
CROSS JOIN (
    SELECT 0 AS n UNION ALL SELECT 1 UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4
    UNION ALL SELECT 5 UNION ALL SELECT 6 UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL SELECT 9
) AS hundreds
CROSS JOIN (
    SELECT 0 AS n UNION ALL SELECT 1
) AS thousands
WHERE ones.n + tens.n * 10 + hundreds.n * 100 + thousands.n * 1000 < 2000;

BEGIN TRANSACTION;

DELETE FROM playlistMusics
WHERE playlistId = 99999
   OR musicId BETWEEN 9000001 AND 9002000
   OR albumId BETWEEN 9000001 AND 9000200;

DELETE FROM albumMusic
WHERE musicId BETWEEN 9000001 AND 9002000
   OR albumId BETWEEN 9000001 AND 9000200;

DELETE FROM musicLoudness
WHERE musicId BETWEEN 9000001 AND 9002000
   OR albumId BETWEEN 9000001 AND 9000200;

DELETE FROM albumArtist
WHERE albumId BETWEEN 9000001 AND 9000200
   OR artistId = 9000000;

DELETE FROM albumCategories
WHERE albumId BETWEEN 9000001 AND 9000200;

DELETE FROM musics
WHERE musicId BETWEEN 9000001 AND 9002000
   OR path LIKE 'F:/XampPerfTest/%';

DELETE FROM albums
WHERE albumId BETWEEN 9000001 AND 9000200
   OR album LIKE 'Perf Test Album %';

DELETE FROM artists
WHERE artistId = 9000000
   OR artist = 'Perf Test Artist';

DELETE FROM playlist
WHERE playlistId = 99999
   OR name = 'Perf Test Playlist';

INSERT OR REPLACE INTO playlist (playlistId, playlistIndex, storeType, cloudPlaylistId, name)
VALUES (99999, 99999, 1, '', 'Perf Test Playlist');

INSERT OR REPLACE INTO artists (artistId, artist, artistNameEn, firstChar, firstCharEn)
VALUES (9000000, 'Perf Test Artist', '', 'P', '');

INSERT OR REPLACE INTO albums (
    albumId,
    artistId,
    album,
    coverId,
    discId,
    dateTime,
    year,
    heart,
    storeType,
    isHiRes,
    isSelected,
    plays
)
SELECT
    9000000 + n,
    9000000,
    'Perf Test Album ' || printf('%03d', n),
    NULL,
    '',
    0,
    2026,
    0,
    1,
    0,
    0,
    0
FROM seq
WHERE n <= 200;

INSERT OR REPLACE INTO musics (
    musicId,
    track,
    title,
    path,
    parentPath,
    offset,
    duration,
    durationStr,
    fileName,
    fileExt,
    bitRate,
    sampleRate,
    rating,
    dateTime,
    albumReplayGain,
    albumPeak,
    trackReplayGain,
    trackPeak,
    genre,
    comment,
    fileSize,
    heart,
    plays,
    lyrc,
    trLyrc,
    coverId,
    isCueFile,
    isZipFile,
    archiveEntryName
)
SELECT
    9000000 + n,
    ((n - 1) % 10) + 1,
    'Perf Test Track ' || printf('%04d', n),
    'F:/XampPerfTest/Album ' || printf('%03d', CAST(((n - 1) / 10) + 1 AS INTEGER)) || '/Track ' || printf('%04d', n) || '.flac',
    'F:/XampPerfTest/Album ' || printf('%03d', CAST(((n - 1) / 10) + 1 AS INTEGER)),
    0.0,
    180.0 + (n % 60),
    printf('%02d:%02d', CAST((180 + (n % 60)) / 60 AS INTEGER), (180 + (n % 60)) % 60),
    'Track ' || printf('%04d', n),
    '.flac',
    1411200,
    44100,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    'Perf',
    '',
    50000000 + n,
    0,
    0,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL
FROM seq;

INSERT OR REPLACE INTO albumMusic (albumMusicId, musicId, artistId, albumId)
SELECT
    (SELECT albumMusicId FROM albumMusic WHERE musicId = 9000000 + n),
    9000000 + n,
    9000000,
    9000000 + CAST(((n - 1) / 10) + 1 AS INTEGER)
FROM seq;

INSERT OR IGNORE INTO playlistMusics (playlistMusicsId, playlistId, musicId, albumId, playing, isChecked)
SELECT
    NULL,
    99999,
    9000000 + n,
    9000000 + CAST(((n - 1) / 10) + 1 AS INTEGER),
    0,
    0
FROM seq;

INSERT OR IGNORE INTO albumArtist (albumArtistId, albumId, artistId)
SELECT
    NULL,
    9000000 + n,
    9000000
FROM seq
WHERE n <= 200;

INSERT OR IGNORE INTO albumCategories (albumCategoryId, category, albumId)
SELECT NULL, 'Local', 9000000 + n FROM seq WHERE n <= 200;

INSERT OR IGNORE INTO albumCategories (albumCategoryId, category, albumId)
SELECT NULL, 'Perf', 9000000 + n FROM seq WHERE n <= 200;

-- Duplicate pass. With the new unique indexes, these should not increase relation table counts.
INSERT OR IGNORE INTO playlistMusics (playlistMusicsId, playlistId, musicId, albumId, playing, isChecked)
SELECT
    NULL,
    99999,
    9000000 + n,
    9000000 + CAST(((n - 1) / 10) + 1 AS INTEGER),
    0,
    0
FROM seq;

INSERT OR IGNORE INTO albumArtist (albumArtistId, albumId, artistId)
SELECT
    NULL,
    9000000 + n,
    9000000
FROM seq
WHERE n <= 200;

INSERT OR IGNORE INTO albumCategories (albumCategoryId, category, albumId)
SELECT NULL, 'Local', 9000000 + n FROM seq WHERE n <= 200;

COMMIT;

SELECT 'musics' AS table_name, COUNT(*) AS rows, 'expected 2000' AS expected
FROM musics
WHERE musicId BETWEEN 9000001 AND 9002000
UNION ALL
SELECT 'playlistMusics', COUNT(*), 'expected 2000'
FROM playlistMusics
WHERE playlistId = 99999
UNION ALL
SELECT 'albumMusic', COUNT(*), 'expected 2000'
FROM albumMusic
WHERE musicId BETWEEN 9000001 AND 9002000
UNION ALL
SELECT 'albumArtist', COUNT(*), 'expected 200'
FROM albumArtist
WHERE albumId BETWEEN 9000001 AND 9000200
UNION ALL
SELECT 'albumCategories', COUNT(*), 'expected 400'
FROM albumCategories
WHERE albumId BETWEEN 9000001 AND 9000200;
