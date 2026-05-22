-- SQLite cannot combine EXPLAIN QUERY PLAN rows into one SELECT in plain SQL.
-- Use scan_insert_explain.py for a single combined result table:
--
--   python tools/sql/scan_insert_explain.py
--
-- Or pass a custom database path:
--
--   python tools/sql/scan_insert_explain.py F:/Source/xamp2/src/xamp/xamp.db

EXPLAIN QUERY PLAN
SELECT musicId
FROM musics
WHERE path = 'F:/XampPerfTest/Album 001/Track 0001.flac'
  AND offset = 0.0
  AND durationStr = '03:01';

EXPLAIN QUERY PLAN
SELECT artistId
FROM artists
WHERE artist = 'Perf Test Artist';

EXPLAIN QUERY PLAN
SELECT coverId
FROM albums
WHERE album = 'Perf Test Album 001';

EXPLAIN QUERY PLAN
SELECT albumId
FROM albums
WHERE album = 'Perf Test Album 001'
  AND storeType = 1;

EXPLAIN QUERY PLAN
SELECT albumId
FROM albumMusic
WHERE musicId = 9000001;

EXPLAIN QUERY PLAN
SELECT coverId
FROM musics
WHERE musicId = 9000001;
