@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_vulkan_material_descriptor_counts.cpp /Fowork\test_vulkan_material_descriptor_counts.obj /Fework\test_vulkan_material_descriptor_counts.exe
if errorlevel 1 exit /b 1
work\test_vulkan_material_descriptor_counts.exe
exit /b %errorlevel%
