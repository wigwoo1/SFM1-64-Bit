@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\material-effective-parameters-20261017"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_shader_parameter_registry.cpp /Fo%OUT%\registry.obj /Fe%OUT%\shader_parameter_registry_reference_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > %OUT%\registry-build.log 2>&1
exit /b %errorlevel%
