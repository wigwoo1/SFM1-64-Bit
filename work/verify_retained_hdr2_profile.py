"""Verify the explicit CPU profile on actual retained sessions; no original runtime."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, os, re, shutil, struct, subprocess, tempfile
from verify_texture_sampling_cross_api import fixture as owned_vtf

ROOT = Path(__file__).resolve().parent.parent
CHANGED = {'work/scene_hdr2_lightmap_candidate.h', 'work/prepared_session_scene_candidate.h',
           'work/retained_session_scene_candidate.h'}
NEW = ['scene_hdr2_profile_candidate.h', 'test_retained_hdr2_profile.cpp',
       'build_retained_hdr2_profile_test.cmd', 'verify_retained_hdr2_profile.py']

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(value, message):
    if not value: raise RuntimeError(message)
def architecture(p):
    data = Path(p).read_bytes(); offset = struct.unpack_from('<I', data, 60)[0]
    need(struct.unpack_from('<H', data, offset+4)[0] == 0x8664 and
         struct.unpack_from('<H', data, offset+24)[0] == 0x20b, 'AMD64 PE32+ required: '+str(p))

def main():
    parent = Path((ROOT/'outputs/retained-hdr2-profile-current.txt').read_text().strip())
    before = json.loads((parent/'before.json').read_text())
    run = Path(tempfile.mkdtemp(prefix='run-', dir=parent))
    receipt = dict(status='IN PROGRESS', run=str(run), time_utc=datetime.now(timezone.utc).isoformat(),
                   preceding_goal_turn='PROGRESS: controlled filtered HDR2 draws verified', commands=[])
    clean = {k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    def execute(args, label, env=None, cwd=ROOT):
        args = list(map(str, args))
        proc = subprocess.run(args, cwd=cwd, env=env, capture_output=True, text=True,
                              timeout=900, creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.log')).write_text(proc.stdout+proc.stderr)
        receipt['commands'].append(dict(args=args, exit_code=proc.returncode))
        need(proc.returncode == 0, label+' failed: '+(proc.stdout+proc.stderr)[-2200:])
        return proc.stdout+proc.stderr
    try:
        for p,h in before.items():
            need(sha(parent/('before-'+Path(p).name)) == h if p in CHANGED else sha(ROOT/p) == h,
                 'protected initial identity '+p)
        vtf = run/'owned.vtf'; vtf.write_bytes(owned_vtf(0, 0))
        cpu = run/'cpu'
        execute(['cmd.exe','/d','/c',ROOT/'work/build_retained_hdr2_profile_test.cmd',cpu], 'build-profile')
        exe = cpu/'test_retained_hdr2_profile_x64.exe'; architecture(exe)
        outputs = [execute([exe,run/('owned-'+str(n)),vtf], 'profile-'+str(n)) for n in range(2)]
        need(outputs[0] == outputs[1] and 'PASS retained_hdr2_profile=1' in outputs[0], 'repeatable profile test')
        receipt['test_output'] = outputs[0]
        receipt['test_executable'] = dict(path=str(exe),sha256=sha(exe),architecture='AMD64 PE32+')
        print(outputs[0], flush=True)
        # Existing exact CPU arithmetic/encoding/admission and dataset rollback
        # tests are compiled afresh. These test programs execute owned code only.
        regressions = [('scene_hdr2_lightmap','build_scene_hdr2_lightmap_test.cmd'),
                       ('retained_bsp_dataset_selection','build_retained_bsp_dataset_selection_test.cmd'),
                       ('source_bsp_paired_lightmap_selection','build_source_bsp_paired_lightmap_selection_test.cmd')]
        receipt['regressions'] = {}; receipt['warnings'] = {}
        for name,script in regressions:
            folder=run/name
            execute(['cmd.exe','/d','/c',ROOT/'work'/script,folder], 'build-'+name)
            binary=folder/('test_'+name+'_x64.exe'); architecture(binary)
            args=[binary] if name=='source_bsp_paired_lightmap_selection' else [binary,run/('fixture-'+name),vtf]
            result=execute(args,name); need('PASS ' in result,'regression protocol '+name)
            receipt['regressions'][name]=dict(executable=str(binary),sha256=sha(binary),output=result)
            receipt['warnings'][name]=re.findall(r'warning (C\d+): ([^\r\n]+)',(folder/'build.log').read_text(errors='replace'))
            print(result,flush=True)
        editor_dir=run/'editor-x64'
        execute(['cmd.exe','/d','/c',ROOT/'work/build_sfm64_editor_x64.cmd',editor_dir], 'complete-editor')
        editor=editor_dir/'sfm64_session_ui.exe'; architecture(editor)
        receipt['editor']=dict(path=str(editor),sha256=sha(editor),architecture='AMD64 PE32+',
                               shader_diagnostics=(editor_dir/'shader-build.log').read_text())
        for name,log in [('profile',cpu/'build.log'),('editor',editor_dir/'build.log')]:
            receipt['warnings'][name]=re.findall(r'warning (C\d+): ([^\r\n]+)',log.read_text(errors='replace'))
        baseline=ROOT/'outputs/publication-editor-x64-20261105T0150Z/sfm64_session_ui.exe'
        shaders={p.name:sha(p) for p in (editor_dir/'shaders').glob('*.spv')}
        need(len(shaders)==17 and all(sha(baseline.parent/'shaders'/n)==h for n,h in shaders.items()),'17 unchanged shader artifacts')
        receipt['shader_sha256']=shaders
        print('Full editor rebuilt AMD64 PE32+;17 unchanged shader artifacts',flush=True)
        # Hidden owned editor control tests, using a disposable session copy.
        # No original application or DLL is loaded, launched or controlled.
        game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
        source=game/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
        originals=[source,game/'tf_movies/maps/mtt_soldier.bsp',game/'usermod/gameinfo.txt']
        pinned={str(p):sha(p) for p in originals}
        session=run/'benchmark-read-copy.dmx';shutil.copyfile(source,session)
        foreign=run/'empty-cwd';foreign.mkdir();captures={}
        for label,binary in [('baseline',baseline),('candidate',editor)]:
            target=run/('captures-'+label);target.mkdir()
            env=dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(target))
            result=execute([binary,'--control-test','--session',session,'--content-directory',game/'usermod','--start-frame','119'], 'editor-'+label,env,foreign)
            need('model_gpu_presentation_passed=1 frames=5' in result and 'gpu_readback_copies=5 capture_frames=1' in result,'five editor GPU frames '+label)
            captures[label]={p.name:sha(p) for p in target.glob('vulkan-retained-frame-*.rgba')}
            need(all('vulkan-retained-frame-'+str(n)+'.rgba' in captures[label] for n in (0,119,120)), 'frame capture coverage '+label)
            print('Owned editor '+label+' control/capture passed',flush=True)
        need(captures['baseline']==captures['candidate'],'omitted profile changed default editor pixels')
        need(sha(session)==pinned[str(source)] and all(sha(p)==h for p,h in pinned.items()),'original/disposable input changed')
        receipt['default_editor_gpu']=dict(captures_sha256=captures,baseline=str(baseline),
            baseline_sha256=sha(baseline),frames_per_executable=5,originals_read_only=pinned)
        for p,h in before.items():
            if p not in CHANGED: need(sha(ROOT/p)==h,'protected final identity '+p)
        receipt.update(status='VERIFIED retained-session explicit CPU profile/frame ownership; GPU profile NOT CONNECTED',
            transitions={p:dict(before=before[p],after=sha(ROOT/p)) for p in CHANGED},
            new_source_sha256={p:sha(ROOT/'work'/p) for p in NEW},protected_sha256={p:h for p,h in before.items() if p not in CHANGED},
            limits=['No editor CLI/UI option or GPU selection for HDR2 is enabled.',
                    'New profile/plan ownership is CPU only; submitted-frame GPU fences still require integration.',
                    'Per-face padded pages are explicit experimental policy, not native atlas equivalence.',
                    'Default GPU captures prove regression only; broader SFM compatibility remains incomplete.'])
    except Exception as error:
        receipt.update(status='PARTIALLY VERIFIED',failure=repr(error)); raise
    finally:
        path=run/'results.json';path.write_text(json.dumps(receipt,indent=2)+'\n')
        (parent/'latest.json').write_text(json.dumps(dict(receipt=str(path),sha256=sha(path)),indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],receipt=str(path),editor=receipt['editor'],
                         warnings={k:len(v) for k,v in receipt['warnings'].items()}),indent=2))
if __name__=='__main__': main()
