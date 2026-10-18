@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /arch:SSE2 work\probe_angle_basis.cpp /Fowork\probe_angle_basis_x86.obj /Fework\probe_angle_basis_x86.exe
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fowork\x64_fsin_cos.obj work\x64_fsin_cos.asm
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\probe_angle_basis.cpp /Fowork\probe_angle_basis_x64.obj /Fework\probe_angle_basis_x64.exe work\x64_fsin_cos.obj
exit /b %errorlevel%
