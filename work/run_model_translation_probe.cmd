@echo off
setlocal
set SFM64_DIAGNOSTIC_CAMERA_MATRIX=
set SFM64_DIAGNOSTIC_MODEL_MATRIX=work\diagnostic-model-translation-v1.bin
build\session-core\probe_vulkan_graphics_x64.exe D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\mamagen\eyes.vtf
exit /b %errorlevel%
