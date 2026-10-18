@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist build\session-core mkdir build\session-core
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_session_document_delete_empty_dag.cpp /Fo:build\session-core\test_session_document_delete_empty_dag.obj /Fe:build\session-core\test_session_document_delete_empty_dag.exe
if errorlevel 1 exit /b 1
build\session-core\test_session_document_delete_empty_dag.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx"
