@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_hierarchy_scale.cpp /Fowork\test_hierarchy_scale.obj /Fework\test_hierarchy_scale.exe
if errorlevel 1 exit /b 1
work\test_hierarchy_scale.exe
