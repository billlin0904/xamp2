from __future__ import annotations

import sqlite3
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DB = ROOT / "src" / "xamp" / "xamp.db"


PROBES = [
    (
        "01 MusicDao::addOrUpdateMusic lookup",
        "path_index",
        """
        SELECT musicId
        FROM musics
        WHERE path = 'F:/XampPerfTest/Album 001/Track 0001.flac'
          AND offset = 0.0
          AND durationStr = '03:01'
        """,
    ),
    (
        "02 MusicDao::addOrUpdateMusic insert subquery",
        "path_index",
        """
        INSERT OR REPLACE INTO musics (
            musicId, title, track, path, fileExt, fileName, duration, durationStr,
            parentPath, bitRate, sampleRate, offset, dateTime, albumReplayGain,
            trackReplayGain, albumPeak, trackPeak, genre, comment, fileSize, heart,
            isCueFile, isZipFile, archiveEntryName
        )
        VALUES (
            (
                SELECT musicId
                FROM musics
                WHERE path = 'F:/XampPerfTest/Album 001/Track 0001.flac'
                  AND offset = 0.0
                  AND durationStr = '03:01'
            ),
            'Perf Test Track 0001', 1,
            'F:/XampPerfTest/Album 001/Track 0001.flac',
            '.flac', 'Track 0001', 181.0, '03:01',
            'F:/XampPerfTest/Album 001', 1411200, 44100, 0.0, 0,
            NULL, NULL, NULL, NULL, 'Perf', '', 50000001, 0, 0, 0, NULL
        )
        """,
    ),
    (
        "03 ArtistDao::getArtistId",
        "artist_name_index",
        """
        SELECT artistId
        FROM artists
        WHERE artist = 'Perf Test Artist'
        """,
    ),
    (
        "04 ArtistDao::addOrUpdateArtist subquery",
        "artist_name_index",
        """
        INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
        VALUES (
            (
                SELECT artistId
                FROM artists
                WHERE artist = 'Perf Test Artist'
            ),
            'Perf Test Artist',
            'P'
        )
        """,
    ),
    (
        "05 AlbumDao::getAlbumCoverId(album)",
        "album_store_index",
        """
        SELECT coverId
        FROM albums
        WHERE album = 'Perf Test Album 001'
        """,
    ),
    (
        "06 AlbumDao::addOrUpdateAlbum subquery",
        "album_store_index",
        """
        INSERT OR REPLACE INTO albums (
            albumId, album, artistId, coverId, storeType, dateTime, discId, year, isHiRes
        )
        VALUES (
            (
                SELECT albumId
                FROM albums
                WHERE album = 'Perf Test Album 001'
                  AND storeType = 1
            ),
            'Perf Test Album 001', 9000000, NULL, 1, 0, '', 2026, 0
        )
        """,
    ),
    (
        "07 AlbumDao::getAlbumIdFromAlbumMusic",
        "album_music_id_index",
        """
        SELECT albumId
        FROM albumMusic
        WHERE musicId = 9000001
        """,
    ),
    (
        "08 AlbumDao::addOrUpdateAlbumMusic subquery",
        "album_music_id_index",
        """
        INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId)
        VALUES (
            (
                SELECT albumMusicId
                FROM albumMusic
                WHERE musicId = 9000001
            ),
            9000001,
            9000000,
            9000001
        )
        """,
    ),
    (
        "09 PlaylistDao::addMusicToPlaylist duplicate guard",
        "playlist_music_unique_index",
        """
        INSERT OR IGNORE INTO playlistMusics (playlistMusicsId, playlistId, musicId, albumId)
        VALUES (NULL, 99999, 9000001, 9000001)
        """,
    ),
    (
        "10 AlbumDao::addOrUpdateAlbumArtist duplicate guard",
        "album_artist_unique_index",
        """
        INSERT OR IGNORE INTO albumArtist (albumArtistId, albumId, artistId)
        VALUES (NULL, 9000001, 9000000)
        """,
    ),
    (
        "11 AlbumDao::addAlbumCategory duplicate guard",
        "album_category_unique_index",
        """
        INSERT OR IGNORE INTO albumCategories (albumCategoryId, albumId, category)
        VALUES (NULL, 9000001, 'Local')
        """,
    ),
    (
        "12 Cover lookup by musicId",
        "INTEGER PRIMARY KEY",
        """
        SELECT coverId
        FROM musics
        WHERE musicId = 9000001
        """,
    ),
]


