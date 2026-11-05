"""Retained editor draw identities: CPU callbacks and actual default editor GPU regression."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, os, re, shutil, struct, subprocess, tempfile

ROOT=Path(__file__).resolve().parent.parent
CHANGED='work/vulkan_model_draw_candidate.h'
NEW=['scene_editor_draw_identity_candidate.h','test_scene_editor_draw_identity.cpp',
     'build_scene_editor_draw_identity_test.cmd','verify_scene_editor_draw_identity.py']
BASELINE=ROOT/'outputs/source-inline-number-bgj5ofe_/run-outw28r1/editor-x64/sfm64_session_ui.exe'

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value,message):
    if not value:raise RuntimeError(message)
def pe(p):
    b=Path(p).read_bytes();i=struct.unpack_from('<I',b,60)[0]
    need(struct.unpack_from('<H',b,i+4)[0]==0x8664 and struct.unpack_from('<H',b,i+24)[0]==0x20b,'AMD64 PE32+ '+str(p))

def main():
    parent=Path((ROOT/'outputs/editor-draw-identity-current.txt').read_text(encoding='utf-8').strip())
    before=json.loads((parent/'before.json').read_text(encoding='utf-8'))
    run=Path(tempfile.mkdtemp(prefix='run-',dir=parent))
    result=dict(status='IN PROGRESS',time_utc=datetime.now(timezone.utc).isoformat(),run=str(run),
                preceding_goal_turn='PROGRESS: inline numbering/admission verified and documented',commands=[],warnings={})
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,env=None,cwd=ROOT):
        args=list(map(str,args));p=subprocess.run(args,cwd=cwd,env=clean if env is None else env,capture_output=True,text=True,timeout=900,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(p.stdout+p.stderr,encoding='utf-8')
        result['commands'].append(dict(args=args,exit_code=p.returncode))
        need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1800:]);return p.stdout+p.stderr
    def compile_test(label,script,exe):
        out=run/label;execute(['cmd.exe','/d','/c',ROOT/'work'/script,out],'build-'+label)
        binary=out/exe;pe(binary)
        result['warnings'][label]=re.findall(r'warning (C\d+): ([^\r\n]+)',(out/'build.log').read_text(errors='replace'))
        return binary
    try:
        backup=parent/'before-vulkan_model_draw_candidate.h'
        need(sha(backup)==before[CHANGED],'backup identity')
        for p,h in before.items():
            if p!=CHANGED:need(sha(ROOT/p)==h,'protected initial '+p)
        old=backup.read_bytes();new=(ROOT/CHANGED).read_bytes()
        include=b'#include "scene_editor_draw_identity_candidate.h"\r\n'
        start=b'const auto retainedDrawIdentities=frozenScene?sceneEditorDrawIdentitiesCandidate'
        at=new.index(start);end=new.index(b'VkCommandBufferBeginInfo beginInfo{};',at)
        guard=b'if(frozenScene){target.push_back(retainedDrawIdentities.at(n));return;}'
        need(new.count(include)==new.count(guard)==1,'editor edit coverage')
        need((new[:at]+new[end:]).replace(include,b'').replace(guard,b'')==old,'unrelated editor bytes changed')
        binary=compile_test('identity','build_scene_editor_draw_identity_test.cmd','test_scene_editor_draw_identity_x64.exe')
        logs=[execute([binary],'identity-'+str(n)) for n in range(2)]
        need(logs[0]==logs[1] and 'PASS editor_retained_draw_identity=1 previous_wrong_ranges=5 bridge_callbacks=5 preserved_depth_order=1 invalid_associations=9' in logs[0],'focused identity test')
        result['identity_test']=dict(path=str(binary),sha256=sha(binary),output=logs[0]);print('Fresh identity test passed twice',flush=True)
        binary=compile_test('pipeline','build_vulkan_scene_pipeline_selection_test.cmd','test_vulkan_scene_pipeline_selection_x64.exe')
        log=execute([binary],'pipeline');need('PASS prepared_per_draw_pipeline=1' in log,'pipeline regression')
        result['pipeline_regression']=dict(path=str(binary),sha256=sha(binary),output=log)
        game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
        source=game/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
        pins={str(p):sha(p) for p in [source,game/'tf_movies/maps/mtt_soldier.bsp',game/'usermod/gameinfo.txt']}
        session=run/'benchmark-read-copy.dmx';shutil.copyfile(source,session)
        editor=run/'editor-x64';execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',editor],'complete-editor')
        binary=editor/'sfm64_session_ui.exe';pe(binary)
        result['editor']=dict(path=str(binary),sha256=sha(binary),architecture='AMD64 PE32+',shader_diagnostics=(editor/'shader-build.log').read_text())
        result['warnings']['editor']=re.findall(r'warning (C\d+): ([^\r\n]+)',(editor/'build.log').read_text(errors='replace'))
        shaders={p.name:sha(p) for p in (editor/'shaders').glob('*.spv')}
        need(len(shaders)==17 and all(sha(BASELINE.parent/'shaders'/n)==h for n,h in shaders.items()),'unchanged17 shaders')
        result['shader_sha256']=shaders;print('Complete AMD64 editor build passed',flush=True)
        foreign=run/'empty-cwd';foreign.mkdir();captures={};identities=[]
        for label,binary in [('baseline',BASELINE),('candidate',binary)]:
            out=run/('captures-'+label);out.mkdir()
            env=dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out))
            log=execute([binary,'--control-test','--session',session,'--content-directory',game/'usermod','--start-frame','119'],'editor-'+label,env,foreign)
            need('model_gpu_presentation_passed=1 frames=5' in log and 'gpu_readback_copies=5 capture_frames=1' in log,'actual five editor GPU frames '+label)
            if label=='candidate':
                matches=re.findall(r'editor_retained_draw_identity=1 draws=(\d+) restored_local_ranges=(\d+)',log)
                need(matches and all(int(a)>0 and int(b)>0 for a,b in matches),'actual editor did not exercise restored local ranges');identities=matches
            captures[label]={p.name:sha(p) for p in out.glob('vulkan-retained-frame-*.rgba')}
            need(all('vulkan-retained-frame-'+str(n)+'.rgba' in captures[label] for n in [0,119,120]),'capture coverage '+label)
            print('Owned editor '+label+' control/capture passed',flush=True)
        need(captures['baseline']==captures['candidate'],'default pixels changed; inspect before another focused attempt')
        need(sha(session)==pins[str(source)] and all(sha(p)==h for p,h in pins.items()),'original/disposable inputs changed')
        for p,h in before.items():
            if p!=CHANGED:need(sha(ROOT/p)==h,'protected final '+p)
        result.update(status='VERIFIED retained editor draw identities and default GPU regression; HDR2 editor GPU route not connected',
            baseline=dict(path=str(BASELINE),sha256=sha(BASELINE)),captures_sha256=captures,editor_identity_counts=identities,
            original_sha256=pins,frames_per_editor=5,transition=dict(path=CHANGED,before=before[CHANGED],after=sha(ROOT/CHANGED)),
            new_source_sha256={p:sha(ROOT/'work'/p) for p in NEW},protected_sha256={p:h for p,h in before.items() if p!=CHANGED},
            limits=['Instrumented callback test verifies original six-field identities and reordered translucent draws; it does not execute a GPU.',
                    'Actual default owned-editor GPU benchmark regression covers five frames and final captures0/119/120.',
                    'HDR2 pipeline/layout/set1/stream1 and every-frame draw-fence integration remain unfinished.',
                    'No original SFM launched/controlled/written; no native pixel equivalence claimed.'])
    except Exception as error:
        result.update(status='PARTIALLY VERIFIED',failure=repr(error));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
        (parent/'latest.json').write_text(json.dumps(dict(receipt=str(p),sha256=sha(p)),indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(status=result['status'],receipt=str(p),editor=result['editor'],warnings={k:len(v) for k,v in result['warnings'].items()}),indent=2))
if __name__=='__main__':main()
