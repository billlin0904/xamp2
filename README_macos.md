# macOS / Apple Silicon 開發

需要完整 Xcode（位於 `/Applications/Xcode.app`）及 Apple Silicon Homebrew。

```sh
./tools/setup_macos.sh
```

腳本安裝 Qt 6、FFmpeg 6 等依賴，下載固定版本的來源依賴與 BASS macOS 音效元件，並產生與編譯 Xcode 專案。BASS 元件的使用與散布須遵循原廠授權。

直接雙擊根目錄的 `xamp.xcworkspace`，選擇 `xamp` scheme、My Mac，按 Run。也可開啟 `out/build/macos-xcode/xamp.xcodeproj`。

Workspace 參照 CMake 產生的專案；首次下載此儲存庫時，請先執行上面的設定腳本。已安裝依賴時，只需執行 `cmake --preset macos-xcode` 即可重新產生專案。

後續重新產生專案與編譯：

```sh
cmake --preset macos-xcode
cmake --build --preset macos-xcode --parallel 4
```

Debug 程式位於 `out/build/macos-xcode/src/xamp/Debug/xamp.app`。此為本機開發建置，依賴 Homebrew 及建置目錄內的動態函式庫。對外發行請使用下方 DMG 封裝腳本。

macOS 使用 CoreAudio 輸出與 Accelerate FFT；PCM2DSD 也使用 Accelerate 的雙精度複數 FFT，沒有使用 Intel MKL。

Apple Silicon 的 PCM2DSD 預設啟用 ARM64 NEON 雙精度向量運算，同時計算左右聲道的 15 階調變器及複數乘法；FFT 使用 `vDSP_fft_zipD`，每級重用 FFTSetupD 與 split-complex 工作空間，反向轉換正規化為 1/N。`PCM_DSD_USE_ACCELERATE=OFF` 可切回 portable FFT。`PCM_DSD_USE_NEON` 與 x86 的 `PCM_DSD_USE_SIMD` 是獨立選項。可用 `cmake --preset macos-xcode -DPCM_DSD_USE_NEON=OFF` 暫時回到純量版本，再建置；不帶此覆寫執行 preset 會重新啟用 NEON。

先前僅啟用 NEON（portable FFT）的測量：本機以 Release、96 kHz 雙聲道 → DSD64、預設 FIR／15 階／dither 測量，每次轉換 20 秒音訊，純量與 NEON 交替各測三次：即時速度中位數為 3.49×／3.77×，NEON 約快 8%。六次輸出的 SHA-256 相同。這是轉換核心測量，不含檔案解碼與播放裝置負載。

加入 Accelerate FFT 後，以相同條件與 NEON portable FFT 交替各測三次，中位數由 3.77× 提升到 5.45×（約快 45%）。兩者保留 NEON 調變器；FFT 數值／FIR 卷積與串流測試均通過，但不同 FFT 後端的浮點舍入不同，因此 DSD 輸出不保證逐 bit 相同。

## VS Code 開發

用 VS Code 開啟儲存庫根目錄 `xamp2`，並安裝工作區建議的 C/C++、CMake Tools、CodeLLDB 擴充套件。首次開啟若要求選擇 Configure Preset，請選 `macos-xcode`；也可由命令面板執行 `CMake: Select Configure Preset`。CMake Tools 會提供 Qt 與各模組的編譯參數給 C/C++ 程式碼提示。

- `Cmd+Shift+B`：重新產生專案並編譯 Debug。
- `F5`：選擇 `xamp Debug (macOS)`，編譯後啟動 LLDB 除錯，可在 `.cpp` 設中斷點。
- `Tasks: Run Task` → `xamp: run (macOS)`：編譯並啟動應用程式。

Release 模式：

- 命令面板 `Tasks: Run Build Task` → `xamp: build Release (macOS)`：編譯 Release。
- `Tasks: Run Task` → `xamp: run Release (macOS)`：編譯並啟動 Release。
- 「執行與偵錯」下拉選單選 `xamp Release (macOS)`，按 F5：編譯並以 LLDB 啟動 Release。最佳化後部分變數或中斷點可能無法如 Debug 般使用。

`Cmd+Shift+B` 預設仍建置 Debug。終端機也可以執行：

```sh
cmake --preset macos-xcode
cmake --build --preset macos-xcode-release --parallel 4
open out/build/macos-xcode/src/xamp/Release/xamp.app
```

Debug 和 Release 使用同一個 Xcode 專案，產物分別位於各自的組態目錄。切換前請先結束正在執行的 XAMP。

VS Code 與 Xcode 共用 `out/build/macos-xcode`，請避免同時在兩個 IDE 編譯。左側預設隱藏 `out` 建置產物；需要查看時可在 `.vscode/settings.json` 的 `files.exclude` 調整。首次準備依賴仍需執行前述 `tools/setup_macos.sh`。

macOS Debug 建置預設只對 PCM2DSD 轉換核心啟用 `-O3`，主程式仍保留一般 Debug 除錯設定。未最佳化的 15 階調變器可能低於即時速度，導致數秒後音訊緩衝耗盡。需要逐步除錯轉換核心時，可在配置時指定 `-DXAMP_OPTIMIZE_PCM_DSD_DEBUG=OFF`；此時不保證即時播放。沒有變更 FIR、調變器、dither 或增益設定。