DUPLICATE_CHECKS = [
    (
        "duplicate playlistMusics groups",
        """
        SELECT COUNT(*)
        FROM (
            SELECT playlistId, musicId
            FROM playlistMusics
            GROUP BY playlistId, musicId
            HAVING COUNT(*) > 1
        )
        """,
    ),
    (
        "duplicate albumArtist groups",
        """
        SELECT COUNT(*)
        FROM (
            SELECT albumId, artistId
            FROM albumArtist
            GROUP BY albumId, artistId
            HAVING COUNT(*) > 1
        )
        """,
    ),
    (
        "duplicate albumCategories groups",
        """
        SELECT COUNT(*)
        FROM (
            SELECT albumId, category
            FROM albumCategories
            GROUP BY albumId, category
            HAVING COUNT(*) > 1
        )
        """,
    ),
]


def status_hint(detail: str, expected_index: str) -> str:
    if detail.startswith("SCALAR SUBQUERY"):
        return "INFO: subquery node"
    if expected_index == "INTEGER PRIMARY KEY":
        return "OK" if "USING INTEGER PRIMARY KEY" in detail else "CHECK: expected primary-key lookup"
    if expected_index in detail:
        return "OK"
    if expected_index == "album_music_id_index" and "sqlite_autoindex_albumMusic" in detail:
        return "OK"
    if expected_index in {
        "playlist_music_unique_index",
        "album_artist_unique_index",
        "album_category_unique_index",
    }:
        return "INFO: INSERT plan may not show unique-index conflict check"
    return f"CHECK: expected {expected_index}"


def index_exists(connection: sqlite3.Connection, table: str, index_name: str) -> bool:
    return any(row[1] == index_name for row in connection.execute(f"PRAGMA index_list('{table}')"))


def duplicate_guard_table(expected_index: str) -> str | None:
    return {
        "playlist_music_unique_index": "playlistMusics",
        "album_artist_unique_index": "albumArtist",
        "album_category_unique_index": "albumCategories",
    }.get(expected_index)


def print_table(rows: list[tuple[str, str, str, str]]) -> None:
    headers = ("probe", "detail", "expected_index", "status_hint")
    widths = [
        max(len(str(row[i])) for row in [headers, *rows])
        for i in range(len(headers))
    ]
    print(" | ".join(header.ljust(widths[i]) for i, header in enumerate(headers)))
    print("-+-".join("-" * width for width in widths))
    for row in rows:
        print(" | ".join(str(value).ljust(widths[i]) for i, value in enumerate(row)))


def main() -> int:
    db_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_DB
    if not db_path.exists():
        print(f"Database not found: {db_path}", file=sys.stderr)
        return 1

    rows: list[tuple[str, str, str, str]] = []
    with sqlite3.connect(db_path) as connection:
        for probe, expected_index, sql in PROBES:
            plan_rows = connection.execute(f"EXPLAIN QUERY PLAN {sql}").fetchall()
            if not plan_rows:
                table = duplicate_guard_table(expected_index)
                if table is not None and index_exists(connection, table, expected_index):
                    rows.append((probe, "INSERT VALUES has no query-plan rows", expected_index, "OK: unique index exists"))
                else:
                    rows.append((probe, "INSERT VALUES has no query-plan rows", expected_index, "INFO"))
                continue
            for _, _, _, detail in plan_rows:
                rows.append((probe, detail, expected_index, status_hint(detail, expected_index)))

        for probe, sql in DUPLICATE_CHECKS:
            count = connection.execute(sql).fetchone()[0]
            status = "OK" if count == 0 else "CHECK: duplicate rows remain"
            rows.append((probe, str(count), "0", status))

    print_table(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
