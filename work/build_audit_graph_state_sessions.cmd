@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set TMP=%CD%\work
set TEMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 work\audit_graph_state_sessions.cpp /Fework\audit_graph_state_sessions.exe /Fowork\audit_graph_state_sessions.obj
if errorlevel 1 exit /b 1
work\audit_graph_state_sessions.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\elements\sessions\11.dmx"

