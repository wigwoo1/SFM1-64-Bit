@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_key_skinning_path.cpp /Fowork\test_key_skinning_path.obj /Fework\test_key_skinning_path.exe
if errorlevel 1 exit /b 1
work\test_key_skinning_path.exe
