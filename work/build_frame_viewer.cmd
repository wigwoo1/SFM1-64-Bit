@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\sfm64_frame_viewer_x64.cpp /Fo:build\session-core\sfm64_frame_viewer_x64.obj /Fe:build\session-core\sfm64_frame_viewer_x64.exe user32.lib gdi32.lib
exit /b %errorlevel%
