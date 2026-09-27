#pragma once
#include <QObject>
#include <atomic>
#include <memory>
#include <optional>
#include <exception>
#include <widget/uiplayerstateadapter.h>
#include <widget/playbacksnapshot.h>
#include <xampplayer.h>
#include "playbackqueue.h"

class PlaybackController final : public QObject {
    Q_OBJECT
public:
    PlaybackController(std::shared_ptr<IAudioPlayer> player, QObject* parent = nullptr);
    const PlaybackSnapshot& snapshot() const { return snapshot_; }
    const std::shared_ptr<UIPlayerStateAdapter>& adapter() const { return adapter_; }
    void setDevice(const DeviceInfo& device) { device_ = device; }
    void play(const PlayListEntity& track, int playlist, PlaybackSource source, const QList<PlayListEntity>& queue);
    void next(int direction);
    void playQueued(int id);
    void togglePause();
    void seek(double seconds);
    void stop(bool shutdown_device = false);
    void refreshOrder();
    void updateAlbumCover(int album_id, const QPixmap& cover);
    void setFavorite(bool favorite);
    void shutdown();
    void discardPlaylist(int playlist_id);
    void saveUpdateSession();
    void restoreUpdateSession();
signals:
    void changed(const PlaybackSnapshot& state);
    void positionChanged(double seconds);
    void failed(std::exception_ptr error);
    void startRequested();
private:
    bool open(const PlayListEntity& track, double position = 0);
    bool restored_{false};
    void publish();
    void acceptState(PlayerState state);
    void clear();
    std::shared_ptr<IAudioPlayer> player_;
    std::shared_ptr<UIPlayerStateAdapter> adapter_;
    std::optional<DeviceInfo> device_;
    PlaybackSnapshot snapshot_;
    PlaybackQueue queue_;
    std::atomic<quint64> generation_{0};
    std::atomic_bool accepting_events_{false};
    bool shutting_down_{false};
};
