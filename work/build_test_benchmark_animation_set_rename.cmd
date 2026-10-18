@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
if not exist build\session-core mkdir build\session-core
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_benchmark_animation_set_rename.cpp /Fo:build\session-core\test_benchmark_animation_set_rename.obj /Fe:build\session-core\test_benchmark_animation_set_rename.exe
if errorlevel 1 exit /b 1
build\session-core\test_benchmark_animation_set_rename.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx" work\mtt-soldier-rename-roundtrip.dmx
if errorlevel 1 exit /b 1
