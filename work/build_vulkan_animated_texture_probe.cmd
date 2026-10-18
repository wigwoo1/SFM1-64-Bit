@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_vulkan_animated_texture_x64.cpp /Fobuild\session-core\probe_vulkan_animated_texture_x64.obj /Febuild\session-core\probe_vulkan_animated_texture_x64.exe user32.lib gdi32.lib comdlg32.lib comctl32.lib
exit /b %errorlevel%
