@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /c work\dependencies\lzma-decoder\LzmaDec.c /Fowork\lzma_decoder.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\probe_native_d3d9_shadow_device.cpp work\lzma_decoder.obj /Fowork\probe_native_d3d9_shadow_device.obj /Fework\probe_native_d3d9_shadow_device.exe /link d3d9.lib user32.lib
