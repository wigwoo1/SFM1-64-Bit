@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_seeded_generator.cpp /Fowork\seeded_generator_native.obj /Fework\sfm_seeded_generator_native.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_seeded_generator_native.exe
exit /b %errorlevel%
