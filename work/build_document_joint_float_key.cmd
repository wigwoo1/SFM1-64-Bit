@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_document_joint_float_key.cpp /Fowork\test_document_joint_float_key.obj /Fework\test_document_joint_float_key.exe
if errorlevel 1 exit /b 1
for %%F in (11 139) do (
 work\test_document_joint_float_key.exe work\roundtrip-%%F.dmx
 if errorlevel 1 exit /b 1
)
