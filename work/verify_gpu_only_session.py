from pathlib import Path
import subprocess,hashlib,json
files=list(Path('outputs').glob('vulkan-retained-frame-*.rgba'))
before={str(p):(hashlib.sha256(p.read_bytes()).hexdigest(),p.stat().st_mtime_ns) for p in files}
result=subprocess.run(['cmd.exe','/d','/c','work\\run_gpu_only_session_probe.cmd'],capture_output=True,text=True,timeout=60)
log=result.stdout+result.stderr
Path('outputs/sfm64-gpu-only-session-validation.txt').write_text(log)
assert result.returncode==0,result.stderr
assert 'model_gpu_presentation_passed=1 frames=61' in log
assert 'gpu_readback_copies=0 capture_frames=0 async_output_writes=0' in log
assert log.count('pixel_verification=disabled')==61
after={str(p):(hashlib.sha256(p.read_bytes()).hexdigest(),p.stat().st_mtime_ns) for p in Path('outputs').glob('vulkan-retained-frame-*.rgba')}
assert before==after,'GPU-only run changed capture files'
Path('outputs/sfm64-gpu-only-session-summary.json').write_text(json.dumps({'presented_frames':61,'gpu_readback_copies':0,'async_output_writes':0,'capture_files_unchanged':True,'source_pixel_verification':'disabled in this run','visible_presentation':'unverified'},indent=2)+'\n')
print('GPU-only session: 61 presented frames, zero image readbacks or file writes, existing captures unchanged')
