# macOS / Apple Silicon 開發

需要完整 Xcode（位於 `/Applications/Xcode.app`）及 Apple Silicon Homebrew。

```sh
./tools/setup_macos.sh
```

腳本安裝 Qt 6、FFmpeg 6 等依賴，下載固定版本的來源依賴與 BASS macOS 音效元件，並產生與編譯 Xcode 專案。BASS 元件的使用與散布須遵循原廠授權。

開啟 `out/build/macos-xcode/xamp.xcodeproj`，選擇 `xamp` scheme、My Mac，按 Run。

後續重新產生專案與編譯：

```sh
cmake --preset macos-xcode
cmake --build --preset macos-xcode --parallel 4
```

Debug 程式位於 `out/build/macos-xcode/src/xamp/Debug/xamp.app`。目前為本機開發建置，依賴 Homebrew 及建置目錄內的動態函式庫；尚未封裝成可在其他電腦直接使用的發行版本。

macOS 使用 CoreAudio 輸出與 Accelerate FFT；DSD 轉換使用可攜式實作，沒有使用 Intel MKL。

macOS 的設定、資料庫與日誌位於 `~/Library/Application Support/xamp`，快取位於系統使用者快取目錄。
