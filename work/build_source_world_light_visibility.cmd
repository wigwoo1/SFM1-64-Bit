@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\world-light-visibility-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_SOURCE_WORLD_LIGHT_VISIBILITY work\probe_source_world_light_visibility.cpp /Fo%OUT%\native.obj /Fe%OUT%\visibility_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_source_world_light_visibility.cpp /Fo%OUT%\owned.obj /Fe%OUT%\visibility_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_source_world_light_visibility.cpp /Fo%OUT%\safety.obj /Fe%OUT%\visibility_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_SOURCE_WORLD_LIGHT_VISIBILITY /DWORLD_LIGHT_QUERY_OVERFLOW_COMPOSITE work\probe_source_world_light_visibility.cpp /Fo%OUT%\complete-native.obj /Fe%OUT%\complete_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\complete-native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DWORLD_LIGHT_QUERY_OVERFLOW_COMPOSITE work\probe_source_world_light_visibility.cpp /Fo%OUT%\complete-owned.obj /Fe%OUT%\complete_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\complete-owned-build.log 2>&1
exit /b %errorlevel%