可用以下指令測量目前部署的轉換核心，過程不會開啟音訊裝置。`realtime_ratio` 需大於 1 並保留足夠餘裕；此測量不包含檔案解碼與 UI 負載。輸出的 SHA-256 可用於比較同一環境、相同參數下的轉換結果。

```sh
python3 tools/benchmark_pcm_dsd.py out/build/macos-xcode/src/xamp/Debug/xamp.app/Contents/MacOS/libpcm_dsd_converter.dylib --rate 96000 --seconds 20
```

macOS 的設定、資料庫與日誌位於 `~/Library/Application Support/xamp`，快取位於系統使用者快取目錄。

## macOS PCM2DSD / DoP

輸出裝置選擇 `CoreAudio (Hog Mode)`，在「設定 → 輸出」啟用 BitPerfect PCM 與 PCM2DSD，傳輸選 DoP。請先使用 DAC 明確支援的倍率，例如 DSD64 對應 176.4／192 kHz carrier（依來源取樣率系列）。不支援 Native DSD。

目前支援單一交錯雙聲道整數輸出串流，包含 24-bit、24-in-32 與 32-bit 容器。後端取得獨佔存取、關閉可設定的混音功能，並驗證 HAL 的實體／虛擬格式與 carrier 取樣率一致；不支援時會報錯，不會退回浮點輸出。停止關閉或開啟失敗時會嘗試還原格式、取樣率、混音狀態並釋放取得的獨佔權。播放中偵測到格式或獨佔權變更會停止輸出。

BitPerfect／DoP 略過 DSP 與軟體音量，請用 DAC 控制音量。CoreAudio 提供整數格式不代表 DAC 一定能解碼 DoP；必須由 DAC 規格確認。多串流、平面或浮點專用裝置目前不支援這條輸出路徑。

開發測試（先完成上方 Debug 應用程式建置）：

```sh
cmake -S src/xamp_output_device/tests/coreaudio -B out/build/coreaudio-integer-tests -G Ninja
cmake --build out/build/coreaudio-integer-tests
ctest --test-dir out/build/coreaudio-integer-tests --output-on-failure
```

測試涵蓋整數格式、DoP payload／marker、靜音與 EOF，以及使用模擬 HAL 執行正式後端的協商、失敗還原、獨佔衝突和格式變更。測試不會開啟實體音訊裝置，不能取代 DAC 實測。


## macOS 安裝包、簽署與公證

```sh
python3 tools/package_macos.py \
  --identity 'Developer ID Application: YOUR NAME (TEAMID)' \
  --notary-profile xamp-notary
```

腳本建置 Release，封裝 Qt、Homebrew 相依函式庫、BASS、PCM2DSD、MeCab 字典及資源，驗證動態連結不依賴開發機的路徑，再使用 Developer ID、Hardened Runtime 與時間戳記簽署。Apple 公證通過後將票證附加至 DMG 並驗證。產物與 SHA-256 位於 `out/package/macos/`；打開 DMG 後將 XAMP2.app 拖曳到 Applications 安裝。

僅支援 Apple Silicon；封裝工具會依所有函式庫實際的最低系統版本設定 LSMinimumSystemVersion，不能單靠主程式的 14.0 建置設定宣稱整包相容 macOS 14。實際最低版本會印在輸出摘要及 DMG 的安裝說明中。

第一次需在本機鑰匙圈安裝含私鑰的 Developer ID Application 憑證，再於終端機儲存公證認證（密碼在互動提示輸入，不寫入指令或儲存庫）：

```sh
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
xcrun notarytool store-credentials xamp-notary \
  --apple-id YOUR_APPLE_ID --team-id YOUR_TEAM_ID
```

`--skip-build` 可封裝現有 Release；`--smoke-test` 會在簽署後短暫啟動套件、驗證視窗初始化並自動退出（需要圖形介面）。省略 `--notary-profile` 只簽署，不公證；兩者都省略只建立標示為 `local-test` 的 ad-hoc 測試包，不能當作已通過 Gatekeeper 的正式版本散布。Bundle ID 預設 `com.billlin0904.xamp2`，可用 `--bundle-id` 覆寫。私鑰與公證認證不得加入 Git。


### 原生安裝導覽（PKG）

需要逐步安裝介面時，使用 `tools/build_macos_installer.py` 將已封裝的 `.app` 製作成 `.pkg`。包含繁體中文歡迎頁、安裝前須知、目標選擇、安裝類型、安裝與完成摘要；固定安裝至 `/Applications/XAMP2.app`，不搜尋其他位置的副本，也不執行自訂安裝腳本。

```sh
python3 tools/build_macos_installer.py /Volumes/"XAMP2 1.0.5"/XAMP2.app \
  --identity 'Developer ID Installer: YOUR NAME (TEAMID)' \
  --notary-profile xamp-notary
```

輸入應為已封裝、驗證過簽章的 `.app`（例如掛載上一步的 DMG 後取得）。Application 憑證簽署 `.app`；Installer 憑證簽署 `.pkg`，兩者用途不同。省略 `--identity` 會產生明確標示 `installer-unsigned.pkg` 的本機預覽版，內含的 `.app` 保持原有簽章。正式發行需提供 Installer 憑證並完成公證；產物與 SHA-256 同樣位於 `out/package/macos/`。
