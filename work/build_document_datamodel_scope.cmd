@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_document_datamodel_scope.cpp /Fowork\test_document_datamodel_scope.obj /Fework\test_document_datamodel_scope.exe
if errorlevel 1 exit /b 1
work\test_document_datamodel_scope.exe > outputs\sfm64-document-datamodel-scope-validation.txt
exit /b %errorlevel%
