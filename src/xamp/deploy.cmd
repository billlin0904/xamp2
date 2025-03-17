@echo off

REM 定義常用路徑變數
SET BUILD_DIR=x64\Release
SET DEPLOY_DIR=deploy

REM 建立部署目錄（若不存在）
if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"

REM 複製 DLL 檔案 (可考慮使用通配符，但需確認無多餘不必要的檔案)
copy %BUILD_DIR%\xamp.exe %DEPLOY_DIR%\
copy %BUILD_DIR%\opengl32sw.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\libcrypto-1_1-x64.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\libfftw3f-3.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\libfftw3-3.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\taglib.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\libssl-1_1-x64.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\libsoxr.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\bassmix.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\ebur128.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\mimalloc-override.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\mimalloc-redirect.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\widget_shared.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\QSimpleUpdater.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\r8bsrc.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\QWKCore.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\QWKWidgets.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\supereq.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\mecab.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\opencc.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\libprotobuf-lite.dll %DEPLOY_DIR%\

REM 拷貝 components 資料夾
robocopy %BUILD_DIR%\components %DEPLOY_DIR%\components /E
REM 拷貝 opencc 資料夾
robocopy %BUILD_DIR%\opencc %DEPLOY_DIR%\opencc /E

REM 複製其餘 DLL、pdb、txt、json 檔案
copy %BUILD_DIR%\xamp_base.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_stream.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_metadata.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_output_device.dll %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_player.dll %DEPLOY_DIR%\

copy %BUILD_DIR%\xamp_base.pdb %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_stream.pdb %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_metadata.pdb %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_output_device.pdb %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp_player.pdb %DEPLOY_DIR%\
copy %BUILD_DIR%\xamp.pdb %DEPLOY_DIR%\

copy lincense.txt %DEPLOY_DIR%\
copy credits.txt %DEPLOY_DIR%\
copy fonticon.json %DEPLOY_DIR%\

REM xcopy EQ presets, Resource, langs, fonts
robocopy eqpresets %DEPLOY_DIR%\eqpresets /E
robocopy Resource %DEPLOY_DIR%\Resource /E
robocopy %BUILD_DIR%\langs %DEPLOY_DIR%\langs /E
robocopy %BUILD_DIR%\fonts %DEPLOY_DIR%\fonts /E

REM 產生多語檔 en_US.qm
lrelease en_US.ts -qm en_US.qm

copy en_US.qm x64\Debug\langs\
copy en_US.qm %BUILD_DIR%\langs\

REM 執行 windeployqt
C:\Qt\6.7.2\msvc2019_64\bin\windeployqt --force %DEPLOY_DIR% %BUILD_DIR%\xamp.exe --release
