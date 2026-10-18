@echo off
setlocal
set SFM64_DIAGNOSTIC_BONE_POSE=
set SFM64_DIAGNOSTIC_MODEL_MATRIX=
set SFM64_DIAGNOSTIC_CAMERA_MATRIX=
set SFM64_DIAGNOSTIC_BLEND_TEST=
set SFM64_DIAGNOSTIC_FRAME_SEQUENCE=work\retained-shot-sequence-v1
set SFM64_DIAGNOSTIC_FRAME_COUNT=3
build\session-core\probe_vulkan_graphics_x64.exe D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\mamagen\eyes.vtf
exit /b %errorlevel%
