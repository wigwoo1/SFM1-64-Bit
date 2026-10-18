@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_actual_power_gamma_startup.cpp /Fowork\actual_power_gamma_startup_compare.obj /Fework\sfm_actual_power_gamma_startup_compare.exe
if not "%errorlevel%"=="0" exit /b 1
work\sfm_actual_power_gamma_startup_compare.exe
exit /b %errorlevel%
