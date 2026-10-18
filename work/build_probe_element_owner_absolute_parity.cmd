@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
set "TMP=%CD%\work"
set "TEMP=%CD%\work"
cl /nologo /std:c++17 /EHsc /W4 /DABSOLUTE_OWNER_PATH work\probe_element_owner_path_parity.cpp /Fowork\probe_owner_absolute_x86.obj /Fework\probe_owner_absolute_x86.exe
if errorlevel 1 exit /b 1
work\probe_owner_absolute_x86.exe outputs\element-owner-absolute-original-oracle.bin
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
cl /nologo /std:c++17 /EHsc /W4 /DABSOLUTE_OWNER_PATH work\probe_element_owner_path_parity.cpp /Fowork\probe_owner_absolute_x64.obj /Fework\probe_owner_absolute_x64.exe
if errorlevel 1 exit /b 1
work\probe_owner_absolute_x64.exe outputs\element-owner-absolute-original-oracle.bin
