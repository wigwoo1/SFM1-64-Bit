@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\inspect_alternate_render_inputs.cpp /Fo:build\session-core\inspect_alternate_render_inputs.obj /Fe:build\session-core\inspect_alternate_render_inputs.exe
exit /b %errorlevel%
