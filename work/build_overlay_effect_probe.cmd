@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TMP=%CD%\work
set TEMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 work\probe_overlay_effect_state.cpp /Fowork\probe_overlay_effect_state.obj /Febuild\session-core\probe_overlay_effect_state_x64.exe
