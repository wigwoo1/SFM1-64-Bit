@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
dumpbin /disasm "D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll" > work\tier0-disassembly.txt
exit /b %errorlevel%
