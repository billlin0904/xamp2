# macOS global shortcut regression

Carbon keycode 0 is the physical A key (ㄇ in the standard Zhuyin layout).
The old conversion used 0 for unsupported media keys and failed layout lookups,
so registering startup playback shortcuts intercepted A system-wide. Truncating
Qt special-key enums into UTF-16 also incorrectly mapped unsupported F24 to G.

The fix uses a distinct invalid sentinel, rejects unsupported special keys before
character lookup, and prevents invalid keys from reaching native registration.
The underlying keyboard layout is used for input methods such as Zhuyin; its
input-source reference stays alive while the borrowed layout data is read.

```sh
cmake --preset macos-xcode -DXAMP_BUILD_SHORTCUT_TESTS=ON
cmake --build out/build/macos-xcode --config Debug --target xamp globalshortcut_test
ctest --test-dir out/build/macos-xcode -C Debug -R '^globalshortcut_test$' --output-on-failure
```

The tests verify unsupported media keys, valid A/function/navigation keys and
rejection of the invalid sentinel. They do not register any valid global hotkey
or change the system input source.
