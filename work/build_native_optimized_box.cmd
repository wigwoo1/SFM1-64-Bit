@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\optimized-box-native-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_BRUSH_ORACLE /DSFM64_NATIVE_BOX_ORACLE work\probe_native_convex_brush_ray.cpp /Fo%OUT%\native.obj /Fe%OUT%\optimized_box_native_x86.exe > %OUT%\build.log 2>&1
if errorlevel 1 type %OUT%\build.log
exit /b %errorlevel%
