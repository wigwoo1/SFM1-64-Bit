@echo off
setlocal
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T ps_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-material-selfillum-candidate.hlsl -Fo build\session-core\sfm64-material-selfillum-candidate.spv -Fc outputs\material-selfillum-shader-abi.spvasm
exit /b %errorlevel%
