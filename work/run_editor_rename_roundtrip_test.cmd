@echo off
setlocal
cd /d "%~dp0.."
set SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0
set SFM64_DIAGNOSTIC_PRESENT_DEVICE=0
set SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST=1
set SFM64_DIAGNOSTIC_ELEMENT_RENAME_TEST=1
set SFM64_DIAGNOSTIC_ELEMENT_RENAME_TARGET=2552
set SFM64_DIAGNOSTIC_ELEMENT_RENAME_VALUE=prototype_rename_test
set SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=work\editor-rename-roundtrip-test.dmx
set SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=outputs
build\session-core\probe_vulkan_win32_surface_x64.exe --session work\roundtrip-11.dmx --content-directory F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod --start-frame 5225 --control-test > outputs\editor-rename-roundtrip-test.log 2>&1
exit /b %errorlevel%
