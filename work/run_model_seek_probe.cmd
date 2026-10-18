@echo off
setlocal
set SFM64_DIAGNOSTIC_SEEK_TEST=1
call work\run_model_swapchain_probe.cmd 0 3 work\retained-shot-sequence-v1
exit /b %errorlevel%
