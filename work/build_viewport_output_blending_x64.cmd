@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 2
set "SFM64_BLEND_OUT=%~f1"
if not exist "%SFM64_BLEND_OUT%" mkdir "%SFM64_BLEND_OUT%"
if errorlevel 1 exit /b 1
"work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe" -spirv -fspv-target-env=vulkan1.0 -T ps_6_0 -E main work\viewport_output_blend_probe.hlsl -Fo "%SFM64_BLEND_OUT%\blend-probe.spv" > "%SFM64_BLEND_OUT%\shader-build.log" 2>&1
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_viewport_output_blending_x64.cpp /Fo:"%SFM64_BLEND_OUT%\viewport_output_blending_x64.obj" /Fe:"%SFM64_BLEND_OUT%\viewport_output_blending_x64.exe" /link /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" > "%SFM64_BLEND_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%SFM64_BLEND_OUT%\build.log"
 exit /b 1
)
type "%SFM64_BLEND_OUT%\build.log"
exit /b 0
