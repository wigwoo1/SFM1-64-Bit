@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\native-spatial-partition-20261016"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_native_spatial_partition.cpp /Fo%OUT%\native.obj /Fe%OUT%\spatial_partition_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\native-build.log 2>&1
exit /b %errorlevel%
