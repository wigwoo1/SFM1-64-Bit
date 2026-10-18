@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_reference_owned_dmx_external_x64.cpp /Fo:build\session-core\test_reference_owned_dmx_external_x64.obj /Fe:build\session-core\test_reference_owned_dmx_external_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_reference_owned_dmx_projection_x64.cpp /Fo:build\session-core\test_reference_owned_dmx_projection_x64.obj /Fe:build\session-core\test_reference_owned_dmx_projection_x64.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_reference_dmx_export_x64.cpp /Fo:build\session-core\test_reference_dmx_export_x64.obj /Fe:build\session-core\test_reference_dmx_export_x64.exe
if errorlevel 1 exit /b 1
