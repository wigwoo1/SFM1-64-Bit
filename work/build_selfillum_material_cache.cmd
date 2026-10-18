@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work\compiler-temp
set TMP=%TEMP%
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_selfillum_material_cache.cpp /Fo:build\session-core\selfillum_material_cache.obj /Fe:build\session-core\selfillum_material_cache.exe
if errorlevel 1 exit /b 1
build\session-core\selfillum_material_cache.exe
