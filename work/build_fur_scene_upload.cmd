@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\test_fur_scene_upload.cpp /Fowork\test_fur_scene_upload.obj /Fework\test_fur_scene_upload.exe
if errorlevel 1 exit /b 1
work\test_fur_scene_upload.exe
