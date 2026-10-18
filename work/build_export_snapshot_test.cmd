@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_export_snapshot.cpp /Fe:work\test_export_snapshot.exe /Fo:work\test_export_snapshot.obj
if errorlevel 1 exit /b 1
work\test_export_snapshot.exe
