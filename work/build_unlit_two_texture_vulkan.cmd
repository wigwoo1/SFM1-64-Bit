@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if not exist outputs\unlit-two-texture-gpu-20261014 mkdir outputs\unlit-two-texture-gpu-20261014
set "OUT=outputs\unlit-two-texture-gpu-20261014"
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T vs_6_0 -E vertexMain -fspv-target-env=vulkan1.0 work\unlit_two_texture_gpu_reference.hlsl -Fo %OUT%\two-texture-reference-vs.spv
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T ps_6_0 -E fragmentMain -fspv-target-env=vulkan1.0 work\unlit_two_texture_gpu_reference.hlsl -Fo %OUT%\two-texture-reference-ps.spv
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T vs_6_0 -E rawVertexMain -fspv-target-env=vulkan1.0 work\unlit_two_texture_gpu_reference.hlsl -Fo %OUT%\two-texture-raw-vs.spv
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T ps_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-unlit-two-texture-candidate.hlsl -Fo %OUT%\sfm64-unlit-two-texture-candidate.spv
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_unlit_two_texture_vulkan.cpp /Fo%OUT%\probe_unlit_two_texture_vulkan.obj /Fe%OUT%\probe_unlit_two_texture_vulkan.exe user32.lib
exit /b %errorlevel%
