@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Od /MT work\test_restore_dispatch_native.cpp /Fowork\restore_dispatch_diagnostic.obj /Fework\restore_dispatch_diagnostic.exe /link /DYNAMICBASE:NO
if not "%errorlevel%"=="0" exit /b 1
