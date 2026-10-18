@echo off
set "TEMP=%CD%\work"
set "TMP=%CD%\work"
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /O2 /fp:strict work\benchmark_cached_play_metadata.cpp /Fowork\benchmark_cached_play_metadata.obj /Fework\benchmark_cached_play_metadata.exe
if errorlevel 1 exit /b 1
work\benchmark_cached_play_metadata.exe work\roundtrip-11.dmx outputs\sfm64-cached-play-metadata-benchmark.json
