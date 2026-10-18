@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fowork\x64_fsin_cos.obj work\x64_fsin_cos.asm
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\audit_light_hierarchy.cpp /Fowork\audit_light_hierarchy.obj /Fework\audit_light_hierarchy.exe work\x64_fsin_cos.obj
if errorlevel 1 exit /b 1
work\audit_light_hierarchy.exe > outputs\sfm64-session-light-hierarchy-audit.txt
exit /b %errorlevel%
