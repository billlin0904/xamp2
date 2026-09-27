# pcm_dsd_converter

純 C11 的 PCM → DSD 串流轉換 DLL，使用 C ABI。DSP 由
`PCM-DSD_Converter_v2/dsd15_core.py` 移植，原作者為 serieril (PCMDSD.com)，
保留 MIT 授權於 `LICENSE`。FIR 及 dc15_signal_guard_best 係數已嵌入 DLL。
執行時不需要 Python、NumPy、SciPy、Numba、Qt 或外部係數檔。

## 產物與建置

FFT 預設使用 Intel oneMKL（靜態連結、sequential、LP64）；建置需要 oneMKL SDK。
可透過 `MKL_DIR` 或 `MKLROOT` 指定安裝位置，Windows 也會搜尋標準 oneAPI 安裝目錄。
每個升頻 stage 建立並重用 DFTI descriptor，inverse scale 為 1/N；不在播放迴圈建立計畫。
設定 `-DPCM_DSD_USE_MKL=OFF` 可建置原本 portable FFT 備援版本。
MKL 與 portable 的浮點結果可能略有差異，因此不保證輸出 DSD 逐 bit 相同。
oneMKL 授權及第三方聲明位於 `licenses/onemkl`，隨產物部署。
`pcm_dsd_benchmark` 可在 Release 下測量 DSD64/128/256，排除建立計畫時間並先暖機。

Windows 下另有 `pcm_dsd_profile` 與 `pcm_dsd_profile_control`，兩者實際呼叫
`pcm_dsd_process`，輸出 CSV。前者以 QPC 計時各處理區段，後者不插入區段計時，
用來檢查量測干擾與輸出 hash。測試執行緒固定於 logical CPU 0，使用預先產生的
1 秒 44.1 kHz 立體聲訊號，原版 FIR／15 階／預設 dither，每組先暖機再跑 5 次。
涵蓋 DSD64/128/256、block 256/1024；create、reset、輸出 hash 不計入 process 時間，
flush 另計。區段計時僅在 profile target 啟用，正式 DLL 沒有計時器或計數器。

x64 預設啟用 `PCM_DSD_USE_SIMD`：15 階調變器以 SSE2 同時計算左右聲道，
樣本與 SOS 階段仍依原順序計算。每區塊載入／存回各聲道狀態，使用區域變數打包，
串流路徑省去不參與回授的 run/prev/last 診斷更新；舊 `dsd15_modulate_pack` API 不變。
頻域乘法使用獨立 AVX2 編譯單元，create 時檢查 CPU AVX2 與 OS YMM 支援才選用，
否則保留純量乘法。不使用 FMA，維持原運算順序。1 階模式仍使用純量調變器。
`-DPCM_DSD_USE_SIMD=OFF` 可關閉這兩項最佳化；不影響 MKL 自身的 CPU dispatch。
`pcm_dsd_simd_test` 比較輸出 bytes、回授狀態、獨立 RNG、過載、carry、reset 與各 DSP 選項。
`pcm_dsd_profile_control --scalar` 可在同一測量程式中切回純量路徑，與預設 SIMD 路徑比較。

```powershell
.\build_release.ps1
# 等同於明確指定 Release：
.\build.ps1 -Configuration Release
# 同時與原始 Python 專案比對，需要測試環境安裝 numpy/scipy/numba：
.\build.ps1 -ReferencePath F:\Source\PCM-DSD_Converter_v2
```

預設使用 Visual Studio 2026、MSVC、Windows SDK、CMake，產出 Windows x64：

Visual Studio 是多組態 generator；直接執行 `cmake --build build` 未指定組態時，
可能建置 Debug。請使用上述腳本，或明確執行：

```powershell
cmake --build build --config Release
cmake --install build --config Release --prefix dist
```

在 Visual Studio 開啟產生的 solution 時，也請選擇工具列的 `Release | x64`。
`CMAKE_BUILD_TYPE=Release` 不會替 Visual Studio 選定組態。

- `dist/bin/pcm_dsd_converter.dll`
- `dist/lib/pcm_dsd_converter.lib`（匯入函式庫）
- `dist/include/pcm_dsd_converter.h`

可用 `-Generator 'Visual Studio 17 2022'` 指定其他已安裝的編譯器；不同 generator
應使用另一個 CMake build 目錄。Release DLL 靜態連結 MSVC C runtime。
建置不需要 Python；只有重新產生係數或參考測試需要 Python。
本子目錄可獨立建置；xamp_stream 專案也會自動建置並部署 Release converter。

