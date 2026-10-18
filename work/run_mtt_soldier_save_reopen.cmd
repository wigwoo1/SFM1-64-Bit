@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\sfm64-editor-x64"
set "RUN_DIR=%OUT%\mtt-editor-roundtrip-%RANDOM%-%RANDOM%"
if exist "%RUN_DIR%" exit /b 1
mkdir "%RUN_DIR%"
set "INPUT=%RUN_DIR%\mtt-soldier-input.dmx"
set "SAVED=%RUN_DIR%\mtt-soldier-edited-copy.dmx"
copy /Y "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx" "%INPUT%" >nul
if errorlevel 1 exit /b 1

set "SFM64_DIAGNOSTIC_STARTUP_TRACE=1"
set "SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0"
set "SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST=1"
set "SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=%SAVED%"
set "SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=%RUN_DIR%\edit-output"
"%OUT%\sfm64_session_ui.exe" --control-test --session "%INPUT%" --content-directory "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod" --start-frame 118 > "%RUN_DIR%\edit-save.log" 2>&1
if errorlevel 1 exit /b 1
findstr /C:"editor_session_saved=" "%RUN_DIR%\edit-save.log" >nul
if errorlevel 1 exit /b 1
if not exist "%SAVED%" exit /b 1
fc /b "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx" "%INPUT%" >nul
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_mtt_soldier_editor_roundtrip.cpp /Fo:"%OUT%\test_mtt_soldier_editor_roundtrip.obj" /Fe:"%OUT%\test_mtt_soldier_editor_roundtrip.exe" > "%RUN_DIR%\compare-build.log" 2>&1
if errorlevel 1 exit /b 1
"%OUT%\test_mtt_soldier_editor_roundtrip.exe" "%INPUT%" "%SAVED%" "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\gameinfo.txt" > "%RUN_DIR%\compare.log" 2>&1
if errorlevel 1 exit /b 1

set "SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST="
set "SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH="
set "SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=%RUN_DIR%\reopen-output"
"%OUT%\sfm64_session_ui.exe" --control-test --session "%SAVED%" --content-directory "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod" --start-frame 119 > "%RUN_DIR%\reopen.log" 2>&1
if errorlevel 1 exit /b 1
findstr /C:"session_argument_loaded=1" "%RUN_DIR%\reopen.log" >nul
if errorlevel 1 exit /b 1
rem Three bound models plus the BSP world; other-shot camera guides are excluded.
findstr /C:"frozen_scene_models=4" "%RUN_DIR%\reopen.log" >nul
if errorlevel 1 exit /b 1
findstr /C:"retained Vulkan sequence passed frames=5" "%RUN_DIR%\reopen.log" >nul
if errorlevel 1 exit /b 1
echo mtt_soldier_edit_save_reopen_passed
echo Results: %RUN_DIR%
exit /b 0
