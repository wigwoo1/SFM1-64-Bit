@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "TMP=%CD%\work"
set "TEMP=%CD%\work"
cl /nologo /std:c++17 /EHsc /W4 work\test_element_owner_registration.cpp /Fowork\test_element_owner_registration.obj /Fework\test_element_owner_registration.exe
if errorlevel 1 exit /b 1
work\test_element_owner_registration.exe