## 格式與範圍

- 輸入：normalized `double`，frame-interleaved PCM，一或兩聲道；單聲道複製為立體聲。
- 輸出：兩聲道 packed DSD，MSB-first，排列為 `L byte, R byte, L byte, R byte`。
- `process` 的輸入數量、consumed 是 **PCM frames**；capacity、written 是 **bytes**。
  一個 PCM frame 包含所有輸入聲道。不要直接把 written 當作 FileStream sample 數。
- 支援 44.1kHz 與 48kHz 系列，以及 DSD16、32、64、128、256、512、1024、2048。
  輸出 rate = 系列基準 × DSD 倍率；輸出/輸入 rate 必須為不小於 1 的 2 次方整數。
- 預設所有倍率共用原版 FIR 和 15 階調變設定；可透過 `pcm_dsd_create_ex` 選擇下列 DSP 選項。
- 預設 gain=0.5（包含調變器 headroom）、Dither=2^-24、seed=20260513。
  對應原版 UI 的 0 dB 輸入設定。若要 UI 的 dB 增益，使用 `0.5 * pow(10, db/20)`。
- NaN 輸入置零，正負 infinity 置為正負 1；有限輸入值不另行裁切。
- 不包含 WAV/DFF 讀寫、DoP framing 或音訊裝置輸出。外層負責檔案解析與傳輸。

## 生命週期與呼叫合約

1. `pcm_dsd_default_config()` 初始化 config，可調整 rate/channels/multiplier/gain/dither/seed。
2. `pcm_dsd_create()` 建立不透明 handle，預先分配 FIR、FFT 及輸出工作空間。
3. 重複呼叫 `pcm_dsd_process()`；依 consumed 前進輸入指標，將 written bytes 交給下游。
4. EOF 後重複呼叫 `pcm_dsd_flush()`，直到 `PCM_DSD_FINISHED`；**最後一次也可能帶有輸出**。
5. `pcm_dsd_destroy()` 釋放資源。

```c
#include "pcm_dsd_converter.h"

pcm_dsd_config config;
pcm_dsd_converter* converter = NULL;
pcm_dsd_default_config(&config);
config.input_sample_rate = 44100;
config.channels = 2;
config.dsd_multiplier = 64;
int status = pcm_dsd_create(&config, &converter);
/* Check status before using converter; see tests/test_converter.c for a runnable example. */
```

輸出緩衝区至少 2 bytes；奇數容量的最後 1 byte 不使用。小緩衝區會產生部分輸出，
此時可能 consumed=0、written>0；累積輸入時也可能 consumed>0、written=0。
可以以 `pcm=NULL, frames=0` 呼叫 process 排出已完成的輸出，但不會觸發 EOF。
輸入輸出不能重疊，所有傳入指標及尺寸由呼叫端保證有效。

內部分塊預設 256 PCM frames，可選 64／128／256／512／1024；固定同一組設定時，呼叫端的 chunk 邊界不影響輸出。
變更內部區塊可能改變 FFT 浮點加總順序，因此不同 block_frames 之間不保證逐 bit 相同。
使用每級 2 倍的 FFT overlap-add FIR；portable 後端無外部 FFT 相依。這與原版 polyphase/FFT
在數學上的濾波響應一致，但浮點加總順序不同。
portable FFT 在 create 時預先建立 twiddle 與 bit-reversal 表；MKL 使用 DFTI 計畫。
兩種後端都將左右聲道以單次複數卷積一起計算。
process/flush 不分配記憶體，且不執行檔案 I/O。單個 handle 非 thread-safe；不同 handle
可在不同執行緒使用。高倍率需要更多工作記憶體與 CPU，尚未驗證即時播放期限。

`flush` 與原版同樣截斷到輸入對應的 DSD frame 長度，不額外輸出 FIR 尾端，也不補償
FIR 群延遲；不足 8 bits 的最後 byte 在低位補零。總 byte 數為
`2 * ceil(input_frames * up_ratio / 8)`。flush 後必須 reset 才能再 process。

`pcm_dsd_reset()` 清除 FIR 歷史、調變與亂數狀態、未讀輸出、EOF 狀態，保留設定及記憶體。
Seek 時外層應重定位 PCM 來源並 reset；需要處理預熱暫態，reset 不保證與從頭播放的
同一時間點位元一致。C API 不會直接控制來源 seek。

