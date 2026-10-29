# Vinyl mode checks

The lyrics page has a checkable record icon button for **Vinyl mode** (黑膠模式).
It uses the existing Material icon family and the same themed button states as shuffle,
with an accessible name and a tooltip. It shows only a centered
turntable, hiding the lyrics and format information, and remembers the choice in
`lyricsVinylMode`. Switching it off restores the cover and lyrics without clearing
the lyrics. Playing rotates the album label and record at the standard LP speed of 33⅓ RPM
(200 degrees per second, one revolution every 1.8 seconds).
The turntable scales with the page, up to 800 logical pixels, using the
full available area with tighter margins.
Playing moves the tonearm from its fixed rest onto the record in about 0.9 seconds.
Pausing/stopping smoothly returns it to the rest; playback progress moves the stylus inward.
A transport reset during stopping does not make the arm jump before returning.
Switching back to the cover or leaving the page stops the animation timer.

The widget is painted in Qt, with no external images, GPU backend, or platform
specific animation dependency. Record grooves and album art are cached at device
resolution; the platter, lighting and tonearm stay independent of record rotation.

Run the lightweight widget tests independently:

```sh
cmake -S tests/vinyl -B out/build/vinyl-tests -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build out/build/vinyl-tests
ctest --test-dir out/build/vinyl-tests --output-on-failure
```

On macOS, enable the integration tests for the actual lyrics page as well:

```sh
cmake --preset macos-xcode -DXAMP_BUILD_VINYL_TESTS=ON
cmake --build out/build/macos-xcode --config Debug --target vinyl_widget_test vinyl_page_test
ctest --test-dir out/build/macos-xcode -C Debug -R 'vinyl_.*_test' --output-on-failure
```

The tests use Qt's offscreen backend and an isolated temporary settings file.
They check playback/pause, hidden-page timer suspension, cover changes, finite
progress handling, mouse/keyboard switching, preservation of lyrics and saved mode.
Set `XAMP_VINYL_PREVIEW_DIR` to an output directory to also save dark/light PNGs.
For translated integration previews, set `XAMP_VINYL_TEST_CATALOGS` to the built
app's `Contents/Resources/langs` directory.
