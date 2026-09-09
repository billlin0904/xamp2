#include "playbackpresenter.h"
#include "playbackcontroller.h"
#include <ui_xamp.h>
#include <thememanager.h>
#include <widget/lrcpage.h>
#include <widget/lyricsshowwidget.h>
#include <widget/richplaylistpage.h>
#include <widget/filesystemviewpage.h>
#include <widget/cdpage.h>
#include <widget/playlistpage.h>
#include <widget/playlisttableview.h>
#include <widget/chatgpt/spectrogramwidget.h>
#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>
#include <cmath>
#include <QTimer>

PlaybackPresenter::PlaybackPresenter(PlaybackController& playback, Ui::XampWindow& ui, IXMainWindow& window,
    LrcPage& lyrics, RichPlaylistPage& playlist, FileSystemViewPage& library, CdPage& cd,
    std::function<void(const PlayListEntity&)> request_lyrics, QObject* parent)
    : QObject(parent), playback_(playback), ui_(ui), window_(window), lyrics_(lyrics), playlist_(playlist),
      library_(library), cd_(cd), request_lyrics_(std::move(request_lyrics)) {
    connect(&playback_, &PlaybackController::changed, this, [this](const PlaybackSnapshot& state) { render(state); });
    connect(&playback_, &PlaybackController::positionChanged, this, [this](double position) { renderPosition(position); });
    // The native taskbar is available only after the host window has been attached/shown.
    QTimer::singleShot(0, this, [this] { render(playback_.snapshot()); });
}

void PlaybackPresenter::render(const PlaybackSnapshot& state) {
    playlist_.applyPlayback(state);
    library_.playlistPage()->playlist()->reload(true);
    cd_.playlistPage()->playlist()->reload(true);
    qTheme.setPlayOrPauseButton(ui_.playButton, state.status == PlaybackStatus::Playing);
    ui_.seekSlider->setSeekEnabled(state.hasTrack());
    const bool volume_enabled = !state.hasTrack() || !state.bitperfect;
    ui_.volumeSlider->setEnabled(volume_enabled);
    ui_.mutedButton->setEnabled(volume_enabled);
    ui_.volumeSlider->setToolTip(volume_enabled ? QString() : tr("BitPerfect: use the DAC volume control"));
    const auto cover = state.hasTrack() && !state.cover.isNull() ? state.cover : qTheme.unknownCover();
    ui_.coverLabel->setPixmap(image_util::roundCoverImage(cover, ui_.coverLabel->size(), image_util::kPlaylistImageRadius));
    window_.setIconicThumbnail(cover);
    lyrics_.setCover(cover);
    ui_.titleLabel->setText(state.hasTrack() ? state.track.title : QString());
    ui_.artistLabel->setText(state.hasTrack() ? state.track.artist : QString());
    lyrics_.format()->setText(state.format + (state.bitperfect ? QStringLiteral(" | BitPerfect PCM") : QString()));
    if (!state.hasTrack()) {
        lyrics_.setPlayListEntity({});
        lyrics_.lyrics()->stop();
        lyrics_.clearBackground();
        ui_.seekSlider->clearWaveform();
        ui_.seekSlider->setRange(0, 0);
        ui_.seekSlider->setValue(0);
        ui_.startPosLabel->setText(formatDuration(0));
        ui_.endPosLabel->setText(formatDuration(0));
        window_.resetTaskbarProgress();
    } else {
        if (!had_track_ || revision_ != state.track_revision) {
            lyrics_.setPlayListEntity(state.track);
            lyrics_.disableLoadLrcButton();
            if (!lyrics_.lyrics()->loadFile(state.track.file_path)) request_lyrics_(state.track);
            lyrics_.clearBackground();
            ui_.seekSlider->setRange(0, qRound64(state.track.duration * 1000.0));
            ui_.seekSlider->loadFile(state.track.file_path);
        }
        renderPosition(state.position);
        if (state.status == PlaybackStatus::Paused) window_.setTaskbarPlayerPaused();
        else window_.setTaskbarPlayingResume();
    }
    revision_ = state.track_revision;
    had_track_ = state.hasTrack();
}

void PlaybackPresenter::renderPosition(double position) {
    const auto& state = playback_.snapshot();
    if (!state.hasTrack()) return;
    const auto duration = state.track.duration;
    const auto full_text = isMoreThan1Hours(duration);
    ui_.startPosLabel->setText(formatDuration(position, full_text));
    ui_.endPosLabel->setText(formatDuration(qMax(0.0, duration - position), full_text));
    ui_.seekSlider->setValue(qRound64(position * 1000.0));
    if (duration > 0) window_.setTaskbarProgress(static_cast<int>(100.0 * position / duration));
    lyrics_.lyrics()->setLrcTime(qRound64(position * 1000.0));
    if (state.source == PlaybackSource::Library)
        library_.spectrogramWidget()->setCurrentPosition(static_cast<float>(position));
}
