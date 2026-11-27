@echo off
setlocal
cd /d "%~dp0.."
set "OUT=%SFM64_IFM_PLAYER_VIEW_TEST_DIRECTORY%"
if not defined OUT set "OUT=outputs\ifm-player-view"
if not exist "%OUT%" mkdir "%OUT%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DNATIVE_IFM_PLAYER_VIEW work\probe_ifm_player_view.cpp /Fo"%OUT%\ifm_view_x86.obj" /Fe"%OUT%\ifm_view_x86.exe" > "%OUT%\ifm_view_x86-build.log" 2>&1
if errorlevel 1 exit /b 1

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_ifm_player_view.cpp /Fo"%OUT%\ifm_view_x64.obj" /Fe"%OUT%\ifm_view_x64.exe" > "%OUT%\ifm_view_x64-build.log" 2>&1
exit /b %ERRORLEVEL%
