@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\profile_prepared_playback.cpp /Fowork\profile_prepared_playback.obj /Fework\profile_prepared_playback.exe
if errorlevel 1 exit /b 1
work\profile_prepared_playback.exe work\roundtrip-11.dmx outputs\sfm64-prepared-playback-profile.json
