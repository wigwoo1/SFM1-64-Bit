from pathlib import Path
import os,subprocess,json
r=subprocess.run(['cmd.exe','/d','/c','work\\build_eye_composition_shader.cmd'],capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
env={k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
shaders=['build/session-core/sfm64-eye-projection-candidate.spv','build/session-core/sfm64-eye-composition-candidate.spv']
r=subprocess.run(['build/session-core/probe_vulkan_graphics_x64.exe',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\anno\mamagen\mamagen',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\materials\models\anno\mamagen\eyes.vtf',*shaders],env=env,capture_output=True,text=True,timeout=60)
log=r.stdout+r.stderr;Path('outputs/sfm64-eye-shader-module-validation.txt').write_text(log);assert r.returncode==0,log
for path in shaders:assert 'source='+path in log
Path('outputs/sfm64-eye-shader-module-summary.json').write_text(json.dumps({'shader_modules_created':shaders,'GPU_module_creation_passed':True,'eye_pipeline_draw':'unfinished','native_shader_parity':'unverified'},indent=2)+'\n');print('Both eye candidate shaders compiled and accepted as Vulkan shader modules; eye draw integration unfinished')
