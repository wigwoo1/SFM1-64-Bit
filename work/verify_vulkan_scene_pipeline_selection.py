"""Per-draw prepared recorder preflight/cache and unchanged editor GPU frames."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,os,re,shutil,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parent.parent
CHANGED={'work/vulkan_scene_commands_candidate.h','work/vulkan_prepared_scene_candidate.h'}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def main():
    parent=Path((ROOT/'outputs/vulkan-draw-pipeline-current.txt').read_text());before=json.loads((parent/'before.json').read_text());baseline=Path((parent/'baseline-editor.txt').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent));result={'status':'IN PROGRESS','time_utc':datetime.now(timezone.utc).isoformat(),'commands':[]};clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,env=None,cwd=ROOT):
        args=list(map(str,args));p=subprocess.run(args,cwd=cwd,env=env or clean,capture_output=True,text=True,timeout=900,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr);result['commands'].append({'args':args,'exit_code':p.returncode,'cwd':str(cwd)});need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-2400:]);return p.stdout+p.stderr
    def architecture(p):
        b=p.read_bytes();i=struct.unpack_from('<I',b,60)[0];need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'AMD64 PE32+')
    try:
        for p,h in before.items():
            if p not in CHANGED:need(sha(p)==h,'protected initial identity '+p)
            else:need(sha(parent/('before-'+Path(p).name))==h,'backup identity '+p)
        execute(['cmd.exe','/d','/c',ROOT/'work/build_vulkan_scene_pipeline_selection_test.cmd',run/'cpu'],'build-test');test=run/'cpu/test_vulkan_scene_pipeline_selection_x64.exe';architecture(test)
        logs=[execute([test],'test-'+str(n)) for n in range(2)];need(logs[0]==logs[1] and 'PASS prepared_per_draw_pipeline=1' in logs[0],'repeatable prepared bridge pipeline checks');result['instrumented_test_output']=logs[0];result['test_exe']={'path':str(test),'sha256':sha(test),'architecture':'AMD64 PE32+'};print(logs[0],flush=True)
        editor_dir=run/'editor-x64';execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',editor_dir],'complete-editor');editor=editor_dir/'sfm64_session_ui.exe';architecture(editor)
        result['editor']={'path':str(editor),'sha256':sha(editor),'architecture':'AMD64 PE32+','shader_diagnostics':(editor_dir/'shader-build.log').read_text()};result['warnings']={n:re.findall(r'warning (C\d+): ([^\r\n]+)',p.read_text(errors='replace')) for n,p in [('cpu',run/'cpu/build.log'),('editor',editor_dir/'build.log')]}
        shaders={p.name:sha(p) for p in (editor_dir/'shaders').glob('*.spv')};need(len(shaders)==15 and all(sha(baseline.parent/'shaders'/n)==h for n,h in shaders.items()),'15 identical existing shaders');result['shader_sha256']=shaders;print('Complete AMD64 editor rebuilt;15 unchanged shaders',flush=True)
        source=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx');source_hash=sha(source);session=run/'benchmark-read-copy.dmx';shutil.copyfile(source,session);foreign=run/'empty-cwd';foreign.mkdir();captures={}
        for label,exe in [('baseline',baseline),('candidate',editor)]:
            target=run/('captures-'+label);target.mkdir();env=dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(target));out=execute([exe,'--control-test','--session',session,'--content-directory',source.parents[4]/'usermod','--start-frame','119'],'editor-gpu-'+label,env,foreign);need('model_gpu_presentation_passed=1 frames=5' in out and 'gpu_readback_copies=5 capture_frames=1' in out,'owned editor five-frame capture '+label);captures[label]={p.name:sha(p) for p in target.glob('vulkan-retained-frame-*.rgba')};need(all(('vulkan-retained-frame-'+str(n)+'.rgba') in captures[label] for n in (0,119,120)),'expected captured frames '+label);print('Owned editor GPU '+label+' passed',flush=True)
        need(captures['baseline']==captures['candidate'],'legacy default complete captured pixels changed');need(sha(source)==source_hash and sha(session)==source_hash,'original/input session unchanged');result['default_editor_gpu']={'captures_sha256':captures,'baseline':str(baseline),'frames_per_executable':5,'original_read_only':{'path':str(source),'sha256':source_hash}}
        for p,h in before.items():
            if p not in CHANGED:need(sha(p)==h,'protected final identity '+p)
        result.update(status='VERIFIED prepared per-draw pipeline preflight/cache and unchanged default editor captures; HDR2 sampled draw unfinished',transitions={p:{'before':before[p],'after':sha(ROOT/p)} for p in CHANGED},protected_sha256={p:h for p,h in before.items() if p not in CHANGED},new_source_sha256={p:sha(ROOT/'work'/p) for p in ['test_vulkan_scene_pipeline_selection.cpp','build_vulkan_scene_pipeline_selection_test.cmd','verify_vulkan_scene_pipeline_selection.py']},limits=['New per-face pipeline ordering is instrumented; no sampled HDR2 shader draw or descriptor/UV-stream integration implemented in this milestone.','Real GPU captures cover unchanged default editor rendering only.','Selector must be pure; hooks must not bind a pipeline or submit commands. Caller retains compatible pipeline/resource owners through submission fences.'])
    except Exception as e:result.update(status='PARTIALLY VERIFIED',failure=repr(e));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(result,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps({'receipt':str(p),'sha256':sha(p)},indent=2)+'\n')
    print(json.dumps({'status':result['status'],'receipt':str(p),'editor':result['editor'],'warnings':{k:len(v) for k,v in result['warnings'].items()}},indent=2))
if __name__=='__main__':main()
