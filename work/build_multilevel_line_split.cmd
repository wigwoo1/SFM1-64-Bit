@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\multilevel-line-split-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DEXTENDED_LINE_SPLIT /DMULTILEVEL_LINE_SPLIT /DNATIVE_LINE_SPLIT work\probe_line_plane_split.cpp /Fo%OUT%\native.obj /Fe%OUT%\split_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DEXTENDED_LINE_SPLIT /DMULTILEVEL_LINE_SPLIT work\probe_line_plane_split.cpp /Fo%OUT%\owned.obj /Fe%OUT%\split_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_line_walk_safety.cpp /Fo%OUT%\safety.obj /Fe%OUT%\line_walk_safety_x64.exe > %OUT%\safety-build.log 2>&1
exit /b %errorlevel%
