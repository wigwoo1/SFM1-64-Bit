@echo off
setlocal
set SFM64_DIAGNOSTIC_CAMERA_MATRIX=work\shot-render-2470-v1\camera.bin
set SFM64_DIAGNOSTIC_MODEL_MATRIX=work\shot-render-2470-v1\model-0-placement.bin
set SFM64_DIAGNOSTIC_BONE_POSE=work\shot-render-2470-v1\model-0-pose.bin
build\session-core\probe_vulkan_graphics_x64.exe D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\mamagen\eyes.vtf
exit /b %errorlevel%
