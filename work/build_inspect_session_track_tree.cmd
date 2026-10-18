@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\inspect_session_track_tree.cpp /Fework\inspect_session_track_tree.exe
if errorlevel 1 exit /b 1
work\inspect_session_track_tree.exe F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx > outputs\mtt-soldier-track-hierarchy.txt
exit /b %errorlevel%
