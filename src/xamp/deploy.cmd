copy x64\Release\xamp.exe deploy\
copy x64\Release\opengl32sw.dll deploy\
copy x64\Release\libcrypto-1_1-x64.dll deploy\
copy x64\Release\libfftw3f-3.dll deploy\
copy x64\Release\taglib.dll deploy\
copy x64\Release\libssl-1_1-x64.dll deploy\
copy x64\Release\chromaprint.dll deploy\
copy x64\Release\libsoxr.dll deploy\
copy x64\Release\r8bsrc.dll deploy\

copy x64\Release\bass.dll deploy\
copy x64\Release\bassdsd.dll deploy\
copy x64\Release\bass_aac.dll deploy\
copy x64\Release\bass_fx.dll deploy\
copy x64\Release\bassmix.dll deploy\

copy x64\Release\xamp_base.dll deploy\
copy x64\Release\xamp_stream.dll deploy\
copy x64\Release\xamp_metadata.dll deploy\
copy x64\Release\xamp_output_device.dll deploy\
copy x64\Release\xamp_player.dll deploy\

copy x64\Release\xamp_base.pdb deploy\
copy x64\Release\xamp_stream.pdb deploy\
copy x64\Release\xamp_metadata.pdb deploy\
copy x64\Release\xamp_output_device.pdb deploy\
copy x64\Release\xamp_player.pdb deploy\
copy x64\Release\xamp.pdb deploy\

copy x64\Release\xamp.pdb deploy\
copy lincense.txt deploy\
copy credits.txt deploy\

xcopy /Y /S /I /E eqpresets deploy\eqpresets
xcopy /Y /S /I /E Resource deploy\Resource
xcopy /Y /S /I /E x64\Release\langs deploy\langs

C:\Qt\Qt5.12.9\5.12.9\msvc2017_64\bin\windeployqt --force deploy x64\Release\xamp.exe --release