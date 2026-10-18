@echo off
setlocal
cd /d "%~dp0.."
set SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0
set SFM64_DIAGNOSTIC_PRESENT_DEVICE=0
set SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod
set SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=work\roundtrip-11.dmx
set SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=870833
set SFM64_DIAGNOSTIC_TIMELINE_BLANKS=1
set SFM64_DIAGNOSTIC_SESSION_TIMELINE=1
set SFM64_DIAGNOSTIC_START_FRAME=5225
set SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST=1
set SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=work\editor-save-roundtrip-test.dmx
set SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=outputs
build\session-core\probe_vulkan_win32_surface_x64.exe --control-test > outputs\editor-save-roundtrip-test.log 2>&1
exit /b %errorlevel%
