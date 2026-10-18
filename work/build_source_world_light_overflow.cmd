@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\world-light-overflow-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_SOURCE_WORLD_LIGHT_OVERFLOW work\probe_source_world_light_overflow.cpp /Fo%OUT%\native.obj /Fe%OUT%\overflow_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_source_world_light_overflow.cpp /Fo%OUT%\owned.obj /Fe%OUT%\overflow_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_source_world_light_overflow.cpp /Fo%OUT%\safety.obj /Fe%OUT%\overflow_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_SOURCE_MODEL_CACHED_LIGHTING /DWORLD_LIGHT_OVERFLOW_COMPOSITE work\probe_source_model_cached_lighting.cpp /Fo%OUT%\composite-native.obj /Fe%OUT%\cached_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\composite-native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DWORLD_LIGHT_OVERFLOW_COMPOSITE work\probe_source_model_cached_lighting.cpp /Fo%OUT%\composite-owned.obj /Fe%OUT%\cached_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\composite-owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_source_model_cached_lighting.cpp /Fo%OUT%\composite-safety.obj /Fe%OUT%\cached_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\composite-safety-build.log 2>&1
exit /b %errorlevel%
