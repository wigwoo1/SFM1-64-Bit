@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1

set "OUT=%CD%\outputs\sfm64-editor-x64"
set "SESSION=%OUT%\runtime-session-input.dmx"
set SFM64_DIAGNOSTIC_STARTUP_TRACE=1
set SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0
set SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=%OUT%\runtime-smoke-output
if not exist "%SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY%" mkdir "%SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY%"
copy /Y "work\roundtrip-11.dmx" "%SESSION%" >nul
if errorlevel 1 exit /b 1

"%OUT%\sfm64_session_ui.exe" --control-test --session "%SESSION%" --content-directory "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod" --start-frame 0 > "%OUT%\runtime-session-smoke.log" 2>&1
exit /b %errorlevel%
