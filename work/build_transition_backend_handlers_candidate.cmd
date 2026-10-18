@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_transition_backend_handlers_candidate.cpp /Fowork\test_transition_backend_handlers_candidate.obj /Fework\test_transition_backend_handlers_candidate.exe
if not "%errorlevel%"=="0" exit /b 1
work\test_transition_backend_handlers_candidate.exe
if not "%errorlevel%"=="0" exit /b 1


