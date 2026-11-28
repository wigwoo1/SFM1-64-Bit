@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%~1"
if not defined OUT exit /b 2
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_ENGINE_COLLIDEABLE_CLIP_DISPATCH work\probe_engine_collideable_clip_dispatch.cpp /Fo"%OUT%\engine_collideable_clip_dispatch_x86.obj" /Fe"%OUT%\engine_collideable_clip_dispatch_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" > "%OUT%\x86-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_engine_collideable_clip_dispatch.cpp /Fo"%OUT%\engine_collideable_clip_dispatch_x64.obj" /Fe"%OUT%\engine_collideable_clip_dispatch_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" > "%OUT%\x64-build.log" 2>&1
if errorlevel 1 exit /b 1
echo PASS collideable clip dispatch builds
