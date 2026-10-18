@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\probe_engine_model_render_resource_cleanup.cpp /Fowork\probe_model_render_resource_cleanup_64.obj /Fework\probe_model_render_resource_cleanup_64.exe
exit /b %errorlevel%
