@echo off
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T vs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-projected-light-vertex-candidate.hlsl -Fo build\session-core\sfm64-projected-light-vertex-candidate.spv -Fc outputs\projected-light-vertex.spvasm
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T ps_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-projected-light-fragment-candidate.hlsl -Fo build\session-core\sfm64-projected-light-fragment-candidate.spv -Fc outputs\projected-light-fragment.spvasm
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T cs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-light-buffer-readback.hlsl -Fo build\session-core\sfm64-light-buffer-readback.spv -Fc outputs\light-buffer-readback.spvasm
if errorlevel 1 exit /b 1
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T ps_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-projected-light-shadow-fragment-candidate.hlsl -Fo build\session-core\sfm64-projected-light-shadow-fragment-candidate.spv -Fc outputs\projected-light-shadow-fragment.spvasm
exit /b %errorlevel%
