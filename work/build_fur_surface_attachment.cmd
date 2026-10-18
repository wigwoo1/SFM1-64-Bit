@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_fur_surface_attachment.cpp /Fowork\test_fur_surface_attachment.obj /Fework\test_fur_surface_attachment.exe
if errorlevel 1 exit /b 1
work\test_fur_surface_attachment.exe
