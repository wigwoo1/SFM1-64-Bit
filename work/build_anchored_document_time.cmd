@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_anchored_document_time.cpp /Fowork\test_anchored_document_time.obj /Fework\test_anchored_document_time.exe
if errorlevel 1 exit /b 1
work\test_anchored_document_time.exe > outputs\sfm64-anchored-document-time-validation.txt
exit /b %errorlevel%
