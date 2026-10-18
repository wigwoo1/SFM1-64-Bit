@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
for %%T in (test_hierarchy_scale test_owned_scalar_float_write test_owned_expression_boundaries test_owned_attachment_listeners test_registered_operator_build test_undo_journal test_document_scalar_edit test_document_float_keys test_owned_session_expression_execute) do (
 cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\%%T.cpp /Fobuild\session-core\%%T.obj /Febuild\session-core\%%T.exe
 if errorlevel 1 exit /b 1
)
for %%T in (test_hierarchy_scale test_owned_scalar_float_write test_owned_expression_boundaries test_owned_attachment_listeners test_registered_operator_build test_undo_journal) do (
 build\session-core\%%T.exe
 if errorlevel 1 exit /b 1
)
for %%F in (11 139) do (
 for %%T in (test_document_scalar_edit test_document_float_keys test_owned_session_expression_execute) do (
  build\session-core\%%T.exe work\roundtrip-%%F.dmx
  if errorlevel 1 exit /b 1
 )
)
