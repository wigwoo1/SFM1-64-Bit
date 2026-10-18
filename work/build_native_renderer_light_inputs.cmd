@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\audit_native_renderer_light_inputs.cpp /Fobuild\session-core\audit_native_renderer_light_inputs.obj /Febuild\session-core\audit_native_renderer_light_inputs.exe
if errorlevel 1 exit /b 1
build\session-core\audit_native_renderer_light_inputs.exe work\short-timeline-session.dmx > outputs\native-renderer-light-inputs.txt
exit /b %errorlevel%
