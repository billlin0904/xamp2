# Playback ownership

`PlaybackController` is the single writer of the current track, artwork, transport status,
position, source playlist, and queue. `PlaybackSnapshot` is a value projection for views.
Selection, filtering, and persisted SQL flags do not establish live playback.

## Components

- `PlaybackController`: opens audio, applies the existing DSP/device plan, handles transport
  commands and backend callbacks, and publishes snapshots only after a successful open.
- `PlaybackQueue`: captures the ordered source list when playback starts. Browsing another
  list does not change transport navigation or the upcoming-tracks preview.
- `PlaybackPresenter`: subscribes to snapshots and renders the footer, lyrics, main artwork,
  now-playing panel, taskbar, and legacy list projections. Initial rendering is deferred until
  the native host window is ready.
- `ApplicationServices`: owns the filesystem, cover and background workers and their threads;
  cancellation and teardown live beside construction.
- `ApplicationUpdater`: owns update checking, download verification and platform installation.
- `Xamp`: composes these components and connects view commands/navigation.

## State rules

- Pausing retains the track and artwork. Stopping clears visible track data and flags while
  retaining the captured queue for a subsequent play command.
- Queue item IDs are scoped to their source playlist; a different visible list cannot claim
  the current marker or replace the current favorite target.
- SQL `playing` fields remain a compatibility projection for legacy table models. They are
  cleared on startup and are not restored as live playback.
- Backend state/time/error callbacks are stamped when emitted. Events queued before a new
  open or seek are rejected by generation. Blocking stop waits for the previous stream before
  accepting callbacks for the next track.
- Matching album cover completions update the controller, which refreshes every artwork view.
- Metadata-read failures preserve existing playback. Failures after stopping the old stream
  clear the state and release partially opened output.
- Local file drops have an external source and stop at EOF. Playlist/library/CD queues retain
  the existing repeat/shuffle modes. Captured queues are not live views of later list edits.

## Regression checks

`tests/playbackarchitecture_test.cpp` checks captured queue independence, preview/navigation
agreement, previous/wrap/repeat/shuffle, invalid queue requests, and stopped/paused marker identity.
CMake builds can enable `XAMP_BUILD_ARCHITECTURE_TESTS` and run `ctest -R playbackarchitecture`.
The test is also a standalone Qt GUI executable (run with `-platform offscreen`).

Manual smoke checks: startup with no false playing marker; play/pause; previous/next;
seek to EOF; browse a different playlist while playing; start from Library and inspect
footer/hero/now-playing/lyrics artwork; stop and device removal; application shutdown.

Metadata preparation is still synchronous and database access still uses the existing
facades. Those are separate follow-up concerns, not changes to the audio engine in this refactor.
