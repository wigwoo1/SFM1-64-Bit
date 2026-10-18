@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_restore_constants_viewport_tail.cpp /Fowork\test_restore_constants_viewport_tail_x86.obj /Fework\sfm_multi_callback32.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_multi_callback32.exe
if not "%errorlevel%"=="0" exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_restore_constants_viewport_tail.cpp /Fowork\test_restore_constants_viewport_tail_x64.obj /Fework\sfm_multi_callback64.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_multi_callback64.exe
if not "%errorlevel%"=="0" exit /b 1

