@echo off
setlocal
work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe -spirv -T vs_6_0 -E main -fspv-target-env=vulkan1.0 work\sfm64-eye-projection-candidate.hlsl -Fo build\session-core\sfm64-eye-projection-candidate.spv
exit /b %errorlevel%
