@echo off
setlocal
if "%~2"=="" (
  echo Usage: build_materialsystem_connect_lifecycle.cmd ^<copied-bin-dir^> ^<isolated-output-dir^>
  exit /b 2
)
set "COPYBIN=%~f1"
set "OUT=%~f2"
if not exist "%COPYBIN%\materialsystem.dll" exit /b 3
if not exist "%COPYBIN%\shaderapidx9.dll" exit /b 3
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 4
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_materialsystem_connect_lifecycle.cpp /Fo"%OUT%\probe.obj" /Fe"%OUT%\probe_materialsystem_connect_lifecycle_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > "%OUT%\build.log" 2>&1
if errorlevel 1 exit /b 5
"%OUT%\probe_materialsystem_connect_lifecycle_x86.exe" "%COPYBIN%" "%OUT%\result.txt"
exit /b %errorlevel%
