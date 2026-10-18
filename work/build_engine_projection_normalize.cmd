@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_engine_projection_normalize.cpp /Fowork\test_engine_projection_normalize.obj /Fework\test_engine_projection_normalize.exe
if errorlevel 1 exit /b 1
work\test_engine_projection_normalize.exe > outputs\sfm64-engine-projection-normalize-validation.txt
exit /b %errorlevel%

