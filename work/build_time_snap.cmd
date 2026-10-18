@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_time_snap.cpp /Fowork\test_time_snap.obj /Fework\test_time_snap.exe
if errorlevel 1 exit /b 1
work\test_time_snap.exe > outputs\sfm64-time-snap-validation.txt
exit /b %errorlevel%
