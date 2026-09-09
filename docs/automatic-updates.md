# 自動更新

Settings → 軟體更新提供手動檢查、自動檢查（預設開啟）、自動下載（預設關閉）、下載進度、取消與重新啟動更新。啟動後 5 秒及每 24 小時檢查；背景檢查不顯示對話框。下載完成停留 Ready，只有使用者點擊重新啟動更新才安裝。

## 實作

ApplicationUpdater 使用 QNetworkAccessManager 非同步取得 manifest、串流寫入快取並計算 SHA256。下載逾時或寫入失敗會刪除不完整檔案；取消後可以重新下載。禁止 HTTP 安裝檔網址、缺少雜湊或無效 manifest；重複請求被忽略。安裝前在背景重新驗證磁碟檔案，驗證失敗不執行。

Windows 沿用 Inno Setup，/UPDATE 加上目前安裝目錄，靜默安裝後重新開啟播放器。正常退出負責釋放音訊與資料庫。PlaybackController 將當前歌曲、位置、來源與 captured queue 保存至設定，新版啟動恢復為暫停，按播放才開啟音訊。音量與順序沿用既有設定。失去來源檔案時略過恢復。此機制不承諾恢復移除的 CD 或遠端來源。

## 發佈

1. 同步應用程式版本與 Inno Setup MyAppVersion；建置並準備 src/xamp/deploy。
2. ISCC 可透過 /DMyAppVersion=版本 設定安裝版本。腳本路徑已改為相對路徑，排除使用者 xamp.ini、config.json 與資料庫。
3. 正式發佈時先完成安裝包簽署，再執行 setup/win32/New-UpdateManifest.ps1，傳入 Version、Package、Output 和 Notes。
4. 將安裝包以 xamp2-setup.exe 上傳至 v版本 GitHub Release。
5. 確認固定版本的檔案可下載後，才發布生成的 updates.json。

現有公開 manifest 沒有在這次修改中切換版本，也沒有上傳 release。SHA256 是完整性驗證，目前尚未加入發行者簽章驗證；正式簽章金鑰／憑證仍需配置。Linux 保留 AppImage 安裝路徑，本輪主要驗證 Windows。

## 測試與限制

applicationupdater_test 使用本機 manifest 和測試檔案驗證無效資訊、舊版、HTTP 拒絕、重複請求、雜湊失敗、Ready 等待、取消與檔案清理，不執行安裝。可使用 XAMP_BUILD_ARCHITECTURE_TESTS 建置測試。

尚未進行跨版本安裝端到端測試、磁碟滿額實機測試、簽章驗證或失敗自動回滾。下載採完整重試，沒有續傳；離開程式後下次重新檢查／下載，Ready 狀態不跨重啟保留。更新會話恢復的實際音訊／清單整合需在跨版本安裝驗收時確認。

Inno Setup 參考：[Run section](https://jrsoftware.org/ishelp/topic_runsection.htm)、[ParamStr](https://jrsoftware.org/ishelp/topic_isxfunc_paramstr.htm)。
