@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_native_cpu_math_startup.cpp /Fowork\native_cpu_math_startup.obj /Fework\sfm_native_cpu_math_startup.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_native_cpu_math_startup.exe
exit /b %errorlevel%


