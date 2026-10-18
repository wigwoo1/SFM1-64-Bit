@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\ifm-draw-packet-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_DRAW_PACKET_IFM work\probe_ifm_draw_packet.cpp /Fo%OUT%\ifm.obj /Fe%OUT%\packet_ifm_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\ifm-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_DRAW_PACKET_ENGINE work\probe_ifm_draw_packet.cpp /Fo%OUT%\engine.obj /Fe%OUT%\packet_engine_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\engine-build.log 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo%OUT%\sincos.obj work\x64_fsin_cos.asm > %OUT%\asm-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_draw_packet.cpp %OUT%\sincos.obj /Fo%OUT%\owned.obj /Fe%OUT%\packet_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_ifm_draw_packet.cpp %OUT%\sincos.obj /Fo%OUT%\safety.obj /Fe%OUT%\packet_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\safety-build.log 2>&1
exit /b %errorlevel%
