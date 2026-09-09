# Integer BitPerfect PCM

在 Settings → Output 啟用 **BitPerfect PCM (WASAPI Exclusive / ASIO)**，再從裝置選單選擇 WASAPI 獨佔或 ASIO 輸出。預設關閉，下次開始播放生效。

## 資料契約與路徑

`src/xamp_base/include/base/pcm.h` 定義：

- `pcm::Format`：取樣率、聲道、整數／浮點、有效位元、容器位元、大小端、左右對齊、交錯／平面排列。
- `pcm::Block`：唯讀 `std::span<const std::byte>`、frame 數及格式。span 不擁有資料；呼叫端必須維持 buffer 的生命週期。
- `pcm::convert`：完全相同的格式直接複製 bytes。不同格式僅進行無損位元排列、符號延伸、補零或聲道拆分；拒絕降低有效位元數、改變取樣率、聲道重映射與浮點輸出。

正式 BitPerfect 路徑：

**FFmpeg 整數解碼 → 32-bit 整數容器的 byte block → DSPManager::processPcm → byte FIFO → 後端格式轉換 → 裝置**

支援 16/24/32-bit 立體聲整數 PCM WAV 及 FLAC。解碼器直接取得 S16/S32（或對應 planar）資料，無 swresample、float、EQ、tempo、軟體音量或 MQA 解碼。S16 無損擴充到 S32，24-bit S32 保留有效 24 位；真正的 32-bit S32 保留所有位元。未知來源精度或不支援的 codec／格式會拒絕。

舊的 `packPcm(float...)` 已移除；DSP 的 float 入口在 BitPerfect 模式會拒絕呼叫。視覺化需要浮點資料時使用獨立副本，不寫回播放 buffer。一般播放模式仍使用原本的 float／DSP 路徑。

## 輸出後端

### WASAPI Exclusive

以來源取樣率協商 PCM。16-bit 使用 16-bit 容器；24-bit 先嘗試 packed 24，再嘗試 32-bit 容器／24-bit 有效位元。32-bit 來源要求 32-bit 有效位元，不退回 24-bit。

WAVEFORMAT 的 `wBitsPerSample`、`wValidBitsPerSample`、`nBlockAlign`、`nAvgBytesPerSec` 分別建立，不從 Windows 共用 mix format 推測。資料左對齊、little endian、交錯聲道。

### ASIO

讀取實際輸出 channel sample type，支援整數 16/24/32 以及 Int32 內的 16/18/20/24 有效位元型態，處理 little/big endian。ASIO SDK 2.3 第 33–34 頁定義：一般 Int32 型態左對齊；`ASIOSTInt32LSB24`／`MSB24` 等變體是右對齊且高位符號延伸，不能套用 WASAPI 的 24-in-32 排列。

每個聲道寫入獨立 buffer。目前選擇的兩個輸出聲道必須有相同 sample type；不同型態、Float／DSD、有效位元不足都明確拒絕。設定取樣率後再讀回確認；播放途中收到不同取樣率通知會停止嚴格路徑。

**驅動宣告的 Int32 只表示傳輸格式，不代表 DAC 物理解析度已達 32-bit。**

## 音量與緩衝

軟體音量／靜音在嚴格模式停用，不會強制將硬體音量設成 100%。請先使用 DAC 調整音量；繞過既有軟體衰減可能變大聲。

預填靜音不消耗來源。完整與部分曲尾都送出；後端負責在 buffer 排空後通知播放結束。ASIO 依驅動回報的輸出延遲再保留排空週期，且只清空當次可寫的 buffer。緩衝不足報錯，不重播舊資料。FFmpeg seek 依 timestamp 丟棄定位前的 frames，避免落在前一個解碼區塊就開始播放。

## Catch2 v3

先建置 `src/xamp/xamp.sln` 的 Release。測試独立於正式程式，Catch2 不連入播放器；安裝包排除 `*_test.exe`。

```powershell
cmake -S . -B out/bitperfect-tests -G "Visual Studio 18 2026" -A x64 `
  -DXAMP_BUILD_TESTS=ON `
  -DXAMP_TEST_RUNTIME_DIR=F:/Source/xamp2/src/xamp/x64/Release
cmake --build out/bitperfect-tests --config Release --parallel
ctest --test-dir out/bitperfect-tests -C Release --output-on-failure
```

請調整 runtime 路徑。CMake 優先使用已安裝的 Catch2 v3，否則下載固定 v3.8.1；省略 runtime 路徑可只執行不需正式 DLL 的測試。

- `src/xamp_stream/tests/pcm_conversion_test.cpp`：完整 16-bit 數值、32-bit 極值與最低位元、大小端、packed 24、左右對齊、聲道排列、拒絕有損格式／不完整資料及曲尾判定。
- `src/xamp_stream/tests/bitperfect_pipeline_test.cpp`：實際 FFmpeg 16/24/32-bit WAV／FLAC → DSP bypass → FIFO → 格式轉換，逐 byte 與原始 PCM 比對，包含非整塊曲尾、非整秒 seek、正常 float 解碼回歸及不支援來源。
- `src/xamp_output_device/tests/output_format_test.cpp`：ASIO sample type 對應與右對齊符號延伸、WASAPI 容器／有效位元／frame 大小的格式結構。

Fixtures 完全由程式產生。`python tests/generate_fixtures.py` 使用 FFmpeg 及 libFLAC；32-bit FLAC 由 libFLAC 建立，避免 FFmpeg 編碼器默默降為 24-bit。原始整數 bytes 是比對依據，不能用待測 converter 的輸出當 oracle。

## 驗證界線

這些自動測試驗證軟體資料與後端格式編碼，沒有開啟實體音訊裝置。實機取樣率切換、ASIO driver callback／暫停恢復／曲尾、WASAPI 裝置支援度及 DAC 最終輸出仍需實機驗證。數位回錄對齊後的有效樣本比較，才能確認驅動與硬體是否也完整保留資料。
