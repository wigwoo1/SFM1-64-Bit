@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1

set "OUT=%CD%\outputs\sfm64-editor-x64"
set "SFM64_DIAGNOSTIC_STARTUP_TRACE=1"
set "SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0"
set "SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=%OUT%\mtt-soldier-smoke-output"
if not exist "%SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY%" mkdir "%SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY%"

"%OUT%\sfm64_session_ui.exe" --control-test --session "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx" --content-directory "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod" --start-frame 119 > "%OUT%\mtt-soldier-runtime-smoke.log" 2>&1
exit /b %errorlevel%
