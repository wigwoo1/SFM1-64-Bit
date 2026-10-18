@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_pixel_color_grid_x64.cpp /Fowork\test_pixel_color_grid_x64.obj /Fework\test_pixel_color_grid_x64.exe
if errorlevel 1 exit /b 1
work\test_pixel_color_grid_x64.exe
