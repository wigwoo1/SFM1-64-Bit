@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\read_ifm_vtable_targets.cpp /Fework\read_ifm_vtable_targets.exe /Fowork\read_ifm_vtable_targets.obj
if errorlevel 1 exit /b 1
work\read_ifm_vtable_targets.exe "F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll" 0x10AAE45C 0x10AAE460 0x10AAE4FC 0x10AAE500 0x10AAE504 0x10074174
