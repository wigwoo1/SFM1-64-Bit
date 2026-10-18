@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\benchmark_fur_scene_preparation.cpp /Fowork\benchmark_fur_scene_preparation.obj /Fework\benchmark_fur_scene_preparation.exe
if errorlevel 1 exit /b 1
work\benchmark_fur_scene_preparation.exe >outputs\sfm64-fur-scene-preparation-benchmark.txt
exit /b %errorlevel%
