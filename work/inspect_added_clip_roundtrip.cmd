@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\inspect_added_clip_roundtrip.cpp /Fework\inspect_added_clip_roundtrip.exe
if errorlevel 1 exit /b 1
work\inspect_added_clip_roundtrip.exe work\test_session_track_clip_trim_roundtrip.dmx.duplicated.dmx
exit /b %errorlevel%
