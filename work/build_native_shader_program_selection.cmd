@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /O2 /W4 /c work\dependencies\lzma-decoder\LzmaDec.c /Fowork\lzma_decoder.obj
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 work\test_native_shader_program_selection.cpp work\lzma_decoder.obj /Fowork\test_native_shader_program_selection.obj /Fework\test_native_shader_program_selection.exe
if errorlevel 1 exit /b 1
work\test_native_shader_program_selection.exe
