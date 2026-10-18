@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /c work\compile_graph_editor_keymode_state.cpp /Fowork\compile_graph_editor_keymode_state.obj
