@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\probe_material_nonposition_upload.cpp /Fowork\probe_material_nonposition_upload_64.obj /Fework\probe_material_nonposition_upload_64.exe
exit /b %errorlevel%
