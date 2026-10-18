@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_device_reset.cpp /Fowork\test_device_reset_x86.obj /Fework\test_device_reset_x86.exe
if errorlevel 1 exit /b 1
work\test_device_reset_x86.exe
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_device_reset.cpp /Fowork\test_device_reset_x64.obj /Fework\test_device_reset_x64.exe
if errorlevel 1 exit /b 1
work\test_device_reset_x64.exe
