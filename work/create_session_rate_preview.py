from pathlib import Path
s=Path('work/run_session_rate_grid_probe.cmd').read_text().replace('setlocal','setlocal\nset SFM64_DIAGNOSTIC_CAPTURE_FRAMES=0\ncd /d "%~dp0.."').replace('probe_vulkan_win32_surface_x64.exe','probe_vulkan_win32_surface_x64.exe --interactive')
Path('work/open_session_rate_vulkan.cmd').write_text(s)
