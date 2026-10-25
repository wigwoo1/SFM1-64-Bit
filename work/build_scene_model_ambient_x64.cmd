@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\scene-model-ambient-20261024"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T cs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-model-ambient-readback.hlsl -Fo "%OUT%\ambient-readback.spv" -Fc "%OUT%\ambient-readback.spvasm" > "%OUT%\shader-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\shader-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_scene_model_ambient_records probe_scene_model_ambient_vulkan) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\%%T.cpp /Fo:"%OUT%\%%T.obj" /Fe:"%OUT%\%%T.exe" > "%OUT%\%%T-build.log" 2>&1
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
