# XAMP 2

![image](https://github.com/billlin0904/xamp2/blob/master/github/demo.JPG)

# GOAL
- 專注本地端播放體驗與低延遲

- C++17 and Qt5
- 沒有呼叫任何系統獨佔鎖，盡可能使用lock free方式
- 減少GUI直接呼叫
- Windows 平台: 支持WASPI(共享/獨佔模式), ASIO
| Platform | DSD mode | Output Device Type |
|----------|----------|----------|
| Windows | Native DSD | ASIO |
| MAC OSX | DOP | CoreAudio |

# 第三方庫
- Taglib
- BASS
- FFmpeg
- Qt5
- FastMemcpy
- spdlog
