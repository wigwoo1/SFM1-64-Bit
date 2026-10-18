@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_iris_projection.cpp /Fo:build\session-core\test_iris_projection.obj /Fe:build\session-core\test_iris_projection.exe
if errorlevel 1 exit /b 1
build\session-core\test_iris_projection.exe
