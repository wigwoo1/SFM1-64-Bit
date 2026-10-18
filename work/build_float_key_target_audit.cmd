@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\audit_float_key_scene_targets.cpp /Fowork\audit_float_key_scene_targets.obj /Fework\audit_float_key_scene_targets.exe
if errorlevel 1 exit /b 1
work\audit_float_key_scene_targets.exe work\roundtrip-11.dmx > outputs\sfm64-float-key-scene-targets.txt
if errorlevel 1 exit /b 1
