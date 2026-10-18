@echo off
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_two_texture_draw_transport test_vulkan_scene_commands) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\%%T.cpp /Fooutputs\two-texture-draw-transport-20261015\%%T.obj /Feoutputs\two-texture-draw-transport-20261015\%%T.exe > outputs\two-texture-draw-transport-20261015\%%T-build.log 2>&1
 if errorlevel 1 exit /b 1
 outputs\two-texture-draw-transport-20261015\%%T.exe
 if errorlevel 1 exit /b 1
)
