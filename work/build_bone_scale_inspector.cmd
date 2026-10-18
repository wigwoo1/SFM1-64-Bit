@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\inspect_session_bone_scales.cpp /Fo:build\session-core\inspect_session_bone_scales.obj /Fe:build\session-core\inspect_session_bone_scales.exe
exit /b %errorlevel%
