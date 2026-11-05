"""Reproduce and verify retained map-local cache identity using owned fixtures."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, os, re, shutil, struct, subprocess, tempfile
from verify_texture_sampling_cross_api import fixture as owned_vtf

ROOT=Path(__file__).resolve().parent.parent
VS=Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
CHANGED={'work/retained_session_scene_candidate.h','work/scene_render_inputs_cache_candidate.h','work/test_retained_hdr2_profile.cpp'}
NEW=['test_inline_map_cache.cpp','build_inline_map_cache_test.cmd','verify_inline_map_cache.py']
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(condition,message):
    if not condition:raise RuntimeError(message)
def architecture(p):
    b=Path(p).read_bytes();n=struct.unpack_from('<I',b,60)[0]
    need((struct.unpack_from('<H',b,n+4)[0],struct.unpack_from('<H',b,n+24)[0])==(0x8664,0x20b),'AMD64 PE32+ required '+str(p))

def main():
    parent=Path((ROOT/'outputs/inline-map-cache-current.txt').read_text().strip())
    before=json.loads((parent/'before.json').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent))
    result=dict(status='IN PROGRESS',time_utc=datetime.now(timezone.utc).isoformat(),run=str(run),
        preceding_goal_turn='PROGRESS: retained-session explicit CPU profile verified',commands=[])
    clean={k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args,label,env=None,cwd=ROOT):
        args=list(map(str,args));q=subprocess.run(args,cwd=cwd,env=env,capture_output=True,text=True,
            timeout=900,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(q.stdout+q.stderr)
        result['commands'].append(dict(args=args,exit_code=q.returncode))
        need(q.returncode==0,label+' failed: '+(q.stdout+q.stderr)[-2200:]);return q.stdout+q.stderr
    try:
        for p,h in before.items():need(sha(parent/('before-'+Path(p).name))==h if p in CHANGED else sha(ROOT/p)==h,'protected initial identity '+p)
        old=(parent/'before-test_retained_hdr2_profile.cpp').read_bytes();nl=b'\r\n' if b'\r\n' in old else b'\n'
        expected=old.replace(b'int main(int argc,char**argv){try{',b'#ifndef SFM64_RETAINED_HDR2_PROFILE_NO_MAIN'+nl+b'int main(int argc,char**argv){try{',1)+nl+b'#endif'+nl
        need((ROOT/'work/test_retained_hdr2_profile.cpp').read_bytes()==expected,'existing profile test changed beyond reuse guard')
        vtf=run/'owned.vtf';vtf.write_bytes(owned_vtf(0,0))
        baseline=parent/'baseline/test_inline_map_cache_x64.exe';architecture(baseline)
        proof=json.loads((parent/'baseline-proof.json').read_text())
        need(proof['exit_code']==0 and proof['executable_sha256']==sha(baseline),'stale baseline executable identity')
        result['baseline_reproduction']={}
        for n in range(2):
            text=execute([baseline,run/('stale-'+str(n)),vtf,'--expect-stale'],'baseline-'+str(n))
            need(text.count('REPRODUCED inline_map_cache_stale=1')==2 and 'PASS stale_reproduction=1 profiles=2' in text,'baseline reproduces both modes')
            result['baseline_reproduction'][str(n)]=text
        result['baseline_executable']=dict(path=str(baseline),sha256=sha(baseline))
        print('Original implementation stale cache reproduced twice in legacy/HDR modes',flush=True)
        cpu=run/'cpu';execute(['cmd.exe','/d','/c',ROOT/'work/build_inline_map_cache_test.cmd',cpu],'build-inline')
        exe=cpu/'test_inline_map_cache_x64.exe';architecture(exe)
        logs=[execute([exe,run/('fixed-'+str(n)),vtf],'inline-'+str(n)) for n in range(2)]
        need(logs[0]==logs[1] and 'PASS inline_map_cache=1 profiles=2' in logs[0],'repeatable corrected cache test')
        result['test_executable']=dict(path=str(exe),sha256=sha(exe),architecture='AMD64 PE32+')
        result['test_output']=logs[0];result['warnings']={'inline':re.findall(r'warning (C\d+): ([^\r\n]+)',(cpu/'build.log').read_text(errors='replace'))}
        print(logs[0],flush=True)
        # Fresh prior profile test confirms guard reuse did not remove coverage.
        profile=run/'profile';execute(['cmd.exe','/d','/c',ROOT/'work/build_retained_hdr2_profile_test.cmd',profile],'build-profile')
        binary=profile/'test_retained_hdr2_profile_x64.exe';architecture(binary)
        text=execute([binary,run/'profile-owned',vtf],'profile')
        need('PASS retained_hdr2_profile=1' in text,'profile regression protocol')
        result['profile_regression']=dict(path=str(binary),sha256=sha(binary),output=text)
        result['warnings']['profile']=re.findall(r'warning (C\d+): ([^\r\n]+)',(profile/'build.log').read_text(errors='replace'))
        print(text,flush=True)
        # Existing ordinary asset/override/alpha/mask/missing/malformed cache tests.
        result['material_regressions']={}
        for name in ['scene_material_inputs','selfillum_material_cache']:
            folder=run/name;folder.mkdir();batch=folder/'build.cmd';binary=folder/('test_'+name+'_x64.exe')
            batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',
                f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\test_{name}.cpp /Fo:"{folder/"test.obj"}" /Fe:"{binary}" >"{folder/"build.log"}" 2>&1','exit /b %errorlevel%','']))
            execute(['cmd.exe','/d','/c',batch],'build-'+name);architecture(binary)
            fixtures=folder/'fixtures';fixtures.mkdir()
            text=execute([binary,'--fixtures',fixtures],name);need('PASS ' in text,'owned cache regression protocol '+name)
            result['material_regressions'][name]=dict(path=str(binary),sha256=sha(binary),output=text)
            result['warnings'][name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(folder/'build.log').read_text(errors='replace'))
            print(text,flush=True)
        editor_dir=run/'editor-x64';execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',editor_dir],'complete-editor')
        editor=editor_dir/'sfm64_session_ui.exe';architecture(editor)
        result['editor']=dict(path=str(editor),sha256=sha(editor),architecture='AMD64 PE32+',shader_diagnostics=(editor_dir/'shader-build.log').read_text())
        result['warnings']['editor']=re.findall(r'warning (C\d+): ([^\r\n]+)',(editor_dir/'build.log').read_text(errors='replace'))
        old_editor=ROOT/'outputs/retained-hdr2-profile-vztpdm6v/run-92oxi_vf/editor-x64/sfm64_session_ui.exe'
        shaders={p.name:sha(p) for p in (editor_dir/'shaders').glob('*.spv')}
        need(len(shaders)==17 and all(sha(old_editor.parent/'shaders'/n)==h for n,h in shaders.items()),'17 unchanged shader artifacts')
        result['shader_sha256']=shaders;print('Complete AMD64 editor rebuilt;17 unchanged shader artifacts',flush=True)
        game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
        source=game/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
        pins={str(p):sha(p) for p in [source,game/'tf_movies/maps/mtt_soldier.bsp',game/'usermod/gameinfo.txt']}
        session=run/'benchmark-read-copy.dmx';shutil.copyfile(source,session);foreign=run/'empty-cwd';foreign.mkdir();captures={}
        for label,binary in [('baseline',old_editor),('candidate',editor)]:
            out=run/('captures-'+label);out.mkdir();env=dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out))
            log=execute([binary,'--control-test','--session',session,'--content-directory',game/'usermod','--start-frame','119'],'editor-'+label,env,foreign)
            need('model_gpu_presentation_passed=1 frames=5' in log and 'gpu_readback_copies=5 capture_frames=1' in log,'five owned editor frames '+label)
            captures[label]={p.name:sha(p) for p in out.glob('vulkan-retained-frame-*.rgba')}
            need(all('vulkan-retained-frame-'+str(n)+'.rgba' in captures[label] for n in [0,119,120]),'capture coverage '+label)
            print('Owned editor '+label+' control/capture passed',flush=True)
        need(captures['baseline']==captures['candidate'],'default editor captures changed')
        need(all(sha(p)==h for p,h in pins.items()) and sha(session)==pins[str(source)],'original/disposable inputs changed')
        result['default_editor_gpu']=dict(baseline=str(old_editor),baseline_sha256=sha(old_editor),
            captures_sha256=captures,frames_per_executable=5,originals_read_only=pins)
        for p,h in before.items():
            if p not in CHANGED:need(sha(ROOT/p)==h,'protected final identity '+p)
        result.update(status='VERIFIED map-scoped inline geometry/material cache identity; GPU HDR2 route remains incomplete',
            transitions={p:dict(before=before[p],after=sha(ROOT/p)) for p in CHANGED},
            new_source_sha256={p:sha(ROOT/'work'/p) for p in NEW},protected_sha256={p:h for p,h in before.items() if p not in CHANGED},
            limits=['Retained model/material caches still own borrowed ordinary/inline frame inputs; the scene owner must outlive those frames and GPU cache.',
                    'Cache identities assume immutable files/mounts and an immutable scene dataset/profile; same-path content replacement is not a verified reload mechanism.',
                    'Existing inline index-1 interpretation was neither changed nor certified against native SFM in this cache correction.',
                    'HDR2 remains a CPU profile; no editor GPU profile or new rendering mode enabled.'])
    except Exception as error:
        result.update(status='PARTIALLY VERIFIED',failure=repr(error));raise
    finally:
        path=run/'results.json';path.write_text(json.dumps(result,indent=2)+'\n')
        (parent/'latest.json').write_text(json.dumps(dict(receipt=str(path),sha256=sha(path)),indent=2)+'\n')
    print(json.dumps(dict(status=result['status'],receipt=str(path),editor=result['editor'],warnings={k:len(v) for k,v in result['warnings'].items()}),indent=2))
if __name__=='__main__':main()
