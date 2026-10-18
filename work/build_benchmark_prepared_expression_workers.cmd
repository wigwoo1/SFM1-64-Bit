@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\benchmark_prepared_expression_workers.cpp /Fowork\benchmark_prepared_expression_workers.obj /Fework\benchmark_prepared_expression_workers.exe
if errorlevel 1 exit /b 1
work\benchmark_prepared_expression_workers.exe work\roundtrip-11.dmx outputs\sfm64-expression-worker-benchmark.json
