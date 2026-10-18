@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_studio_decal_queued_owner.cpp /Fowork\test_studio_decal_queued_owner.obj /Fework\test_studio_decal_queued_owner.exe
if errorlevel 1 exit /b 1
work\test_studio_decal_queued_owner.exe
