@echo off
setlocal
if "%~2"=="" (
  echo Usage: build_engine_service_provider_factories.cmd ^<original-bin-read-only^> ^<isolated-output-dir^>
  exit /b 2
)
set "ORIGINALBIN=%~f1"
set "OUT=%~f2"
set "COPYBIN=%OUT%\copied-bin"
if not exist "%ORIGINALBIN%\materialsystem.dll" exit /b 3
if not exist "%COPYBIN%" mkdir "%COPYBIN%"
if not exist "%OUT%" mkdir "%OUT%"
for %%M in (tier0.dll vstdlib.dll filesystem_stdio.dll shaderapidx9.dll materialsystem.dll vphysics.dll inputsystem.dll datacache.dll vgui2.dll shaderapidx10.dll shaderapiempty.dll localize.dll soundsystem.dll soundemittersystem.dll studiorender.dll scenefilecache.dll networksystem.dll vguimatsurface.dll video_services.dll valve_avi.dll mdllib.dll engine.dll) do (
  if not exist "%ORIGINALBIN%\%%M" exit /b 3
  copy /b /y "%ORIGINALBIN%\%%M" "%COPYBIN%\%%M" >nul
  if errorlevel 1 exit /b 4
)
for %%M in (steam_api.dll binkw32.dll) do (
  if not exist "%ORIGINALBIN%\%%M" exit /b 3
  copy /b /y "%ORIGINALBIN%\%%M" "%COPYBIN%\%%M" >nul
  if errorlevel 1 exit /b 4
)
if not exist "%COPYBIN%\tools" mkdir "%COPYBIN%\tools"
if not exist "%ORIGINALBIN%\tools\ifm.dll" exit /b 3
copy /b /y "%ORIGINALBIN%\tools\ifm.dll" "%COPYBIN%\tools\ifm.dll" >nul
if errorlevel 1 exit /b 4
for %%M in (QtCore4.dll QtGui4.dll QtSvg4.dll shiboken-python2.7.dll sixense.dll python27.dll) do (
  if not exist "%ORIGINALBIN%\%%M" exit /b 3
  copy /b /y "%ORIGINALBIN%\%%M" "%COPYBIN%\%%M" >nul
  if errorlevel 1 exit /b 4
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 5
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_engine_service_provider_factories.cpp /Fo"%OUT%\probe.obj" /Fe"%OUT%\probe_engine_service_provider_factories_x86.exe" /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > "%OUT%\build.log" 2>&1
if errorlevel 1 exit /b 6
"%OUT%\probe_engine_service_provider_factories_x86.exe" "%COPYBIN%" "%OUT%\results.txt"
exit /b %errorlevel%
