@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
set "TMP=%CD%\work"
set "TEMP=%CD%\work"
cl /nologo /std:c++17 /EHsc /W4 work\test_undo_record_submission.cpp /Fowork\test_undo_record_submission.obj /Fework\test_undo_record_submission.exe
if errorlevel 1 exit /b 1
work\test_undo_record_submission.exe

