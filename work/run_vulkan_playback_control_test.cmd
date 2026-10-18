@echo off
setlocal
cd /d "%~dp0.."
set SFM64_DIAGNOSTIC_PRESENT_DEVICE=0
set SFM64_DIAGNOSTIC_PRESENT_MODEL=F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen
set SFM64_DIAGNOSTIC_FRAME_SEQUENCE=work\retained-shot-sequence-v1
set SFM64_DIAGNOSTIC_FRAME_COUNT=3
set SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=outputs\vulkan-playback-controls-current
set SFM64_DIAGNOSTIC_SEEK_TEST=
set SFM64_DIAGNOSTIC_BONE_POSE=
set SFM64_DIAGNOSTIC_MODEL_MATRIX=
set SFM64_DIAGNOSTIC_CAMERA_MATRIX=
set SFM64_DIAGNOSTIC_BLEND_TEST=
build\session-core\probe_vulkan_win32_surface_x64.exe --control-test
exit /b %errorlevel%
