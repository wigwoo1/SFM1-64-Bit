@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_owned_expression_boundaries.cpp /Fowork\test_owned_expression_boundaries.obj /Fework\test_owned_expression_boundaries.exe
if errorlevel 1 exit /b 1
