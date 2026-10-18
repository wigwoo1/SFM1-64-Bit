@echo off
setlocal
set TEMP=%CD%\work
set TMP=%CD%\work
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_shadow_bias_capabilities.cpp /Fo:build\session-core\probe_shadow_bias_capabilities.obj /Fe:build\session-core\probe_shadow_bias_capabilities.exe
if errorlevel 1 exit /b 1
build\session-core\probe_shadow_bias_capabilities.exe > outputs\shadow-bias-device-capabilities.txt
exit /b %errorlevel%
