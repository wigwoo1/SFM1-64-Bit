@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist outputs\unlit-two-texture-gpu-20261014 mkdir outputs\unlit-two-texture-gpu-20261014
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_unlit_two_texture_d3d9.cpp /Fooutputs\unlit-two-texture-gpu-20261014\probe_unlit_two_texture_d3d9.obj /Feoutputs\unlit-two-texture-gpu-20261014\probe_unlit_two_texture_d3d9.exe /link d3d9.lib user32.lib
exit /b %errorlevel%
