@echo off
setlocal
if "%~3"=="" (
  echo Usage: build_sfm_tool_service_factories.cmd ^<game-bin-read-only^> ^<tools-bin-read-only^> ^<isolated-output-dir^>
  exit /b 2
)
set "GAMEBIN=%~f1"
set "TOOLBIN=%~f2"
set "OUT=%~f3"
set "COPYBIN=%OUT%\copied-bin"
if not exist "%GAMEBIN%\tier0.dll" exit /b 3
if not exist "%TOOLBIN%\commedit.dll" exit /b 3
if not exist "%COPYBIN%" mkdir "%COPYBIN%"
if not exist "%OUT%" mkdir "%OUT%"
for %%M in (tier0.dll vstdlib.dll) do (
  if not exist "%GAMEBIN%\%%M" exit /b 3
  copy /b /y "%GAMEBIN%\%%M" "%COPYBIN%\%%M" >nul
  if errorlevel 1 exit /b 4
)
for %%M in (commedit.dll pet.dll vmt.dll) do (
  if not exist "%TOOLBIN%\%%M" exit /b 3
  copy /b /y "%TOOLBIN%\%%M" "%COPYBIN%\%%M" >nul
  if errorlevel 1 exit /b 4
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 5
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_sfm_tool_service_factories.cpp /Fo"%OUT%\probe.obj" /Fe"%OUT%\probe_sfm_tool_service_factories_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > "%OUT%\build.log" 2>&1
if errorlevel 1 exit /b 6
"%OUT%\probe_sfm_tool_service_factories_x86.exe" "%COPYBIN%"
if errorlevel 1 exit /b 7
copy /b /y "%COPYBIN%\sfm-tool-service-factories-results.txt" "%OUT%\results.txt" >nul
if errorlevel 1 exit /b 8
exit /b 0
