@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\audit_projected_light_textures.cpp /Fowork\audit_projected_light_textures.obj /Fework\audit_projected_light_textures.exe
if errorlevel 1 exit /b 1
work\audit_projected_light_textures.exe > outputs\sfm64-projected-light-texture-audit.txt
exit /b %errorlevel%
