@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\engine-studio-point-clip-20261016"
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%"\default-regression mkdir "%OUT%"\default-regression
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_ENGINE_STUDIO_POINT_CLIP work\probe_engine_studio_point_clip.cpp /Fo%OUT%\native.obj /Fe%OUT%\studio_clip_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PHY_POINT_TRACE work\probe_phy_point_trace.cpp /Fo%OUT%\default-regression\native.obj /Fe%OUT%\default-regression\point_trace_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\default-regression\native-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo%OUT%\sincos.obj work\x64_fsin_cos.asm > %OUT%\asm-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_engine_studio_point_clip.cpp %OUT%\sincos.obj /Fo%OUT%\owned.obj /Fe%OUT%\studio_clip_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_engine_studio_point_clip.cpp %OUT%\sincos.obj /Fo%OUT%\safety.obj /Fe%OUT%\studio_clip_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_phy_point_trace.cpp %OUT%\sincos.obj /Fo%OUT%\default-regression\owned.obj /Fe%OUT%\default-regression\point_trace_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\default-regression\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_phy_point_trace.cpp %OUT%\sincos.obj /Fo%OUT%\default-regression\safety.obj /Fe%OUT%\default-regression\point_trace_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\default-regression\safety-build.log 2>&1
exit /b %errorlevel%
