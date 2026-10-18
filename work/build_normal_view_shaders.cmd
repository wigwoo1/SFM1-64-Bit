@echo off
setlocal
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T vs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-world-normal-candidate.hlsl -Fo build\session-core\sfm64-world-normal-candidate.spv -Fc outputs\world-normal-shader.spvasm
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T ps_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-normal-view-candidate.hlsl -Fo build\session-core\sfm64-normal-view-candidate.spv -Fc outputs\normal-view-shader.spvasm
exit /b %errorlevel%
