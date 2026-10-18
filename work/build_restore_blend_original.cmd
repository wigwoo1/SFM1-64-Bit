@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /Od /MT work\test_restore_blend_original.cpp /Fowork\test_restore_blend_original.obj /Fework\sfm_blend_original.exe
if not "%errorlevel%"=="0" exit /b 1

work\sfm_blend_original.exe
if not "%errorlevel%"=="0" exit /b 1

