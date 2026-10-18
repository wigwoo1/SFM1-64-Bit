@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_owner_reset_cleanup_candidate.cpp /Fowork\test_owner_reset_cleanup_candidate.obj /Fework\test_owner_reset_cleanup_candidate.exe
if errorlevel 1 exit /b 1
work\test_owner_reset_cleanup_candidate.exe
