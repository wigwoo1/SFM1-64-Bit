@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /arch:SSE2 work\probe_studio_association_release.cpp /Fowork\probe_studio_association_release_32.obj /Fework\probe_studio_association_release_32.exe
exit /b %errorlevel%
