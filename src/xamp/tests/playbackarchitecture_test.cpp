#include "playbackqueue.h"
#include <QGuiApplication>
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* description) {
    if (!ok) throw std::runtime_error(description);
}
static PlayListEntity track(int id, int album = 1) {
    PlayListEntity value;
    value.playlist_music_id = id;
    value.music_id = id;
    value.album_id = album;
    value.file_path = QStringLiteral("/music/%1.flac").arg(id);
    value.title = QStringLiteral("Track %1").arg(id);
    return value;
}
int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    try {
        const auto a = track(1), b = track(2), c = track(3, 2);
        PlaybackQueue queue;
        require(!queue.next(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE, 1), "Empty queue must not invent a track");
        QList<PlayListEntity> visible{a,b,c};
        queue.reset(visible, b);
        visible.clear(); // Browsing/filtering another page must not replace the active queue.
        require(queue.current()->playlist_music_id == 2, "Playback cursor changed with browsing");
        require(queue.preview(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE)[0].playlist_music_id == 3, "Preview must match next track");
        require(queue.next(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE, -1)->playlist_music_id == 1, "Previous must go backwards");
        require(queue.next(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE, -1)->playlist_music_id == 3, "Previous must wrap correctly");
        require(queue.next(PlayerOrder::PLAYER_ORDER_REPEAT_ONE, 1)->playlist_music_id == 3, "Repeat one must retain current track");
        require(queue.select(2)->playlist_music_id == 2, "Queue click must address the captured queue");
        require(!queue.select(999), "Unknown queue item must be ignored");
        require(queue.current()->playlist_music_id == 2, "Invalid queue click must not move cursor");
        for (int i = 0; i < 100; ++i) {
            const auto next = queue.next(PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM, 1);
            require(next && next->playlist_music_id >= 1 && next->playlist_music_id <= 3, "Shuffle escaped source queue");
        }
        require(queue.preview(PlayerOrder::PLAYER_ORDER_SHUFFLE_ALBUM).isEmpty(), "Random next tracks must not be fabricated");
        queue.reset({}, a);
        require(queue.next(PlayerOrder::PLAYER_ORDER_REPEAT_ONCE, -1)->playlist_music_id == 1, "Single-track wrap failed");
        PlaybackSnapshot state;
        state.playlist_id = 7;
        state.track = a;
        require(!state.hasTrack() && !state.matches(7,1), "Stopped snapshot must not expose persisted playback flags");
        state.status = PlaybackStatus::Playing;
        require(state.matches(7,1) && !state.matches(8,1) && !state.matches(7,2), "Playback marker leaked to another list/item");
        state.status = PlaybackStatus::Paused;
        require(state.hasTrack() && state.matches(7,1), "Pause must retain track identity and artwork");
        state.status = PlaybackStatus::Stopped;
        require(!state.matches(7,1), "Stop must clear the marker");
        std::cout << "Playback queue and snapshot regression checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
