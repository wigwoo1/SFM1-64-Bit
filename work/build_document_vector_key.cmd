@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_document_vector_key.cpp /Fowork\test_document_vector_key.obj /Fework\test_document_vector_key.exe
if errorlevel 1 exit /b 1
for %%C in (0 1 2) do (
 work\test_document_vector_key.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx" %%C
 if errorlevel 1 exit /b 1
 work\test_document_vector_key.exe work\mtt_soldier_opaque_roundtrip.dmx %%C
 if errorlevel 1 exit /b 1
)
