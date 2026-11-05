"""Explicit HDR2 editor route, real GPU frame lifetime and default regression."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib,json,os,re,shutil,struct,subprocess,tempfile
from verify_texture_sampling_cross_api import fixture as vtf_fixture
from verify_vulkan_hdr2_indexed_draw import oracle
ROOT=Path(__file__).resolve().parent.parent
CHANGED={'work/vulkan_model_draw_candidate.h','work/vulkan_graphics_pipeline_candidate.h','work/probe_vulkan_win32_surface_x64.cpp'}
NEW=['editor_hdr2_profile_candidate.h','vulkan_editor_hdr2_frame_candidate.h','test_editor_hdr2_profile.cpp','build_editor_hdr2_profile_test.cmd',
     'probe_vulkan_editor_hdr2_frame_x64.cpp','build_vulkan_editor_hdr2_frame_probe.cmd','verify_editor_hdr2_route.py']
BASE=ROOT/'outputs/editor-draw-identity-gtxkv0ay/run-0fzhd5h2/editor-x64/sfm64_session_ui.exe'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def pe(p):
    b=Path(p).read_bytes();i=struct.unpack_from('<I',b,60)[0];need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'AMD64 PE32+')
def main():
    parent=Path((ROOT/'outputs/editor-hdr2-route-current.txt').read_text().strip());before=json.loads((parent/'before.json').read_text())
    run=Path(tempfile.mkdtemp(prefix='run-',dir=parent));r=dict(status='IN PROGRESS',time_utc=datetime.now(timezone.utc).isoformat(),run=str(run),preceding_goal_turn='PROGRESS: retained editor draw identity verified',commands=[],warnings={})
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,env=None,cwd=ROOT,expected=0):
        args=list(map(str,args));p=subprocess.run(args,cwd=cwd,env=clean if env is None else env,capture_output=True,text=True,timeout=900,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(p.stdout+p.stderr,encoding='utf-8');r['commands'].append(dict(args=args,exit_code=p.returncode,expected_exit=expected))
        need(p.returncode==expected,label+' failed: '+(p.stdout+p.stderr)[-1600:]);return p.stdout+p.stderr
    def build(label,script,name):
        out=run/label;execute(['cmd.exe','/d','/c',ROOT/'work'/script,out],'build-'+label);p=out/name;pe(p)
        r['warnings'][label]=re.findall(r'warning (C\d+): ([^\r\n]+)',(out/'build.log').read_text(errors='replace'));return p
    def capture_editor(binary,label,args,extra=None,hdr=False):
        out=run/('captures-'+label);out.mkdir();env=dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out));env.update(extra or {})
        log=execute([binary,*args],label,env,foreign)
        need('model_gpu_presentation_passed=1 frames=5' in log and 'gpu_readback_copies=5 capture_frames=1' in log,'five GPU frames '+label)
        if hdr:need(log.count('editor_hdr2_draw_fence_completed=')==5 and 'editor_hdr2_pipeline=1 shared_sets=2 push_bytes=160 stream1_uv=1' in log,'HDR2 every draw fence '+label)
        captures={p.name:sha(p) for p in out.glob('vulkan-retained-frame-*.rgba')};need(captures,'missing actual editor captures '+label)
        r.setdefault('editor_runs',{})[label]=dict(captures=captures,hdr_frames=re.findall(r'editor_hdr2_frame=(\d+) faces=(\d+) opaque_batches=(\d+) translucent_batches=(\d+)',log))
        print('Actual editor '+label+' passed',flush=True);return out,log,captures
    try:
        for p,h in before.items():need(sha(parent/('before-'+Path(p).name))==h if p in CHANGED else sha(ROOT/p)==h,'initial identity '+p)
        b=bytearray(vtf_fixture(0,0)[:88]);struct.pack_into('<HH',b,16,2,2);struct.pack_into('<I',b,20,0x101);struct.pack_into('<H',b,24,1);b[56]=1
        white=run/'white.vtf';white.write_bytes(b+bytes([255]*16));quadrants=run/'quadrants.vtf';quadrants.write_bytes(b+bytes([255,255,255,255,255,0,0,255,0,255,0,255,0,0,255,255]))
        exe=build('profile','build_editor_hdr2_profile_test.cmd','test_editor_hdr2_profile_x64.exe');logs=[execute([exe,run/('owned-'+str(n)),white],'profile-'+str(n)) for n in range(2)]
        need(logs[0]==logs[1] and 'PASS editor_hdr2_explicit_profile=1 invalid_profiles=16 fixture_roundtrip=2' in logs[0],'profile parser/owned fixtures')
        r['profile_test']=dict(path=str(exe),sha256=sha(exe),output=logs[0]);owned=run/'owned-0';owned_hashes={str(p.relative_to(owned)):sha(p) for p in owned.rglob('*') if p.is_file()}
        exe=build('gpu','build_vulkan_editor_hdr2_frame_probe.cmd','probe_vulkan_editor_hdr2_frame_x64.exe')
        editor=build('editor-x64','build_sfm64_editor_x64.cmd','sfm64_session_ui.exe');r['editor']=dict(path=str(editor),sha256=sha(editor),architecture='AMD64 PE32+',shader_diagnostics=(editor.parent/'shader-build.log').read_text())
        r['shader_sha256']={p.name:sha(p) for p in (editor.parent/'shaders').glob('*.spv')};need(len(r['shader_sha256'])==17 and all(sha(BASE.parent/'shaders'/n)==h for n,h in r['shader_sha256'].items()),'unchanged17 shaders');print('Complete AMD64 editor built',flush=True)
        metrics=[];outputs=[]
        for repeat in range(2):
            out=run/('gpu-captures-'+str(repeat));outputs.append(execute([exe,editor.parent/'shaders',out,quadrants],'gpu-'+str(repeat)))
            need('PASS editor_hdr2_gpu_frame_cases=4 repeated_after_upload_ready=1 partial_discards=8 invalid_transitions=100' in outputs[-1],'GPU frame coverage');need(re.search(r'prepare_refusals=[1-9]\d*',outputs[-1]),'GPU preparation failure coverage')
            current={}
            for encoding in (113,36):
                for filtering in ('nearest','linear'):current[f'{encoding}-{filtering}']=oracle(out/f'{encoding}-{filtering}/hdr2.rgba',encoding,0,True)
            metrics.append(current);print('GPU lifetime and full image oracle repeat'+str(repeat)+' passed',flush=True)
        need(outputs[0]==outputs[1] and metrics[0]==metrics[1],'repeated GPU result mismatch');r['gpu_test']=dict(path=str(exe),sha256=sha(exe),output=outputs[0],oracle=metrics[0])
        foreign=run/'empty-cwd';foreign.mkdir()
        args=['--control-test','--session',owned/'owned.dmx','--content-directory',owned]
        for name,profile,session in [('half-nearest','113-nearest-1.profile','owned.dmx'),('half-exposure2','113-nearest-2.profile','owned.dmx'),('numeric-linear','36-linear-1.profile','owned.dmx'),('all-unadmitted','113-nearest-1.profile','unavailable.dmx')]:
            _,log,_=capture_editor(editor,name,['--control-test','--session',owned/session,'--content-directory',owned],dict(SFM64_EXPERIMENTAL_HDR2_PROFILE=str(owned/profile)),True)
            if name=='all-unadmitted':need('faces=0 ' in log,'all-unadmitted editor admission')
            else:need('faces=3 ' in log,'owned editor lit faces')
        # Boundary controls visit a real gap at frame2, restore frame0, then step.
        ticks=run/'ticks.txt';ticks.write_text('0\n416\n10000\n')
        _,log,_=capture_editor(editor,'blank-restore',['--control-test'],dict(SFM64_EXPERIMENTAL_HDR2_PROFILE=str(owned/'113-nearest-1.profile'),
            SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=str(owned/'owned.dmx'),SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='0',SFM64_DIAGNOSTIC_SCENE_ROOT='1',
            SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=str(owned),SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_TIMELINE_BLANKS='1',
            SFM64_DIAGNOSTIC_SCENE_PREVIEW='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1'),True)
        need('editor_hdr2_frame=2 faces=0 opaque_batches=0 translucent_batches=0' in log,'actual editor gap visit')
        # Invalid startup profile fails before committing a route/window run.
        bad=execute([editor,*args],'invalid-profile',dict(clean,SFM64_EXPERIMENTAL_HDR2_PROFILE=str(owned/'absent.profile')),foreign,1)
        need('HDR2 profile file unavailable' in bad,'invalid profile exact refusal')
        game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');source=game/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
        pins={str(p):sha(p) for p in [source,game/'tf_movies/maps/mtt_soldier.bsp',game/'usermod/gameinfo.txt']};session=run/'benchmark-read-copy.dmx';shutil.copyfile(source,session)
        args=['--control-test','--session',session,'--content-directory',game/'usermod','--start-frame','119']
        _,_,baseline=capture_editor(BASE,'benchmark-baseline',args);_,_,candidate=capture_editor(editor,'benchmark-default',args)
        need(baseline==candidate and all(f'vulkan-retained-frame-{n}.rgba' in candidate for n in [0,119,120]),'default preview regression')
        _,log,_=capture_editor(editor,'benchmark-hdr2',args,dict(SFM64_EXPERIMENTAL_HDR2_PROFILE=str(owned/'113-nearest-1.profile')),True)
        need('faces=1528 ' in log,'benchmark HDR2 admission coverage')
        need(sha(session)==pins[str(source)] and all(sha(p)==h for p,h in pins.items()),'original/disposable file changed')
        need(all(sha(owned/p)==h for p,h in owned_hashes.items()),'owned input mutated')
        for p,h in before.items():
            if p not in CHANGED:need(sha(ROOT/p)==h,'protected final '+p)
        r.update(status='VERIFIED explicit experimental HDR2 editor route, controlled GPU pixels and draw-fence lifetime; native visual parity incomplete',
            baseline=dict(path=str(BASE),sha256=sha(BASE)),transitions={p:dict(before=before[p],after=sha(ROOT/p)) for p in CHANGED},
            new_source_sha256={p:sha(ROOT/'work'/p) for p in NEW},protected_sha256={p:h for p,h in before.items() if p not in CHANGED},original_sha256=pins,owned_input_sha256=owned_hashes,
            limits=['Explicit startup profile; not native exposure/atlas/tone-map negotiation or a completed SFM renderer.',
                    'Only admitted plain static style0 non-displaced opaque faces use HDR2; unsupported batches retain preview fallback.',
                    'Full controlled GPU image oracle covers8192 channels per repeat with maximum one-LSB bound from the existing independent oracle.',
                    'Actual editor/benchmark route and controls tested without computer use; original SFM never launched/controlled/written.',
                    'No Vulkan validation layer enabled; real resource failures/state/discard paths instrumented in controlled GPU probe.'])
    except Exception as error:r.update(status='PARTIALLY VERIFIED',failure=repr(error));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8');(parent/'latest.json').write_text(json.dumps(dict(receipt=str(p),sha256=sha(p)),indent=2)+'\n')
    print(json.dumps(dict(status=r['status'],receipt=str(p),editor=r['editor'],warnings={k:len(v) for k,v in r['warnings'].items()}),indent=2))
if __name__=='__main__':main()
