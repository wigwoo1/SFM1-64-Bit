@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /DTEST_PARALLEL_EXPRESSIONS work\test_prepared_timeline_pass_x64.cpp /Fowork\test_parallel_prepared_expressions.obj /Fework\test_parallel_prepared_expressions.exe
if errorlevel 1 exit /b 1
work\test_parallel_prepared_expressions.exe work\roundtrip-11.dmx
