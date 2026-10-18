@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_actual_power_gamma_startup.cpp /Fowork\actual_power_gamma_startup_native.obj /Fework\sfm_actual_power_gamma_startup_native.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_actual_power_gamma_startup_native.exe
exit /b %errorlevel%
