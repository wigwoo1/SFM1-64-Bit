@echo off
setlocal
call work\build_editor_hierarchy_rows_test.cmd
if errorlevel 1 exit /b 1
call work\build_benchmark_editor_hierarchy_mtt.cmd
if errorlevel 1 exit /b 1
work\benchmark_editor_hierarchy_mtt.exe F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx
if errorlevel 1 exit /b 1
call work\build_sfm64_session_ui.cmd
exit /b %errorlevel%
