@echo off
setlocal
cd /d "%~dp0.."
set "OUT=outputs\bsp-leaf-ambient-20261015"
if not exist "%OUT%" mkdir "%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_source_bsp_leaf_ambient audit_source_bsp_leaf_ambient) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%T.cpp /Fo%OUT%\%%T.obj /Fe%OUT%\%%T.exe > %OUT%\%%T-build.log 2>&1
 if errorlevel 1 (
  type %OUT%\%%T-build.log
  exit /b 1
 )
)
