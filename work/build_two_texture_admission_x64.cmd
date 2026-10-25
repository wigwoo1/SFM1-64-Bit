@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\two-texture-admission-20261024"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%"
set "TMP=%OUT%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_two_texture_bridge_resources test_two_texture_draw_transport test_vulkan_scene_commands test_unlit_two_texture_inputs test_material_uv_transform test_mtt_soldier_scene_render_preparation test_source_bsp_world_materials test_atomic_session_save test_session_document_save_copy test_mtt_soldier_editor_roundtrip) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\%%T.cpp /Fo:"%OUT%\%%T.obj" /Fe:"%OUT%\%%T.exe" > "%OUT%\%%T-build.log" 2>&1
 if errorlevel 1 (
  type "%OUT%\%%T-build.log"
  exit /b 1
 )
)
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_vulkan_win32_surface_x64.cpp /Fo:"%OUT%\sfm64_session_ui_admission.obj" /Fe:"%OUT%\sfm64_session_ui_admission.exe" user32.lib gdi32.lib comdlg32.lib comctl32.lib > "%OUT%\editor-build.log" 2>&1
if errorlevel 1 (
 type "%OUT%\editor-build.log"
 exit /b 1
)
dumpbin /headers "%OUT%\sfm64_session_ui_admission.exe" > "%OUT%\binary-headers.txt"
if errorlevel 1 exit /b 1
findstr /C:"8664 machine (x64)" "%OUT%\binary-headers.txt" >nul
if errorlevel 1 exit /b 1
findstr /C:"20B magic # (PE32+)" "%OUT%\binary-headers.txt" >nul
if errorlevel 1 exit /b 1
echo Full AMD64 editor and ten affected tests built in %OUT%
exit /b 0
