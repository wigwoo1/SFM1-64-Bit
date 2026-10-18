@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_pending_find_or_append.cpp /Fowork\test_pending_find_or_append.obj /Fework\test_pending_find_or_append.exe
if errorlevel 1 exit /b 1
work\test_pending_find_or_append.exe work\native-pending-find-or-append-v1.bin
