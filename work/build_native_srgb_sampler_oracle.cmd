@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work\compiler-temp
set TMP=%TEMP%
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_native_srgb_sampler_oracle.cpp /Fo:build\session-core\native_srgb_sampler_oracle.obj /Fe:build\session-core\native_srgb_sampler_oracle.exe
if errorlevel 1 exit /b 1
build\session-core\native_srgb_sampler_oracle.exe
