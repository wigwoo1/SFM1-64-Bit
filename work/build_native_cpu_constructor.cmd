@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_native_cpu_constructor.cpp /Fowork\probe_native_cpu_constructor.obj /Fework\probe_native_cpu_constructor.exe
if errorlevel 1 exit /b 1
work\probe_native_cpu_constructor.exe
