@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_projected_light_texture_inputs.cpp /Fobuild\session-core\test_projected_light_texture_inputs.obj /Febuild\session-core\test_projected_light_texture_inputs.exe
if errorlevel 1 exit /b 1
build\session-core\test_projected_light_texture_inputs.exe > outputs\projected-light-texture-input-validation.txt
exit /b %errorlevel%
