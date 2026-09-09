#pragma once
#include <QObject>
#include <functional>
#include <widget/playbacksnapshot.h>
namespace Ui { class XampWindow; }
class PlaybackController;
class IXMainWindow;
class LrcPage;
class RichPlaylistPage;
class FileSystemViewPage;
class CdPage;

// Owns rendering/subscriptions only; it never opens, pauses, or chooses tracks.
class PlaybackPresenter final : public QObject {
public:
    PlaybackPresenter(PlaybackController& playback, Ui::XampWindow& ui, IXMainWindow& window,
        LrcPage& lyrics, RichPlaylistPage& playlist, FileSystemViewPage& library, CdPage& cd,
        std::function<void(const PlayListEntity&)> request_lyrics, QObject* parent);
private:
    void render(const PlaybackSnapshot& state);
    void renderPosition(double position);
    PlaybackController& playback_;
    Ui::XampWindow& ui_;
    IXMainWindow& window_;
    LrcPage& lyrics_;
    RichPlaylistPage& playlist_;
    FileSystemViewPage& library_;
    CdPage& cd_;
    std::function<void(const PlayListEntity&)> request_lyrics_;
    quint64 revision_{0};
    bool had_track_{false};
};
