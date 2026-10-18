@echo off
setlocal
cd /d "%~dp0.."
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\outputs\save-copy-safety-20261014
set TMP=%CD%\outputs\save-copy-safety-20261014
if not exist "%TEMP%" mkdir "%TEMP%"
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss-fff"') do set RUN_ID=%%i
set TEST_DIR=outputs\save-copy-safety-20261014\test-data-%RUN_ID%
mkdir "%TEST_DIR%"
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_atomic_session_save.cpp /Fo:"%TEMP%\test_atomic_session_save.obj" /Fe:"%TEMP%\test_atomic_session_save.exe"
if errorlevel 1 exit /b 1
"%TEMP%\test_atomic_session_save.exe" "%TEST_DIR%"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_session_document_save_copy.cpp /Fo:"%TEMP%\test_session_document_save_copy.obj" /Fe:"%TEMP%\test_session_document_save_copy.exe"
if errorlevel 1 exit /b 1
"%TEMP%\test_session_document_save_copy.exe" "%TEST_DIR%"
if errorlevel 1 exit /b 1
certutil -hashfile "%TEST_DIR%\existing.dmx" SHA256 > "%TEST_DIR%\target-sha256.txt"
certutil -hashfile "%TEST_DIR%\race-target.dmx" SHA256 >> "%TEST_DIR%\target-sha256.txt"
certutil -hashfile "%TEST_DIR%\document-existing.dmx" SHA256 >> "%TEST_DIR%\target-sha256.txt"
certutil -hashfile "%TEST_DIR%\document-race-target.dmx" SHA256 >> "%TEST_DIR%\target-sha256.txt"
exit /b %errorlevel%
