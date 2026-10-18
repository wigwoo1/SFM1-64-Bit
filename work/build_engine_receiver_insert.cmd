@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_engine_receiver_insert.cpp /Fowork\test_engine_receiver_insert.obj /Fework\test_engine_receiver_insert.exe
if errorlevel 1 exit /b 1
work\test_engine_receiver_insert.exe > outputs\sfm64-engine-receiver-insert-validation.txt
exit /b %errorlevel%


