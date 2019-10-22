# XAMP 2

![image](https://github.com/billlin0904/xamp2/blob/master/github/demo.JPG)

# GOAL
- 專注本地端播放體驗與低延遲

- C++11 and Qt5
- 沒有呼叫任何系統獨佔鎖，盡可能使用lock free方式
- 減少GUI直接呼叫
- Windows 平台: 支持WASPI(共享/獨佔模式), ASIO
- 支持原始DSD(RAW DSD)回放

# 第三方庫
- Taglib
- BASS
- FFmpeg
- Qt
- FastMemcpy
- spdlog
