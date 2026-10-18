@echo off
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\build\temp
set TMP=%CD%\build\temp
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_viewport_camera_guide_geometry.cpp /Febuild\session-core\test_viewport_camera_guide_geometry.exe
if errorlevel 1 exit /b 1
build\session-core\test_viewport_camera_guide_geometry.exe
