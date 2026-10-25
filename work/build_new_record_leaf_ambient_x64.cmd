@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\new-record-leaf-ambient-20261024"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_new_record_leaf_ambient probe_new_record_leaf_ambient test_benchmark_new_record_ambient) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%T.cpp /Fo:"%OUT%\%%T.obj" /Fe:"%OUT%\%%T.exe" > "%OUT%\%%T-build.log" 2>&1
 if errorlevel 1 (
  type "%OUT%\%%T-build.log"
  exit /b 1
 )
 dumpbin /headers "%OUT%\%%T.exe" > "%OUT%\%%T-headers.txt"
 if errorlevel 1 exit /b 1
 findstr /C:"8664 machine (x64)" "%OUT%\%%T-headers.txt" >nul
 if errorlevel 1 exit /b 1
 findstr /C:"20B magic # (PE32+)" "%OUT%\%%T-headers.txt" >nul
 if errorlevel 1 exit /b 1
)
exit /b 0
