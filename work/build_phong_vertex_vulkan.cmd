@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\phong-vertex-vulkan-20261025"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
set "DXC=work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe"
%DXC% -spirv -T vs_6_0 -E plainMain -Gis -O3 -fvk-use-gl-layout -fspv-target-env=vulkan1.0 work\phong_vertex_vulkan_candidate.hlsl -Fo "%OUT%\phong-plain.spv" >"%OUT%\plain-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\plain-build.log"
 exit /b 1
)
%DXC% -spirv -T vs_6_0 -E packedMain -Gis -O3 -fvk-use-gl-layout -fspv-target-env=vulkan1.0 work\phong_vertex_vulkan_candidate.hlsl -Fo "%OUT%\phong-packed.spv" >"%OUT%\packed-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\packed-build.log"
 exit /b 1
)
%DXC% -spirv -T ps_6_0 -E captureMain -Gis -O3 -fvk-use-gl-layout -fspv-target-env=vulkan1.0 work\phong_vertex_vulkan_candidate.hlsl -Fo "%OUT%\phong-capture.spv" >"%OUT%\capture-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\capture-build.log"
 exit /b 1
)
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_phong_vertex_vulkan.cpp /Fo:"%OUT%\vulkan.obj" /Fe:"%OUT%\phong_vertex_vulkan_x64.exe" >"%OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\build.log"
 exit /b 1
)
exit /b 0
