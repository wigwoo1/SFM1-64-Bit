@echo off
call work\build_vertex_buffer_creation_native.cmd
if not "%errorlevel%"=="0" exit /b 1
call work\build_vertex_buffer_creation_compare.cmd
if not "%errorlevel%"=="0" exit /b 1
