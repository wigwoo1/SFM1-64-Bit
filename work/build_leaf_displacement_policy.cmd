@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\leaf-displacement-policy-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_LEAF_DISP_POLICY work\probe_leaf_displacement_policy.cpp /Fo%OUT%\native.obj /Fe%OUT%\policy_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_leaf_displacement_policy.cpp /Fo%OUT%\owned.obj /Fe%OUT%\policy_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_leaf_displacement_policy.cpp /Fo%OUT%\safety.obj /Fe%OUT%\policy_safety_x64.exe > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
%OUT%\policy_safety_x64.exe > %OUT%\safety-test.log 2>&1
exit /b %errorlevel%
