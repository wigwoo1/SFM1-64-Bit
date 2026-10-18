@echo off
setlocal
set inputDirectory=work\shot-render-sampled-870833-v1
if not "%~1"=="" set "inputDirectory=%~1"
set "SFM64_DIAGNOSTIC_CAMERA_MATRIX=%inputDirectory%\camera.bin"
set "SFM64_DIAGNOSTIC_MODEL_MATRIX=%inputDirectory%\model-0-placement.bin"
set "SFM64_DIAGNOSTIC_BONE_POSE=%inputDirectory%\model-0-pose.bin"
set SFM64_DIAGNOSTIC_BLEND_TEST=
build\session-core\probe_vulkan_graphics_x64.exe D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\mamagen\eyes.vtf
exit /b %errorlevel%
