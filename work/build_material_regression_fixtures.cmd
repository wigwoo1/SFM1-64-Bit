@echo off
setlocal
cd /d "%~dp0.."
if errorlevel 1 exit /b 1
set "OUT=%CD%\outputs\material-test-fixtures-20261027"
if not exist "%OUT%" mkdir "%OUT%"
set "TEMP=%OUT%\temp"
set "TMP=%TEMP%"
if not exist "%TEMP%" mkdir "%TEMP%"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "TESTS=test_session_material_overrides test_scene_material_inputs test_selfillum_material_cache test_scene_draw_plan test_session_material_identity"
if "%~1"=="--support-only" set "TESTS=test_scene_draw_plan test_session_material_identity"
for %%T in (%TESTS%) do (
 cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\%%T.cpp /Fo:"%OUT%\%%T.obj" /Fe:"%OUT%\%%T.exe" >"%OUT%\%%T-build.log" 2>&1
 if errorlevel 1 exit /b 1
)
exit /b 0
