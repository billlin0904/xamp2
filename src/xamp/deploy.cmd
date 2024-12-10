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
copy x64\Release\widget_shared.dll deploy\
copy x64\Release\QSimpleUpdater.dll deploy\
copy x64\Release\r8bsrc.dll deploy\

copy x64\Release\widget_shared.dll deploy\

copy x64\Release\QWKCore.dll deploy\
copy x64\Release\QWKWidgets.dll deploy\

copy x64\Release\supereq.dll deploy\
copy x64\Release\mecab.dll deploy\

xcopy x64\Release\components deploy\components\ /E /I /H /C /Y

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
copy fonticon.json deploy\

xcopy /Y /S /I /E eqpresets deploy\eqpresets
xcopy /Y /S /I /E Resource deploy\Resource
xcopy /Y /S /I /E x64\Release\langs deploy\langs
xcopy /Y /S /I /E x64\Release\fonts deploy\fonts

lrelease en_US.ts -qm en_US.qm

copy en_US.qm x64\Debug\langs\
copy en_US.qm x64\Release\langs\

C:\Qt\6.7.2\msvc2019_64\bin\windeployqt --force deploy x64\Release\xamp.exe --release