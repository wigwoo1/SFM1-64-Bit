@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_native_pair_query_recreation.cpp /Fowork\test_native_pair_query_recreation.obj /Fework\test_native_pair_query_recreation.exe
if errorlevel 1 exit /b 1
work\test_native_pair_query_recreation.exe


