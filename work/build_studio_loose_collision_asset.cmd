@echo off
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fooutputs\studio-vcollide-read-20261016\sincos.obj work\x64_fsin_cos.asm > outputs\studio-vcollide-read-20261016\asm-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_studio_loose_collision_asset.cpp outputs\studio-vcollide-read-20261016\sincos.obj /Fooutputs\studio-vcollide-read-20261016\owned.obj /Feoutputs\studio-vcollide-read-20261016\studio_collision_asset_owned_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > outputs\studio-vcollide-read-20261016\owned-build.log 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_studio_loose_collision_asset.cpp outputs\studio-vcollide-read-20261016\sincos.obj /Fooutputs\studio-vcollide-read-20261016\safety.obj /Feoutputs\studio-vcollide-read-20261016\studio_collision_asset_safety_x64.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > outputs\studio-vcollide-read-20261016\safety-build.log 2>&1
exit /b %errorlevel%
