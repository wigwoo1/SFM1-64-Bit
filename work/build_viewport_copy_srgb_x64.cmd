@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 2
set "SFM64_COPY_OUT=%~f1"
if not exist "%SFM64_COPY_OUT%" mkdir "%SFM64_COPY_OUT%"
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_viewport_copy_srgb_x64.cpp /Fo:"%SFM64_COPY_OUT%\viewport_copy_srgb_x64.obj" /Fe:"%SFM64_COPY_OUT%\viewport_copy_srgb_x64.exe" d3d9.lib user32.lib /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" > "%SFM64_COPY_OUT%\build.log" 2>&1
if errorlevel 1 (
  type "%SFM64_COPY_OUT%\build.log"
  exit /b 1
)
type "%SFM64_COPY_OUT%\build.log"
exit /b 0
