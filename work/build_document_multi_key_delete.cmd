@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_document_multi_key_delete.cpp /Fowork\test_document_multi_key_delete.obj /Fework\test_document_multi_key_delete.exe
if errorlevel 1 exit /b 1
for %%K in (scalar vector3) do (
 work\test_document_multi_key_delete.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx" %%K
 if errorlevel 1 exit /b 1
 work\test_document_multi_key_delete.exe work\mtt_soldier_opaque_roundtrip.dmx %%K
 if errorlevel 1 exit /b 1
)
exit /b %errorlevel%
