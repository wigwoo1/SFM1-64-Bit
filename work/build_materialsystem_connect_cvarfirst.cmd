@echo off
setlocal
if "%~2"=="" (echo Usage: build_materialsystem_connect_cvarfirst.cmd ^<original-bin-read-only^> ^<isolated-output-dir^>& exit /b 2)
set "ORIGINALBIN=%~f1"
set "OUT=%~f2"
set "COPYBIN=%OUT%\copied-bin"
if not exist "%ORIGINALBIN%\materialsystem.dll" exit /b 3
if not exist "%COPYBIN%" mkdir "%COPYBIN%"
if not exist "%OUT%" mkdir "%OUT%"
for %%M in (tier0.dll vstdlib.dll filesystem_stdio.dll shaderapidx9.dll materialsystem.dll) do (
  if not exist "%ORIGINALBIN%\%%M" exit /b 3
  copy /b /y "%ORIGINALBIN%\%%M" "%COPYBIN%\%%M" >nul
  if errorlevel 1 exit /b 4
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 5
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_materialsystem_connect_cvarfirst.cpp /Fo"%OUT%\probe.obj" /Fe"%OUT%\probe_materialsystem_connect_cvarfirst_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > "%OUT%\build.log" 2>&1
if errorlevel 1 exit /b 6
"%OUT%\probe_materialsystem_connect_cvarfirst_x86.exe" "%COPYBIN%" "%OUT%\result.txt"
exit /b %errorlevel%