@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "ROOT=%CD%"
set "OUT=%ROOT%\outputs\source-plain-lightmap-sampling-test-x64"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /Bv /std:c++17 /fp:strict /EHsc /W4 /O2 /I"%ROOT%\work" "%ROOT%\work\test_source_plain_lightmap_sampling.cpp" /Fo:"%OUT%\test.obj" /Fe:"%OUT%\test_source_plain_lightmap_sampling.exe" > "%OUT%\build.log" 2>&1
if errorlevel 1 (type "%OUT%\build.log" & exit /b 1)
type "%OUT%\build.log"
"%OUT%\test_source_plain_lightmap_sampling.exe"
exit /b %errorlevel%
