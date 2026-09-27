# PCM2DSD playback

設定 → 輸出：先勾選 BitPerfect PCM，再勾選 PCM2DSD。預設關閉。
可設定 DSD16～2048、輸入增益 -24～+12 dB（預設 0 dB，保留核心 0.5 headroom）、
Dither（預設開啟）。設定儲存在 xamp.ini，下次開啟音軌生效。

僅對既有 BitPerfect 所支援的立體聲 16/24/32-bit integer WAV／FLAC 啟用。
BitPerfect 關閉時不建立轉換器；原生 DSD 檔不經此轉換器。
DoP 需要相容 DAC 的 WASAPI 獨佔或 ASIO 整數輸出；Native DSD 僅限相容 ASIO 驅動。拒絕共享模式與其他裝置類型。

資料路徑：

```text
AvLibFileStream (integer PCM)
  → Pcm2DsdFileStream (double PCM → C DLL → packed DSD → integer DoP)
  → strict integer FIFO (no EQ/resampler/software volume)
  → WASAPI Exclusive / ASIO integer container conversion
  → DoP-capable DAC
```

預設採 DoP，也可在設定中選擇 Native DSD（ASIO）。DoP 的 DSD64 使用 176.4/192 kHz carrier，
DSD128 使用 352.8/384 kHz，依輸入的 44.1/48 kHz 系列決定。
介面顯示實際 carrier 與 DSD rate，不把轉換後的訊號標為原始 PCM bit-perfect。
DAC 需自行支援 DoP；作業系統能接受某 PCM rate 並不能證明 DAC 能解碼 DoP。

DoP 排列依 [DoP open Standard v1.1](https://dsd-guide.com/dop-open-standard)：
marker 0x05/0xFA 跨 frame 交替、同 frame 左右聲道相同；最早的 DSD bit 放在 bit15。
產生 signed 24-bit、left-aligned 32-bit little-endian carrier，沿用現有無損整數容器轉換。
不足一個完整 DoP frame 時，用 DSD idle byte 0x69 補齊缺少的聲道 byte pair。

Seek 清空緩衝、重置調變器，並從目標前最多 4,096 PCM frames 預熱後丟棄預熱輸出。
保留原 FIR 群延遲／截尾行為；seek 後不保證與從頭播放同位置逐 bit 相同。

## Build / tests

PCM2DSD 的播放 FIFO 目標容量為 16 秒（上限 128 MiB），開始播放及 seek 時先轉換
4 秒音訊；一般 BitPerfect PCM 維持原有 8 秒容量與 1 秒預先緩衝。
預先緩衝會增加開始播放／跳轉的等待時間，不能補足長期低於即時速度的轉換效能。
播放狀態簡化為 `FLAC → DSD64 | DoP | WASAPI Exclusive`，詳細 MHz 與 DoP
傳輸取樣率保留在滑鼠提示中。

`Pcm2DsdFileStream` 位於 `xamp_stream/include/stream/pcm2dsdfilestream.h` 與
`xamp_stream/src/pcm2dsdfilestream.cpp`，由 `xamp_stream.dll` 匯出；使用
`#include <stream/pcm2dsdfilestream.h>` 與 `xamp::stream::Pcm2DsdFileStream`。
串流模組使用原生 DLL 載入器，不依賴 Qt。

MSBuild 的 xamp_stream.vcxproj（含 Source/Header Files filters）在 x64 組態會自動建置 Release
converter 並複製 DLL 與 MIT license 到輸出目錄，包含 Debug xamp_stream 時。
xamp.vcxproj 透過 ProjectReference 建置 xamp_stream，後者再相依於 xamp_base；主程式使用匯出的類別，不再直接編譯此類別。
CMake 也會建置並部署 converter target。

```powershell
cmake -S src/xamp/tests/pcm2dsd -B out/build/pcm2dsd-tests -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build out/build/pcm2dsd-tests --config Release
ctest --test-dir out/build/pcm2dsd-tests -C Release --output-on-failure
src/xamp/x64/Release/pcm2dsd_stream_test.exe --benchmark
```

測試透過 xamp_stream.dll 匯出的類別載入 C DLL：16/24/32-bit source、DoP payload/marker、禁止的 DSP 不執行、
caller chunk、EOF、seek/reset，以及 Qt 設定預設值、BitPerfect gate、持久化及繁中畫面。
另有 converter 自身的 Numba 位元對照與 SciPy FIR 波形對照測試。

本機 2 秒合成來源的測試（轉換加 strict FIFO，非 DAC 實測）約 DSD64 2.26×、
DSD128 1.13×、DSD256 0.56× 即時速度。較高倍率未必能持續即時播放；需要更快 CPU
及支援對應 carrier rate 的 DAC。未執行實體 DAC 聽測。

## 可調整的 DSP 參數

設定頁已提供 FIR（原版 4095 taps／Blackman 1023／511）、調變器（原版 15 階／
實驗性 1 階）、內部處理區塊（64／128／256／512／1024 PCM frames）。
預設維持原版 FIR、15 階及 256 frames；下次播放才套用，僅在 BitPerfect PCM2DSD 路徑啟用。
1 階的帶內噪聲較高，適合低負載測試；較大的區塊會增加記憶體與單次處理延遲。
C ABI 選項及 DSP 測試細節見 `../thirdparty/pcm_dsd_converter/README.md`。

## ASIO Native DSD

設定頁「DSD 傳輸模式」可選 `DoP (WASAPI / ASIO)`（預設）或 `Native DSD (ASIO)`。
Native 僅限 ASIO，驅動需接受 DSD IO format、指定的 DSD bit clock，以及 stereo
`ASIOSTDSDInt8MSB1` 或 `ASIOSTDSDInt8LSB1`。不支援 NER8；不支援時回報錯誤，
不會偷偷退回 PCM。可手動選回 DoP。

`Pcm2DsdFileStream::setDSDMode(DSD_MODE_NATIVE)` 會重置串流，改成每 sample 為
1 個 packed DSD byte（stereo L,R interleaved），`getFormat()` 的 rate 是 DSD bit clock，
`integerPcmFormat()` 為空。AudioPlayer 直接寫入／讀取 byte FIFO，不呼叫 float DSP，
保持 16 秒目標 FIFO 容量、4 秒預先緩衝（容量上限 128 MiB）。

ASIO 僅在最後輸出時拆分左右聲道，依驅動 MSB／LSB 要求反轉各 byte 的位元順序。
ASIO bufferSize 為 DSD samples，因此每聲道 byte 數為 bufferSize/8，時間由輸出的
DSD samples 計算。尾端及停止時用 0x69（LSB 為 0x96）idle，EOF 後等待裝置 buffer
排空再通知播放結束；軟體音量與 DSP 維持略過。

狀態列例如 `FLAC → DSD64 | Native DSD | ASIO`，提示顯示實際 MHz。
自動測試涵蓋原始 C DLL payload 對照、tiny reads、模式切換、seek/EOF、MSB／LSB
聲道分離與 idle 尾端，以及傳輸設定持久化。尚未執行實體 ASIO DAC 播放驗證。
