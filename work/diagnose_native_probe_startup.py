import subprocess,ctypes
ctypes.windll.kernel32.SetErrorMode(3)
for exe,out in [('probe_native_curve_dispatch','native-curve-dispatch-diagnostic-v1.bin'),('probe_native_curve_components','native-curve-control-v1.bin')]:
 try:
  p=subprocess.run(['work/'+exe+'.exe','work/'+out],capture_output=True,text=True,timeout=15)
  print(exe,'exit',hex(p.returncode & 0xffffffff),'stdout',p.stdout[:500],'stderr',p.stderr[:500])
 except Exception as e:print(exe,type(e).__name__,str(e)[:300])
