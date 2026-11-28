@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%~1"
if not defined OUT exit /b 2
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_PHY_HULL_CONTENTS work\probe_phy_hull_contents.cpp /Fo"%OUT%\phy_hull_contents_x86.obj" /Fe"%OUT%\phy_hull_contents_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" > "%OUT%\x86-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
ml64 /nologo /c /Fo"%OUT%\sincos.obj" work\x64_fsin_cos.asm > "%OUT%\asm-build.log" 2>&1
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_phy_hull_contents.cpp "%OUT%\sincos.obj" /Fo"%OUT%\phy_hull_contents_x64.obj" /Fe"%OUT%\phy_hull_contents_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" > "%OUT%\x64-build.log" 2>&1
if errorlevel 1 exit /b 1
echo PASS composed PHY hull contents builds
