@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\brush-dispatch-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_NATIVE_BRUSH_ORACLE /DSFM64_AUTO_BRUSH_DISPATCH work\probe_native_convex_brush_ray.cpp /Fo%OUT%\native.obj /Fe%OUT%\brush_ray_native_x86.exe > %OUT%\native-build.log 2>&1
if errorlevel 1 (
 type %OUT%\native-build.log
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_AUTO_BRUSH_DISPATCH work\probe_native_convex_brush_ray.cpp /Fo%OUT%\candidate.obj /Fe%OUT%\brush_ray_candidate_x64.exe > %OUT%\candidate-build.log 2>&1
if errorlevel 1 type %OUT%\candidate-build.log
exit /b %errorlevel%
