$items = Get-Process -Name cl,link,probe_vulkan_win32_surface_x64 -ErrorAction SilentlyContinue
if ($items) { $items | Select-Object ProcessName,Id,StartTime | ConvertTo-Json -Compress } else { Write-Output 'No compiler, linker, or surface probe process found.' }
