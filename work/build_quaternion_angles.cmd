@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /arch:SSE2 work\probe_quaternion_angles.cpp /Fowork\probe_quaternion_angles_x86.obj /Fework\probe_quaternion_angles_x86.exe
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\probe_quaternion_angles.cpp /Fowork\probe_quaternion_angles_x64.obj /Fework\probe_quaternion_angles_x64.exe
exit /b %errorlevel%
