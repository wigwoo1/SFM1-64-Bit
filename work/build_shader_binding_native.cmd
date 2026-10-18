@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_shader_binding_native.cpp /Fowork\test_shader_binding_native_x86.obj /Fework\test_shader_binding_native_x86.exe
if not "%errorlevel%"=="0" exit /b 1
work\test_shader_binding_native_x86.exe
if not "%errorlevel%"=="0" exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_shader_binding_native.cpp /Fowork\test_shader_binding_native_x64.obj /Fework\test_shader_binding_native_x64.exe
if not "%errorlevel%"=="0" exit /b 1
work\test_shader_binding_native_x64.exe









if not "%errorlevel%"=="0" exit /b 1


