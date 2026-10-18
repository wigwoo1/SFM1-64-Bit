@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Od /MT work\diagnose_dispatch_headers.cpp /Fowork\diagnose_dispatch_headers.obj /Fework\diagnose_dispatch_headers.exe
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Od /MT /DRESTORE_DISPATCH_DIAGNOSTIC_NO_ASM work\diagnose_restore_dispatch_startup.cpp /Fowork\diagnose_dispatch_noasm.obj /Fework\diagnose_dispatch_noasm.exe
if not "%errorlevel%"=="0" exit /b 1
