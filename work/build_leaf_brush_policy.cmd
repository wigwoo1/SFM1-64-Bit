@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\leaf-brush-policy-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_LEAF_POLICY work\probe_leaf_brush_policy.cpp /Fo%OUT%\native.obj /Fe%OUT%\leaf_policy_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_leaf_brush_policy.cpp /Fo%OUT%\owned.obj /Fe%OUT%\leaf_policy_owned_x64.exe > %OUT%\owned-build.log 2>&1
exit /b %errorlevel%
