@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_vcs6_shader_cache_index.cpp /Fowork\test_vcs6_shader_cache_index.obj /Fework\test_vcs6_shader_cache_index.exe
exit /b %errorlevel%
