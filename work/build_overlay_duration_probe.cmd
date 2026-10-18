@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TMP=%CD%\work
set TEMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /fp:strict work\probe_overlay_duration_division.cpp /Fowork\probe_overlay_duration_division.obj /Febuild\session-core\probe_overlay_duration_division_x64.exe
