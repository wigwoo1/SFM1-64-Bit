@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_pair_query_recreation_candidate.cpp /Fowork\test_pair_query_recreation_candidate.obj /Fework\test_pair_query_recreation_candidate.exe
if errorlevel 1 exit /b 1
work\test_pair_query_recreation_candidate.exe



