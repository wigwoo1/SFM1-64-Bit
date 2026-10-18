@echo off
setlocal
set "TMP=%CD%\work"
set "TEMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul
cl /nologo /std:c++17 /EHsc /W4 work\probe_sfm_time_to_frame_parity.cpp /Fowork\probe_time_frame_x86.obj /Fework\probe_time_frame_x86.exe
if errorlevel 1 exit /b 1
work\probe_time_frame_x86.exe outputs\sfm-time-to-frame-original-oracle.bin
if errorlevel 1 exit /b 1
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
cl /nologo /std:c++17 /EHsc /W4 work\probe_sfm_time_to_frame_parity.cpp /Fowork\probe_time_frame_x64.obj /Fework\probe_time_frame_x64.exe
if errorlevel 1 exit /b 1
work\probe_time_frame_x64.exe outputs\sfm-time-to-frame-original-oracle.bin
