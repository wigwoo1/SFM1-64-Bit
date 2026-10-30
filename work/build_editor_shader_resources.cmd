@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="" exit /b 1
set "SHADERS=%~f1"
if not exist "%SHADERS%" mkdir "%SHADERS%"
if not exist "%SHADERS%" exit /b 1
if not defined SFM64_DXC_EXE set "SFM64_DXC_EXE=%CD%\work\dependencies\dxc-v1.9.2609\bin\x64\dxc.exe"
if not exist "%SFM64_DXC_EXE%" (
 echo Shader compiler missing. Set SFM64_DXC_EXE to your DXC executable.
 exit /b 1
)
for /f "usebackq eol=# tokens=1,2" %%P in ("%CD%\work\editor_shader_manifest.txt") do (
 call :compile "%%Q" "%%P"
 if errorlevel 1 exit /b 1
)
exit /b 0
:compile
"%SFM64_DXC_EXE%" -spirv -fspv-target-env=vulkan1.0 -T %~2 -E main "work\%~1.hlsl" -Fo "%SHADERS%\%~1.spv"
exit /b %errorlevel%
