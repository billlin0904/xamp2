copy x64\Release\xamp.exe deploy\
copy x64\Release\opengl32sw.dll deploy\
copy x64\Release\libcrypto-1_1-x64.dll deploy\
copy x64\Release\libfftw3f-3.dll deploy\
copy x64\Release\taglib.dll deploy\
copy x64\Release\libssl-1_1-x64.dll deploy\
copy x64\Release\chromaprint.dll deploy\
copy x64\Release\libsoxr.dll deploy\

copy x64\Release\bass.dll deploy\
copy x64\Release\bassdsd.dll deploy\
copy x64\Release\bass_aac.dll deploy\

copy x64\Release\xamp_base.dll deploy\
copy x64\Release\xamp_stream.dll deploy\
copy x64\Release\xamp_metadata.dll deploy\
copy x64\Release\xamp_output_device.dll deploy\
copy x64\Release\xamp_player.dll deploy\

xcopy /Y /S /I /E xamp\Resources deploy\Resources
xcopy /Y /S /I /E x64\langs deploy\langs

C:\Qt\Qt5.12.5\5.12.5\msvc2017_64\bin\windeployqt --force deploy x64\Release\xamp.exe --release