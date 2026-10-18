@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
dumpbin /disasm "D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\sdktools\python\2.7\win32\Lib\site-packages\vp.pyd" > work\viewport-procedural-disassembly.txt
exit /b %errorlevel%
