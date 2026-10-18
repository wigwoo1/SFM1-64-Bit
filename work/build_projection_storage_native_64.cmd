@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\probe_engine_projection_storage.cpp /Fowork\probe_projection_storage_64.obj /Fework\probe_projection_storage_64.exe
exit /b %errorlevel%
