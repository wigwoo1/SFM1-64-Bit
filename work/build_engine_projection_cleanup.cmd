@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_engine_projection_cleanup.cpp /Fowork\test_engine_projection_cleanup.obj /Fework\test_engine_projection_cleanup.exe
if errorlevel 1 exit /b 1
work\test_engine_projection_cleanup.exe
