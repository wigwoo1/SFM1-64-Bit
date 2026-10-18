@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\optimized-box-x64-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /DSFM64_OWNED_BOX_CANDIDATE work\probe_native_convex_brush_ray.cpp /Fo%OUT%\candidate.obj /Fe%OUT%\optimized_box_candidate_x64.exe > %OUT%\build.log 2>&1
if errorlevel 1 type %OUT%\build.log
exit /b %errorlevel%
