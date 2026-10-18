from pathlib import Path
import subprocess,json
s=Path('work/run_session_rate_grid_probe.cmd').read_text().replace('roundtrip-11.dmx','invalid-skin-session-v2.dmx').replace('FRAME_COUNT=61','FRAME_COUNT=3')
Path('work/run_invalid_skin_probe.cmd').write_text(s)
r=subprocess.run(['cmd.exe','/d','/c','work\\run_invalid_skin_probe.cmd'],capture_output=True,text=True,timeout=60)
log=r.stdout+r.stderr;Path('outputs/sfm64-invalid-live-skin-validation.txt').write_text(log)
assert r.returncode!=0 and 'material family/slot' in log,log
assert 'model_gpu_presentation_passed=1' not in log
Path('outputs/sfm64-live-skin-selection-summary.json').write_text(json.dumps({'invalid_skin_rejected':True,'material_family_count':1,'alternate_skin_visual_parity':'not tested; current asset has one family','animated_skin_changes':'rejected; material rebuild unfinished'},indent=2)+'\n')
print('Invalid saved skin rejected by material-family validation before model presentation')
