@echo off
set "TEMP=%CD%\work\compiler-temp"
set "TMP=%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if not "%errorlevel%"=="0" exit /b 1
for %%P in (WINDOWS FSTREAM REGISTRY) do (
 cl /nologo /std:c++17 /EHsc /W4 /Od /MT /DPROBE_%%P work\diagnose_dispatch_headers.cpp /Fowork\probe_header_%%P.obj /Fework\probe_header_%%P.exe
 if errorlevel 1 exit /b 1
)

cl /nologo /std:c++17 /EHsc /W4 /Od /MT /DPROBE_WINDOWS /DPROBE_FSTREAM /DPROBE_REGISTRY work\diagnose_dispatch_headers.cpp /Fowork\probe_header_ALL.obj /Fework\probe_header_ALL.exe
if errorlevel 1 exit /b 1

