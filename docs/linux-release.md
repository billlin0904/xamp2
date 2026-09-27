# XAMP 2 Linux 1.0.3 測試版

此套件適用於 Linux x86_64，建置與啟動驗證環境為 Ubuntu 24.04／WSL2、Qt 6.4.2。需要 glibc 2.39 或更新版本；其他發行版尚未驗證。

## 下載與啟動

從 [GitHub Release](https://github.com/billlin0904/xamp2/releases/tag/v1.0.3) 下載 `xamp2-1.0.3-linux-x86_64.zip`，解壓縮至可寫入的資料夾：

```bash
unzip xamp2-1.0.3-linux-x86_64.zip
chmod +x linux-x64/XAMP.AppDir/AppRun linux-x64/XAMP.AppDir/usr/bin/xamp linux-x64/XAMP.AppDir/usr/bin/xamp.bin
./linux-x64/XAMP.AppDir/AppRun
```

使用 `AppRun` 啟動，讓啟動器設定 Qt 與動態函式庫路徑。請保留完整 AppDir 結構。這是 ZIP 免安裝套件，不是 AppImage、deb 或 rpm。

套件不含個人音樂庫、資料庫、設定或快取。首次啟動會在 `XAMP.AppDir/usr/bin` 建立 `xamp.db`、`xamp.ini` 等檔案。更新前請關閉播放器並備份這些檔案。

## 音訊與驗證範圍

- Linux 提供 PulseAudio、PipeWire 與 ALSA 後端，實際可用裝置依系統而定。
- WSLg 建議選擇 PulseAudio 的 `RDPSink`；Windows 的 G 槽在 WSL 中為 `/mnt/g`。
- 已通過 Release 建置、套件相依檢查、主視窗啟動，以及 PCM→DSD MKL／SIMD 的 4 項自動測試。
- 一般播放、各種 DAC 與硬體輸出仍待人工驗證。Windows 的 WASAPI、ASIO、Mica／Acrylic 不適用於 Linux；轉換器測試通過不代表 WSLg 支援 DSD 直出。

## WSL 視窗沒有顯示

如果工作列有 XAMP，但視窗透明且標題含 `[WARN:COPY MODE]`，可能是 [WSLg 共享記憶體問題](https://github.com/microsoft/wslg/issues/1456)。本次環境在重啟 WSL 後恢復正常。

先儲存並關閉其他 WSL 工作，再於 Windows 終端機執行 `wsl --shutdown`。重新開啟 Ubuntu 終端機並保持開啟，然後執行 `AppRun`。

問題回報：[GitHub Issues](https://github.com/billlin0904/xamp2/issues)。請附作業系統、音訊後端與錯誤訊息，並先移除紀錄中的個人檔案路徑。
