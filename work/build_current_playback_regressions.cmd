@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_cached_reference_comparison test_parallel_expression_samples test_parallel_private_expression_chains test_session_expression_runtime test_parallel_prepared_expressions) do (
 cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\%%T.cpp /Fo:build\session-core\%%T.obj /Fe:build\session-core\%%T.exe
 if errorlevel 1 exit /b 1
 for %%F in (11 139) do (
  build\session-core\%%T.exe work\roundtrip-%%F.dmx
  if errorlevel 1 exit /b 1
 )
)
