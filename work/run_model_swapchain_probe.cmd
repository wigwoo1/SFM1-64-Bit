@echo off
setlocal
if not "%~1"=="" set "SFM64_DIAGNOSTIC_PRESENT_DEVICE=%~1"
set SFM64_DIAGNOSTIC_PRESENT_MODEL=D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen
set SFM64_DIAGNOSTIC_FRAME_SEQUENCE=work\retained-shot-sequence-v1
set SFM64_DIAGNOSTIC_FRAME_COUNT=3
if not "%~2"=="" set "SFM64_DIAGNOSTIC_FRAME_COUNT=%~2"
if not "%~3"=="" set "SFM64_DIAGNOSTIC_FRAME_SEQUENCE=%~3"
set SFM64_DIAGNOSTIC_BONE_POSE=
set SFM64_DIAGNOSTIC_MODEL_MATRIX=
set SFM64_DIAGNOSTIC_CAMERA_MATRIX=
set SFM64_DIAGNOSTIC_BLEND_TEST=
build\session-core\probe_vulkan_win32_surface_x64.exe
exit /b %errorlevel%
