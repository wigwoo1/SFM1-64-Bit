@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb473b63ed5efd3adb308a6c885ef180ea1\include work\inspect_mdl_mesh_asset.cpp /Fe:build\session-core\inspect_mdl_mesh_asset.exe
exit /b %errorlevel%