## 舊版 native 入口

另匯出 `dsd15_modulate_pack`，符合原版 Python ctypes 所期待的 ABI。
這個低階入口只處理**已升頻的單聲道**資料，固定使用原版 dc15 特化的 loop 常數；
係數矩陣須是原版 8×6 SOS 格式（各節 b0/a0=1），不能用來替代一般任意 SOS 調變器。
可將 DLL 另存為原專案指定的 `native/dsd15_native.dll` 並設定
`CODEX_DSD15_USE_NATIVE=1`。本建置不修改原專案。

## 驗證與限制

`ctest`：C 呼叫端 DLL 載入/連結、reset、EOF、無效 rate，以及 FFT 對照直接 FIR 卷積。
`tests/verify_reference.py`：

- 8 種測試組合（靜音、亂數、正弦、過載 × Dither 開關），各 23 塊，調變 bytes 與狀態
  精確符合原版 Numba；測試保留 uint64 亂數型別。
- 9,001-frame 立體聲 DSD64 升頻對照原版 SciPy，最大絕對誤差須 < 1e-10。
- 呼叫端分塊、3-byte 小容量緩衝區、兩種系列全倍率、mono 複製、EOF 尺寸與補位。
- up=1 的完整串流與原版逐 byte 一致。

這些測試**不代表完整 PCM→DSD 在升頻時與 Python 逐 byte 一致**，也不構成所有倍率
的音質認證。FFT 的微小誤差可能改變後續 1-bit 調變結果。未加入 FileStream 或 DAC 播放
整合、效能最佳化及主觀聽感測試。

## DSP 選項（pcm_dsd_create_ex）

舊 `pcm_dsd_config` 結構、`pcm_dsd_create` 與 `dsd15_modulate_pack` ABI 保持不變。
新 API 使用獨立 `pcm_dsd_options`；先呼叫 `pcm_dsd_default_options` 再設定：

| 欄位 | 選項 | 預設 |
|---|---|---|
| filter | `PCM_DSD_FIR_ORIGINAL`（4095 taps）、`PCM_DSD_FIR_BLACKMAN_1023`、`PCM_DSD_FIR_BLACKMAN_511` | ORIGINAL |
| modulator | `PCM_DSD_MODULATOR_15`、`PCM_DSD_MODULATOR_1` | 15 |
| block_frames | 64、128、256、512、1024 | 256 |

```c
pcm_dsd_options options;
pcm_dsd_default_options(&options);
options.filter = PCM_DSD_FIR_BLACKMAN_1023;
options.block_frames = 512;
int status = pcm_dsd_create_ex(&config, &options, &converter);
```

所有濾波器均為線性相位；新增兩組使用 Blackman-windowed sinc，截止頻率為每級輸出
Nyquist 的 0.5 倍，插零後 DC 增益正規化為 2。較短 FIR 的過渡頻帶較寬、群延遲較小。
不升頻（up_ratio=1）時不執行 FIR，所以濾波器選擇不改變輸出。
設計方法參考 [SciPy firwin](https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.firwin.html)。

1 階為獨立的 error-feedback sigma-delta 實作：累加輸入與前次量化誤差、輸出 ±1，
保留新的量化誤差。增益及 dither 之後的輸入限制在 ±1，避免過載時累積誤差；
原版 15 階路徑保持不變。1 階較省 CPU，但帶內噪聲較高，UI 標示為實驗性，
一般聆聽仍預設 15 階。架構參考 [Analog Devices sigma-delta tutorial](https://www.analog.com/en/resources/interactive-design-tools/sigma-delta-adc-tutorial.html)。
不支援任意截短原版 SOS 來指定 3／5／7 階。

較大的處理區塊可能降低 FFT 開銷，但增加記憶體與單次轉換延遲；實際效能需依 CPU
與倍率量測。所有工作空間仍在 create 時配置，process／flush 不做配置。
單獨調整此參數不保證改善 underrun。

`pcm_dsd_options` 測試涵蓋 30 組選項的 chunk/reset/EOF、預設 ABI 相容、FIR 對稱性、
DC／通帶／阻帶、跨區塊直接卷積，以及 1 階 DC 密度與過載狀態界限。未執行實體 DAC 聽測。
