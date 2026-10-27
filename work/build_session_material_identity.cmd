@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-resource-identity-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%\temp"
set "TMP=%TEMP%"
if not exist "%TEMP%" mkdir "%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\test_session_material_identity.cpp /Fo:"%OUT%\identity.obj" /Fe:"%OUT%\material_identity_x64.exe" >"%OUT%\identity-build.log" 2>&1
if errorlevel 1 exit /b 1
for %%T in (test_session_material_overrides test_scene_material_inputs test_scene_draw_plan test_selfillum_material_cache) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%T.cpp /Fo:"%OUT%\%%T.obj" /Fe:"%OUT%\%%T.exe" >"%OUT%\%%T-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
exit /b 0
