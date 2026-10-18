@echo off
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\probe_native_studio_vcollide_load.cpp /Fooutputs\studio-vcollide-read-20261016\load-native.obj /Feoutputs\studio-vcollide-read-20261016\studio_vcollide_load_native_x86.exe /link /MANIFEST:EMBED /MANIFESTUAC:level='asInvoker' > outputs\studio-vcollide-read-20261016\load-native-build.log 2>&1
exit /b %errorlevel%
