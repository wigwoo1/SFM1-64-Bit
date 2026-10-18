@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_engine_tick_accumulator.cpp /Fo:build\session-core\test_engine_tick_accumulator.obj /Fe:build\session-core\test_engine_tick_accumulator.exe
exit /b %errorlevel%
