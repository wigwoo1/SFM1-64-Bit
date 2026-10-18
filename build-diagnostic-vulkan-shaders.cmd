@echo off
setlocal
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -fspv-target-env=vulkan1.0 -T vs_6_0 -E main work\sfm64-diagnostic-model.hlsl -Fo build\session-core\sfm64-diagnostic-model.spv
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -fspv-target-env=vulkan1.0 -T ps_6_0 -E main work\sfm64-diagnostic-texture.hlsl -Fo build\session-core\sfm64-diagnostic-texture.spv
if errorlevel 1 exit /b 1
exit /b 0
