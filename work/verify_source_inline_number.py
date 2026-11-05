"""Verify direct inline model numbering; original DLLs are static data only."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, os, re, shutil, struct, subprocess, tempfile
from audit_source_inline_numbering import audit, PIN, FILES
from verify_texture_sampling_cross_api import fixture as owned_vtf

ROOT=Path(__file__).resolve().parent.parent
VS=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
CHANGED={'work/retained_session_scene_candidate.h','work/inspect_source_bsp_inline_model.cpp',
         'work/test_source_bsp_inline_brush.cpp','work/test_inline_map_cache.cpp',
         'work/source_bsp_inline_brush_candidate.h','work/prepared_session_scene_candidate.h','work/vulkan_model_draw_candidate.h'}
NEW=['source_bsp_inline_model_name_candidate.h','audit_source_inline_numbering.py',
     'test_source_inline_number.cpp','build_source_inline_number_test.cmd','verify_source_inline_number.py']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,message):
    if not x:raise RuntimeError(message)
def pe(p):
    b=Path(p).read_bytes();i=struct.unpack_from('<I',b,60)[0]
    need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'AMD64 PE32+ '+str(p))
def main():
    parent=Path((ROOT/'outputs/source-inline-number-current.txt').read_text().strip());before=json.loads((parent/'before.json').read_text())
    run=Path(tempfile.mkdtemp(prefix='run-',dir=parent));result=dict(status='IN PROGRESS',time_utc=datetime.now(timezone.utc).isoformat(),
        run=str(run),preceding_goal_turn='PROGRESS: installed interface/loader targets found; transient host command failures recovered',commands=[])
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,env=None,cwd=ROOT):
        args=list(map(str,args));q=subprocess.run(args,cwd=cwd,env=env,capture_output=True,text=True,timeout=900,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(q.stdout+q.stderr);result['commands'].append(dict(args=args,exit_code=q.returncode))
        need(q.returncode==0,label+' failed: '+(q.stdout+q.stderr)[-2200:]);return q.stdout+q.stderr
    def compile_one(name):
        out=run/name;out.mkdir();exe=out/(name+'_x64.exe');batch=out/'build.cmd'
        batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',
            f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{name}.cpp /Fo:"{out/"test.obj"}" /Fe:"{exe}" >"{out/"build.log"}" 2>&1','exit /b %errorlevel%','']))
        execute(['cmd.exe','/d','/c',batch],'build-'+name);pe(exe)
        result['warnings'][name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(out/'build.log').read_text(errors='replace'));return exe
    try:
        for p,h in before.items():need(sha(parent/('before-'+Path(p).name))==h if p in CHANGED else sha(ROOT/p)==h,'protected initial '+p)
        old=(parent/'before-test_inline_map_cache.cpp').read_text();new=(ROOT/'work/test_inline_map_cache.cpp').read_text()
        need(re.findall(r'check\((.*?)\);',old,re.S)==re.findall(r'check\((.*?)\);',new,re.S),'existing cache assertion changed')
        static=audit(run/'static-evidence');need(static['complete_instruction_count']==454,'complete installed static spans')
        result['static_evidence']=dict(path=str(run/'static-evidence/evidence.json'),sha256=sha(run/'static-evidence/evidence.json'),
            instructions=454,admission=static['unavailable_inline_admission'],conclusion=static['conclusion'],originals=static['originals'])
        vtf=run/'owned.vtf';vtf.write_bytes(owned_vtf(0,0));result['warnings']={}
        old_exe=parent/'baseline/test_source_inline_number_x64.exe';proof=json.loads((parent/'baseline-proof.json').read_text());pe(old_exe)
        need(proof['exit_code']==0 and sha(old_exe)==proof['executable_sha256'],'before-fix executable identity')
        traces=[execute([old_exe,run/('before-'+str(n)),vtf,'--expect-previous'],'before-'+str(n)) for n in range(2)]
        need(traces[0]==traces[1] and traces[0].count('REPRODUCED previous_inline_number_minus_one=1')==2,'old off-by-one reproduction')
        result['before_executable']=dict(path=str(old_exe),sha256=sha(old_exe),output=traces[0]);print('Before-fix wrong-record/upper-bound behavior reproduced twice',flush=True)
        out=run/'cpu';execute(['cmd.exe','/d','/c',ROOT/'work/build_source_inline_number_test.cmd',out],'build-number')
        exe=out/'test_source_inline_number_x64.exe';pe(exe)
        traces=[execute([exe,run/('number-'+str(n)),vtf],'number-'+str(n)) for n in range(2)]
        need(traces[0]==traces[1] and 'PASS source_inline_number=1 profiles=2 malformed_or_unsupported_refusals=16 unavailable_inline_diagnostics=2' in traces[0],'corrected record selection/bounds')
        result['number_executable']=dict(path=str(exe),sha256=sha(exe),output=traces[0]);print(traces[0],flush=True)
        result['warnings']['number']=re.findall(r'warning (C\d+): ([^\r\n]+)',(out/'build.log').read_text(errors='replace'))
        result['regressions']={}
        for name,script in [('inline_map_cache','build_inline_map_cache_test.cmd'),('retained_hdr2_profile','build_retained_hdr2_profile_test.cmd')]:
            out=run/name;execute(['cmd.exe','/d','/c',ROOT/'work'/script,out],'build-'+name);binary=out/('test_'+name+'_x64.exe');pe(binary)
            text=execute([binary,run/('owned-'+name),vtf],name);need('PASS ' in text,'regression protocol '+name)
            result['regressions'][name]=dict(path=str(binary),sha256=sha(binary),output=text)
            result['warnings'][name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(out/'build.log').read_text(errors='replace'));print(text,flush=True)
        game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');bsp=game/'tf_movies/maps/mtt_soldier.bsp'
        source=game/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
        pins={str(p):sha(p) for p in [bsp,source,game/'usermod/gameinfo.txt',*FILES.values()]}
        audit_copy=run/'benchmark-admission-copy.dmx';shutil.copyfile(source,audit_copy)
        text=execute([exe,'--audit-session',audit_copy],'benchmark-admission')
        need('PASS benchmark_unavailable_inline_nodes=13 effects_absent=13' in text and sha(audit_copy)==pins[str(source)],'original-session unavailable-inline admission schema')
        result['benchmark_admission']=dict(input=str(audit_copy),sha256=sha(audit_copy),output=text)
        # Independent bytes establish the last valid record and its upper bound.
        # The historical *17 assumption was wrong: this file has records0..16.
        data=bsp.read_bytes();offset,length=struct.unpack_from('<II',data,8+14*16);count=length//48
        need(length%48==0 and count==17 and offset+length<=len(data),'pinned benchmark model-table extent')
        first,faces=struct.unpack_from('<ii',data,offset+16*48+40)
        inspector=compile_one('inspect_source_bsp_inline_model')
        text=execute([inspector,bsp,'*16'],'inspect-benchmark')
        need('direct_model_lump_index=16' in text and f'model_count={count} ' in text and f'first_face={first} face_count={faces} ' in text,'benchmark record16 interpretation')
        bad_args=[str(inspector),str(bsp),'*17']
        bad=subprocess.run(bad_args,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/'inspect-benchmark-upper-bound.log').write_text(bad.stdout+bad.stderr)
        result['commands'].append(dict(args=bad_args,exit_code=bad.returncode,expected_failure=True))
        need(bad.returncode==1 and bad.stderr.strip()=='inline model index outside BSP model lump','benchmark *17 must be rejected')
        result['benchmark_inspector']=dict(path=str(inspector),sha256=sha(inspector),output=text,
            independent_record16=dict(first_face=first,face_count=faces,model_count=count),
            rejected_name='*17',rejection_output=bad.stdout+bad.stderr)
        legacy=compile_one('test_source_bsp_inline_brush');text=execute([legacy,bsp],'legacy-brush')
        need('inline_brush_model=16 ' in text,'existing numeric reader index16 regression')
        result['regressions']['direct_numeric_reader']=dict(path=str(legacy),sha256=sha(legacy),output=text);print(text,flush=True)
        editor_dir=run/'editor-x64';execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',editor_dir],'complete-editor')
        editor=editor_dir/'sfm64_session_ui.exe';pe(editor);result['editor']=dict(path=str(editor),sha256=sha(editor),architecture='AMD64 PE32+',shader_diagnostics=(editor_dir/'shader-build.log').read_text())
        result['warnings']['editor']=re.findall(r'warning (C\d+): ([^\r\n]+)',(editor_dir/'build.log').read_text(errors='replace'))
        baseline=ROOT/'outputs/inline-map-cache-htm7e4tx/run-cv287hg5/editor-x64/sfm64_session_ui.exe'
        shaders={p.name:sha(p) for p in (editor_dir/'shaders').glob('*.spv')}
        need(len(shaders)==17 and all(sha(baseline.parent/'shaders'/n)==h for n,h in shaders.items()),'17 unchanged shaders');result['shader_sha256']=shaders
        print('Complete AMD64 editor built;17 unchanged shaders',flush=True)
        session=run/'benchmark-read-copy.dmx';shutil.copyfile(source,session);foreign=run/'empty-cwd';foreign.mkdir();captures={}
        for label,binary in [('baseline',baseline),('candidate',editor)]:
            out=run/('captures-'+label);out.mkdir();env=dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out))
            text=execute([binary,'--control-test','--session',session,'--content-directory',game/'usermod','--start-frame','119'],'editor-'+label,env,foreign)
            need('model_gpu_presentation_passed=1 frames=5' in text and 'gpu_readback_copies=5 capture_frames=1' in text,'five editor frames '+label)
            if label=='candidate':need('unavailable_inline_model=*17' in text,'candidate did not diagnose benchmark unavailable model')
            captures[label]={p.name:sha(p) for p in out.glob('vulkan-retained-frame-*.rgba')};need(all('vulkan-retained-frame-'+str(n)+'.rgba' in captures[label] for n in [0,119,120]),'capture coverage '+label)
            print('Owned editor '+label+' control/capture passed',flush=True)
        need(captures['baseline']==captures['candidate'],'default benchmark captures changed; inspect actual differences rather than adjust expectations')
        need(sha(session)==pins[str(source)] and all(sha(p)==h for p,h in pins.items()),'original/disposable inputs changed')
        result['default_editor_gpu']=dict(baseline=str(baseline),baseline_sha256=sha(baseline),captures_sha256=captures,frames_per_executable=5,originals_read_only=pins)
        for p,h in before.items():
            if p not in CHANGED:need(sha(ROOT/p)==h,'protected final identity '+p)
        result.update(status='VERIFIED installed inline numbering and effects-absent unavailable-model admission; full native rendering parity incomplete',
            transitions={p:dict(before=before[p],after=sha(ROOT/p)) for p in CHANGED},new_source_sha256={p:sha(ROOT/'work'/p) for p in NEW},
            protected_sha256={p:h for p,h in before.items() if p not in CHANGED},
            limits=['Native evidence is static file analysis; original engine code never executed.',
                    'Malformed names remain strictly rejected rather than matching the original permissive parser/fatal errors.',
                    'Effects-absent unavailable inline models preserve their document node and get a diagnostic without drawing; effects fallback remains unsupported.',
                    'Existing inline transforms/lighting and complete SFM GPU fidelity are not certified by numbering tests.',
                    'Default captures prove regression at measured frames; the HDR2 editor GPU profile remains disabled.'])
    except Exception as error:
        result.update(status='PARTIALLY VERIFIED',failure=repr(error));raise
    finally:
        path=run/'results.json';path.write_text(json.dumps(result,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps(dict(receipt=str(path),sha256=sha(path)),indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],receipt=str(path),editor=result['editor'],warnings={k:len(v) for k,v in result['warnings'].items()}),indent=2))
if __name__=='__main__':main()
