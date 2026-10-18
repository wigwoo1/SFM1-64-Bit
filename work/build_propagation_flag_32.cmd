@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\probe_document_propagation_flag.cpp /Fowork\probe_propagation_flag_32.obj /Fework\probe_propagation_flag_32.exe
exit /b %errorlevel%
