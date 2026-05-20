# XAMP 2

XAMP 2 是一款專注本地音樂播放體驗的桌面音樂播放器。

它主要為 Windows 設計，支援常見音樂格式、專輯封面、歌詞、播放清單、等化器，以及 WASAPI / ASIO 等音訊輸出方式。目標是提供乾淨、快速、適合日常聽音樂的播放器。

## 主要功能

- 播放本機音樂檔案
- 管理音樂庫與播放清單
- 顯示專輯封面與歌詞
- 支援等化器與音訊處理
- 支援 WASAPI、ASIO、XAudio2 等 Windows 音訊輸出
- 支援 DSD 播放模式，依音訊設備能力而定

## 下載與安裝

Windows 使用者可以到 GitHub Releases 下載預先編譯好的安裝檔。

下載後執行安裝程式，依畫面指示完成安裝即可。

> macOS 版本目前可以自行編譯，但尚未提供正式下載檔。

## 支援平台

| 平台 | 支援狀態 |
|----------|----------|
| Windows | 主要支援 |
| macOS | 可編譯，暫不提供正式安裝檔 |

## 音訊輸出支援

| 平台 | 模式 | 輸出方式 |
|----------|----------|----------|
| Windows | Native DSD | ASIO |
| Windows | DSD to PCM | WASAPI Shared / Exclusive |
| Windows | DOP | WASAPI Exclusive |
| Windows | PCM | XAudio2 |
| macOS | DOP | CoreAudio |

實際可用的模式會依作業系統、音效卡、驅動程式與音訊檔案格式而有所不同。

## 從原始碼編譯

本專案主要使用 C++17 / C++20、Qt 6 與 FFmpeg。

如果你只是想使用播放器，建議直接下載 Releases 內的安裝檔。  
如果你想自行編譯，請先準備：

- Windows 10 / 11
- Visual Studio 2022
- Qt 6
- FFmpeg 4.4.4
- 專案所需的第三方函式庫

目前建置流程仍偏向開發環境使用，之後會再整理更完整的編譯說明。

## 授權

XAMP 2 本身使用 MIT License。

本專案也使用多個第三方開源或商業元件，例如 Qt、FFmpeg、BASS、TagLib、OpenCC、QWindowKit、spdlog 等。這些元件分別受其原始授權條款約束。

正式散布 binary 安裝檔時，會盡量保留並整理第三方元件的授權與 notice 資訊。

## 免責聲明

本專案仍在持續開發中。若你遇到無法播放、裝置不相容、歌詞或封面抓取失敗等問題，歡迎回報 issue。
