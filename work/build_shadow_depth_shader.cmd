@echo off
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T cs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-shadow-depth-readback.hlsl -Fo build\session-core\sfm64-shadow-depth-readback.spv -Fc outputs\shadow-depth-readback.spvasm
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T vs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-shadow-depth-vertex.hlsl -Fo build\session-core\sfm64-shadow-depth-vertex.spv -Fc outputs\shadow-depth-vertex.spvasm
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T vs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-shadow-scene-vertex.hlsl -Fo build\session-core\sfm64-shadow-scene-vertex.spv -Fc outputs\shadow-scene-vertex.spvasm
exit /b %errorlevel%
