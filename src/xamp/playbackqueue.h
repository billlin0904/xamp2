#pragma once
#include <widget/playbacksnapshot.h>
#include <widget/playerorder.h>
#include <QRandomGenerator>
#include <optional>

// A playback request captures its ordered queue, independent of later UI navigation/filtering.
class PlaybackQueue {
public:
    void reset(QList<PlayListEntity> tracks, const PlayListEntity& current) {
        tracks_ = std::move(tracks);
        cursor_ = -1;
        for (int i = 0; i < tracks_.size(); ++i)
            if (sameTrack(tracks_[i], current)) { cursor_ = i; break; }
        if (cursor_ < 0) { tracks_.append(current); cursor_ = tracks_.size() - 1; }
    }
    std::optional<PlayListEntity> current() const {
        if (cursor_ < 0 || cursor_ >= tracks_.size()) return std::nullopt;
        return tracks_[cursor_];
    }
    const QList<PlayListEntity>& tracks() const { return tracks_; }
    bool empty() const { return tracks_.isEmpty(); }
    std::optional<PlayListEntity> next(PlayerOrder order, int direction) {
        if (tracks_.isEmpty()) return std::nullopt;
        if (order == PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM) {
            QList<int> albums;
            for (const auto& track : tracks_) if (!albums.contains(track.album_id)) albums.append(track.album_id);
            const int album = albums[QRandomGenerator::global()->bounded(static_cast<int>(albums.size()))];
            QList<int> candidates;
            for (int i = 0; i < tracks_.size(); ++i) if (tracks_[i].album_id == album) candidates.append(i);
            cursor_ = candidates[QRandomGenerator::global()->bounded(static_cast<int>(candidates.size()))];
        } else if (order != PlayerOrder::PLAYER_ORDER_REPEAT_ONE) {
            cursor_ = (cursor_ + direction % tracks_.size() + tracks_.size()) % tracks_.size();
        }
        return tracks_[cursor_];
    }
    std::optional<PlayListEntity> select(int id) {
        for (int i = 0; i < tracks_.size(); ++i) if (tracks_[i].playlist_music_id == id) {
            cursor_ = i; return tracks_[i];
        }
        return std::nullopt;
    }
    QList<PlayListEntity> preview(PlayerOrder order) const {
        QList<PlayListEntity> result;
        if (tracks_.isEmpty() || order == PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM) return result;
        if (order == PlayerOrder::PLAYER_ORDER_REPEAT_ONE) { result.append(tracks_[cursor_]); return result; }
        for (int i = 1; i <= qMin(3, static_cast<int>(tracks_.size()) - 1); ++i)
            result.append(tracks_[(cursor_ + i) % tracks_.size()]);
        return result;
    }
    static bool sameTrack(const PlayListEntity& a, const PlayListEntity& b) {
        return a.playlist_music_id == b.playlist_music_id && a.file_path == b.file_path;
    }
private:
    QList<PlayListEntity> tracks_;
    int cursor_{-1};
};
