@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\camera-guide-scope-20261014"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /Bv /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_vulkan_win32_surface_x64.cpp /Fo:"%OUT%\sfm64_session_ui_camera_guide_scope.obj" /Fe:"%OUT%\sfm64_session_ui_camera_guide_scope.exe" user32.lib gdi32.lib comdlg32.lib comctl32.lib > "%OUT%\editor-build.log" 2>&1
if errorlevel 1 (
  type "%OUT%\editor-build.log"
  exit /b 1
)
"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe" /headers "%OUT%\sfm64_session_ui_camera_guide_scope.exe" > "%OUT%\editor-binary-headers.txt"
if errorlevel 1 exit /b 1
findstr /C:"8664 machine (x64)" "%OUT%\editor-binary-headers.txt" >nul
if errorlevel 1 exit /b 1
findstr /C:"20B magic # (PE32+)" "%OUT%\editor-binary-headers.txt" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_mtt_soldier_scene_render_preparation.cpp /Fo:"%OUT%\test_mtt_preparation.obj" /Fe:"%OUT%\test_mtt_preparation.exe" > "%OUT%\preparation-build.log" 2>&1
if errorlevel 1 (
  type "%OUT%\preparation-build.log"
  exit /b 1
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_mtt_soldier_editor_roundtrip.cpp /Fo:"%OUT%\test_mtt_roundtrip.obj" /Fe:"%OUT%\test_mtt_roundtrip.exe" > "%OUT%\roundtrip-build.log" 2>&1
if errorlevel 1 (
  type "%OUT%\roundtrip-build.log"
  exit /b 1
)
echo Full x64 editor and benchmark comparison tests built in %OUT%
exit /b 0
