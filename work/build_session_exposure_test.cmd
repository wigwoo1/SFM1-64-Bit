@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 2
set "SFM64_EXPOSURE_OUT=%~f1"
if not exist "%SFM64_EXPOSURE_OUT%" mkdir "%SFM64_EXPOSURE_OUT%"
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /bigobj work\test_session_exposure_candidate.cpp /Fo:"%SFM64_EXPOSURE_OUT%\test.obj" /Fe:"%SFM64_EXPOSURE_OUT%\test_session_exposure_x64.exe" > "%SFM64_EXPOSURE_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%SFM64_EXPOSURE_OUT%\build.log"
 exit /b 1
)
type "%SFM64_EXPOSURE_OUT%\build.log"
exit /b 0
