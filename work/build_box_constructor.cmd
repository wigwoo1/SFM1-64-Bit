@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\box-constructor-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_BOX_CONSTRUCTOR work\probe_box_constructor.cpp /Fo%OUT%\native.obj /Fe%OUT%\constructor_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_box_constructor.cpp /Fo%OUT%\owned.obj /Fe%OUT%\constructor_owned_x64.exe > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_box_record_safety.cpp /Fo%OUT%\safety.obj /Fe%OUT%\box_record_safety_x64.exe > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_bsp_collision_records.cpp /Fo%OUT%\records.obj /Fe%OUT%\bsp_collision_records_x64.exe > %OUT%\records-build.log 2>&1
exit /b %errorlevel%
