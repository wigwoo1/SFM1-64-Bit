@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\test_document_frame_rate.cpp /Fowork\test_document_frame_rate.obj /Fework\test_document_frame_rate.exe
if errorlevel 1 exit /b 1
work\test_document_frame_rate.exe > outputs\sfm64-document-frame-rate-validation.txt
exit /b %errorlevel%
