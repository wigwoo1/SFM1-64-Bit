@echo off
setlocal
cd /d "%~dp0.."
set SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0
set SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=outputs
build\session-core\sfm64_session_ui.exe --session "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx" --content-directory "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod" --start-frame 0 > outputs\mtt-soldier-editor-session-launch.log 2>&1
exit /b %errorlevel%
