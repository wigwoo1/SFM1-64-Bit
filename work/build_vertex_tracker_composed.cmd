@echo off
call work\build_vertex_tracker_composed_native.cmd
if not "%errorlevel%"=="0" exit /b 1
call work\build_vertex_tracker_composed_compare.cmd
if not "%errorlevel%"=="0" exit /b 1
