@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set TMP=%CD%\work
set TEMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 work\test_session_graph_state_lookup.cpp /Fework\test_session_graph_state_lookup.exe /Fowork\test_session_graph_state_lookup.obj
if errorlevel 1 exit /b 1
work\test_session_graph_state_lookup.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\elements\sessions\11.dmx" "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\elements\sessions\139.dmx" "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\elements\sessions\360.dmx"
