@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_deferred_light_removal.cpp /Fowork\test_deferred_light_removal.obj /Fework\test_deferred_light_removal.exe
if errorlevel 1 exit /b 1
work\test_deferred_light_removal.exe > outputs\sfm64-deferred-light-removal-validation.txt
exit /b %errorlevel%
