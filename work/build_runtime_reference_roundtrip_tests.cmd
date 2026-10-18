@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_runtime_reference_roundtrip.cpp /Fowork\test_runtime_reference_roundtrip.obj /Fework\test_runtime_reference_roundtrip.exe
if errorlevel 1 exit /b 1
