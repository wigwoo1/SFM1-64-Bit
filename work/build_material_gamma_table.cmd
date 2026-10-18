@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work\compiler-temp
set TMP=%TEMP%
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_material_gamma_table.cpp /Fo:build\session-core\material_gamma_table.obj /Fe:build\session-core\material_gamma_table.exe
if errorlevel 1 exit /b 1
build\session-core\material_gamma_table.exe
