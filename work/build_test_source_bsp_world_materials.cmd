@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set TEMP=%CD%\work
set TMP=%CD%\work
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_source_bsp_world_materials.cpp /Fework\test_source_bsp_world_materials.exe
if errorlevel 1 exit /b 1
work\test_source_bsp_world_materials.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp" "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\gameinfo.txt"
