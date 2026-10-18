@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\test_obj_scene_bridge.cpp /Fo:build\session-core\test_obj_scene_bridge.obj /Fe:build\session-core\test_obj_scene_bridge.exe
if errorlevel 1 exit /b 1
build\session-core\test_obj_scene_bridge.exe

