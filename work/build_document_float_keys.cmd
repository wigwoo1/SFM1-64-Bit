@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_document_float_keys test_document_scalar_edit) do (
 cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\%%T.cpp /Fowork\%%T.obj /Fework\%%T.exe
 if errorlevel 1 exit /b 1
 for %%F in (11 139) do (
  work\%%T.exe work\roundtrip-%%F.dmx
  if errorlevel 1 exit /b 1
 )
)
