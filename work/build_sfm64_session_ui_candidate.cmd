@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_vulkan_win32_surface_x64.cpp /Fo:build\session-core\sfm64_session_ui_candidate.obj /Fe:build\session-core\sfm64_session_ui_candidate.exe user32.lib gdi32.lib comdlg32.lib comctl32.lib
exit /b %errorlevel%
