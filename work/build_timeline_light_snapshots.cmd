@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_timeline_light_snapshots.cpp /Fobuild\session-core\test_timeline_light_snapshots.obj /Febuild\session-core\test_timeline_light_snapshots.exe
if errorlevel 1 exit /b 1
build\session-core\test_timeline_light_snapshots.exe > outputs\timeline-light-snapshot-validation.txt
exit /b %errorlevel%
