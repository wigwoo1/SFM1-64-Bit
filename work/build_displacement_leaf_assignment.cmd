@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\displacement-leaf-assignment-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DDISPLACEMENT_LEAF_ASSIGNMENT /DNATIVE_STATIONARY_LEAVES work\probe_stationary_leaf_selection.cpp /Fo%OUT%\native.obj /Fe%OUT%\selection_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DDISPLACEMENT_LEAF_ASSIGNMENT work\probe_stationary_leaf_selection.cpp /Fo%OUT%\owned.obj /Fe%OUT%\selection_owned_x64.exe > %OUT%\owned-build.log 2>&1
exit /b %errorlevel%
