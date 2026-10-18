@echo off
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\outputs\save-copy-safety-20261014
set TMP=%CD%\outputs\save-copy-safety-20261014
if not exist "%TEMP%" mkdir "%TEMP%"
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_session_track_clip_trim.cpp /Fo:outputs\save-copy-safety-20261014\test_session_track_clip_trim.obj /Fe:outputs\save-copy-safety-20261014\test_session_track_clip_trim.exe
if errorlevel 1 exit /b 1
outputs\save-copy-safety-20261014\test_session_track_clip_trim.exe F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx outputs\save-copy-safety-20261014\test_session_track_clip_trim_roundtrip.dmx
exit /b %errorlevel%
