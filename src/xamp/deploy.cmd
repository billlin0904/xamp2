copy x64\Release\xamp.exe deploy\
copy x64\Release\opengl32sw.dll deploy\
copy x64\Release\libcrypto-1_1-x64.dll deploy\
copy x64\Release\libfftw3f-3.dll deploy\
copy x64\Release\libfftw3-3.dll deploy\
copy x64\Release\taglib.dll deploy\
copy x64\Release\libssl-1_1-x64.dll deploy\
copy x64\Release\libsoxr.dll deploy\
copy x64\Release\bassmix.dll deploy\
copy x64\Release\ebur128.dll deploy\
copy x64\Release\mimalloc-override.dll deploy\
copy x64\Release\mimalloc-redirect.dll deploy\
copy x64\Release\QSimpleUpdater.dll deploy\
copy x64\Release\r8bsrc.dll deploy\

copy x64\Release\bass.dll deploy\
copy x64\Release\bassdsd.dll deploy\
copy x64\Release\bass_aac.dll deploy\
copy x64\Release\bassflac.dll deploy\
copy x64\Release\bass_fx.dll deploy\
copy x64\Release\bassmix.dll deploy\
copy x64\Release\basscd.dll deploy\
copy x64\Release\bassenc.dll deploy\
copy x64\Release\bassenc_flac.dll deploy\

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
xcopy /Y /S /I /E x64\Release\fonts deploy\fonts

lrelease en_US.ts -qm en_US.qm
lrelease ja_JP.ts -qm ja_JP.qm
lrelease zh_TW.ts -qm zh_TW.qm
copy en_US.qm x64\Debug\langs\
copy ja_JP.qm x64\Debug\langs\
copy zh_TW.qm x64\Debug\langs\
copy en_US.qm x64\Release\langs\
copy ja_JP.qm x64\Release\langs\
copy zh_TW.qm x64\Release\langs\

C:\Qt\5.15.2\msvc2019_64\bin\windeployqt --force deploy x64\Release\xamp.exe --release