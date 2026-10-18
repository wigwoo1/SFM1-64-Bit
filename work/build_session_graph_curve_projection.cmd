@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_session_graph_curve_projection.cpp /Fowork\test_session_graph_curve_projection.obj /Fework\test_session_graph_curve_projection.exe
if errorlevel 1 exit /b 1
work\test_session_graph_curve_projection.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx"
if errorlevel 1 exit /b 1
