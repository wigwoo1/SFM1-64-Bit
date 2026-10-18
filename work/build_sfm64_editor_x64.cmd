@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1

set "OUT=%CD%\outputs\sfm64-editor-x64"
set "TEMP=%OUT%\temp"
set "TMP=%OUT%\temp"
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%TEMP%" mkdir "%TEMP%"

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

cl /nologo /Bv /std:c++17 /fp:strict /EHsc /W4 /O2 /Iwork\dependencies\vulkan-headers-6802bb4733b6\Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1\include work\probe_vulkan_win32_surface_x64.cpp /Fo:"%OUT%\sfm64_session_ui.obj" /Fe:"%OUT%\sfm64_session_ui.exe" user32.lib gdi32.lib comdlg32.lib comctl32.lib > "%OUT%\build.log" 2>&1
if errorlevel 1 (
  type "%OUT%\build.log"
  exit /b 1
)

"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe" /headers "%OUT%\sfm64_session_ui.exe" > "%OUT%\binary-headers.txt"
if errorlevel 1 exit /b 1
findstr /C:"8664 machine (x64)" "%OUT%\binary-headers.txt" >nul
if errorlevel 1 exit /b 1
findstr /C:"20B magic # (PE32+)" "%OUT%\binary-headers.txt" >nul
if errorlevel 1 exit /b 1

> "%OUT%\build-manifest.txt" echo source=work\probe_vulkan_win32_surface_x64.cpp
>> "%OUT%\build-manifest.txt" echo executable=%OUT%\sfm64_session_ui.exe
>> "%OUT%\build-manifest.txt" echo target=AMD64 PE32+
>> "%OUT%\build-manifest.txt" echo flags=/std:c++17 /fp:strict /EHsc /W4 /O2
>> "%OUT%\build-manifest.txt" echo architecture_check=dumpbin confirmed AMD64 and PE32+
certutil -hashfile "%OUT%\sfm64_session_ui.exe" SHA256 >> "%OUT%\build-manifest.txt"
if errorlevel 1 exit /b 1
certutil -hashfile "work\probe_vulkan_win32_surface_x64.cpp" SHA256 >> "%OUT%\build-manifest.txt"
if errorlevel 1 exit /b 1
type "%OUT%\build-manifest.txt"
exit /b 0
