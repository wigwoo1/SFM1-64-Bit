@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "TMP=%CD%\work"
set "TEMP=%CD%\work"
cl /nologo /std:c++17 /EHsc /W4 work\test_string_attribute_undo_replay.cpp /Fowork\test_string_attribute_undo_replay.obj /Fework\test_string_attribute_undo_replay.exe
if errorlevel 1 exit /b 1
work\test_string_attribute_undo_replay.exe

