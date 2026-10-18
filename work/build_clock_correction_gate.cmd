@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_clock_correction_gate.cpp /Fo:build\session-core\test_clock_correction_gate.obj /Fe:build\session-core\test_clock_correction_gate.exe
if errorlevel 1 exit /b 1
build\session-core\test_clock_correction_gate.exe > outputs\sfm64-clock-correction-gate-validation.txt
exit /b %errorlevel%
