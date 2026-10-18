@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_session_render_light_snapshot.cpp /Fobuild\session-core\test_session_render_light_snapshot.obj /Febuild\session-core\test_session_render_light_snapshot.exe
if errorlevel 1 exit /b 1
build\session-core\test_session_render_light_snapshot.exe
