@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\audit_session_light_fields.cpp /Fowork\audit_session_light_fields.obj /Fework\audit_session_light_fields.exe
if errorlevel 1 exit /b 1
work\audit_session_light_fields.exe > outputs\sfm64-session-light-field-audit.txt
exit /b %errorlevel%
