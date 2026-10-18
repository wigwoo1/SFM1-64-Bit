@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict /arch:SSE2 work\probe_engine_projection_receiver_gate.cpp /Fowork\probe_projection_receiver_gate_32.obj /Fework\probe_projection_receiver_gate_32.exe
exit /b %errorlevel%
