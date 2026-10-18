@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 work\test_animation_set_projection_group.cpp /Fowork\test_animation_set_projection_group.obj /Fework\test_animation_set_projection_group.exe
if errorlevel 1 exit /b 1
work\test_animation_set_projection_group.exe
