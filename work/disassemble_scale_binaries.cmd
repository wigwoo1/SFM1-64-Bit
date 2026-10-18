@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
dumpbin /disasm "D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll" > work\native-client-scale-disassembly.txt
exit /b %errorlevel%
