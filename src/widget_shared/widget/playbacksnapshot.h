#pragma once

#include <QPixmap>
#include <QList>
#include <widget/playlistentity.h>

enum class PlaybackStatus { Stopped, Playing, Paused };
enum class PlaybackSource { Playlist, Library, Cd, External };

// Value projection of PlaybackController state. Views never infer playback from selection or SQL.
struct PlaybackSnapshot {
    quint64 track_revision{0};
    PlaybackStatus status{PlaybackStatus::Stopped};
    PlaybackSource source{PlaybackSource::External};
    int playlist_id{-1};
    PlayListEntity track;
    QPixmap cover;
    QString format;
    double position{0};
    QList<PlayListEntity> upcoming;
    bool shuffle{false};
    bool bitperfect{false};

    bool hasTrack() const { return status != PlaybackStatus::Stopped && !track.file_path.isEmpty(); }
    bool matches(int playlist, int item) const {
        return hasTrack() && playlist == playlist_id && item == track.playlist_music_id;
    }
};
