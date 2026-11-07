"""Actual-editor exposure input, GPU output and unchanged-preview regression.

Generated fixtures only for writes. The original benchmark is read, never run
inside original SFM. Pass a workspace output directory containing before.json
and a fresh candidate-editor complete build. No UI automation or SFM control.
"""
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import math
import os
import re
import struct
import subprocess
import sys
import tempfile
from verify_texture_sampling_cross_api import fixture

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def encoded(value):
    value = min(1., max(0., value))
    return math.floor((12.92 * value if value <= .0031308 else 1.055 * value ** (1/2.4)-.055)*255+.5)


def main():
    need(len(sys.argv) == 2, 'Supply an isolated workspace output directory')
    parent = Path(sys.argv[1]).resolve()
    need(parent.is_relative_to(ROOT/'outputs'), 'Workspace outputs required')
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=parent))
    before = json.loads((parent/'before.json').read_text())
    candidate = parent/'candidate-editor/sfm64_session_ui.exe'
    baseline = Path(before['baseline_editor'])
    clean = {k:v for k,v in os.environ.items() if not k.startswith('SFM64_')}
    result = dict(status='IN PROGRESS', checked_utc=datetime.now(timezone.utc).isoformat(), commands=[], captures={}, metrics={})
    foreign = run/'empty-cwd'; foreign.mkdir()

    def execute(args, label, env=None, expected=0):
        process = subprocess.run(list(map(str,args)), cwd=foreign, env=env or clean,
            capture_output=True, text=True, timeout=240, creationflags=subprocess.CREATE_NO_WINDOW)
        text = process.stdout+process.stderr
        (run/(label+'.log')).write_text(text,encoding='utf-8')
        result['commands'].append(dict(arguments=list(map(str,args)), exit_code=process.returncode, expected=expected))
        need(process.returncode == expected, label+' failed: '+text[-1800:])
        return text

    def capture(binary, label, session, content, frame=0, profile=None, extra=None, expected=0):
        out = run/label;out.mkdir()
        env = dict(clean,SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out))
        if profile: env['SFM64_EXPERIMENTAL_HDR2_PROFILE']=str(profile)
        env.update(extra or {})
        text=execute([binary,'--control-test','--session',session,'--content-directory',content,'--start-frame',str(frame)],label,env,expected)
        files={p.name:sha(p) for p in out.glob('*.rgba')}
        if expected: need(not files, 'Refused frame produced captures')
        else: need('model_gpu_presentation_passed=1 frames=5' in text, 'Five actual editor frames '+label)
        result['captures'][label]=files
        print('PASS '+label,flush=True)
        return out,text,files

    try:
        for binary in [candidate,baseline]:
            wire=binary.read_bytes(); pe=struct.unpack_from('<I',wire,60)[0]
            need(wire[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',wire,pe+4)[0]==0x8664 and struct.unpack_from('<H',wire,pe+24)[0]==0x20b,'AMD64 PE32+')
        need(sha(baseline)==before['baseline_sha256'],'Baseline changed')
        result['executables']={name:dict(path=str(p),sha256=sha(p)) for name,p in [('baseline',baseline),('candidate',candidate)]}
        warning=lambda p:Counter(re.findall(r'warning (C\d+): ([^\r\n]+)',(p.parent/'build.log').read_text(errors='replace')))
        need(warning(candidate)==warning(baseline),'New complete-editor warning')
        result['compiler_warning_count']=sum(warning(candidate).values())
        need(not (candidate.parent/'shader-build.log').read_text().strip(),'Shader diagnostics')
        shader_hashes=lambda p:{f.name:sha(f) for f in (p.parent/'shaders').glob('*.spv')}
        need(shader_hashes(candidate)==shader_hashes(baseline),'Unrelated shader changed')
        result['shader_sha256']=shader_hashes(candidate)
        white=bytearray(fixture(0,0)[:88]);struct.pack_into('<HH',white,16,2,2);struct.pack_into('<I',white,20,0x101);struct.pack_into('<H',white,24,1);white[56]=1
        (run/'white.vtf').write_bytes(white+bytes([255]*16))
        execute(['cmd.exe','/d','/c',ROOT/'work/build_session_exposure_test.cmd',run/'cpu-test'],'build-cpu')
        owned=run/'owned'
        text=execute([run/'cpu-test/test_session_exposure_x64.exe',owned,run/'white.vtf',parent/'benchmark-input.dmx'],'cpu-and-fixtures')
        need('PASS session_exposure=1 rejected_graphs=12' in text and 'animation_seek=6' in text,'CPU/input coverage')
        need(text.count('benchmark_tick=')==3,'Evaluated benchmark exposure checks')
        result['cpu_warnings']=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'cpu-test/build.log').read_text(errors='replace'))
        session=owned/'exposure-animated.dmx'; mode=owned/'session-camera.profile'
        reference=owned/'113-nearest-1.profile'
        source_before={str(p.relative_to(owned)):sha(p) for p in owned.rglob('*') if p.is_file()}
        literal_checks=0
        for frame, exposure in [(0,.5),(24,2),(48,8)]:
            manual=run/('manual-'+str(frame)+'.profile');manual.write_text(reference.read_text().replace('exposure 1\n',f'exposure {exposure}\n'))
            actual,log,_=capture(candidate,'session-'+str(frame),session,owned,frame,mode)
            fixed,_,_=capture(candidate,'manual-'+str(frame),session,owned,frame,manual)
            filename=f'vulkan-retained-frame-{frame}.rgba'
            wire=(actual/filename).read_bytes();need(wire==(fixed/filename).read_bytes(),'Session product differs from independently chosen numeric exposure')
            need(f'product={exposure:g} scope=admitted_lightmaps_only' in log,'Evaluated exposure diagnostic')
            colors=[tuple(encoded(v) for v in [.05,.05,.08])+(255,), (encoded(.25*exposure),0,0,255),(0,encoded(.375*exposure),0,255)]
            counts=Counter(); maximum=0
            need(len(wire)==512*512*4,'Owned capture extent')
            for n in range(0,len(wire),4):
                pixel=tuple(wire[n:n+4]);errors=[max(abs(a-b) for a,b in zip(pixel,c)) for c in colors];best=min(range(3),key=errors.__getitem__)
                need(errors[best]<=1,'Literal exposure pixel mismatch '+str(pixel));maximum=max(maximum,errors[best]);counts[best]+=1
            need(all(counts[n]>0 for n in range(3)),'Empty fixture coverage')
            result['metrics'][str(frame)]=dict(product=exposure,pixels=len(wire)//4,counts=dict(counts),max_error_byte=maximum,numeric_reference_byte_exact=True)
            literal_checks+=len(wire)//4
        repeat,_,_=capture(candidate,'session-repeat',session,owned,0,mode)
        need((repeat/'vulkan-retained-frame-0.rgba').read_bytes()==(run/'session-0/vulkan-retained-frame-0.rgba').read_bytes(),'Repeated exposure pixels differ')
        old,_,hashes=capture(baseline,'default-baseline',session,owned)
        _,_,new_hashes=capture(candidate,'default-candidate',session,owned)
        need(hashes==new_hashes,'Default owned rendering changed')
        capture(candidate,'unsupported-exposure',owned/'exposure-invalid.dmx',owned,0,mode,expected=1)
        ticks=run/'blank-ticks.txt';ticks.write_text('0\n10000\n30000\n')
        blank=run/'blank-restore';blank.mkdir()
        blank_env=dict(clean,SFM64_EXPERIMENTAL_HDR2_PROFILE=str(mode),
            SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(blank),
            SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=str(session),SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='0',
            SFM64_DIAGNOSTIC_SCENE_ROOT='1',SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=str(owned),
            SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),SFM64_DIAGNOSTIC_TIMELINE_BLANKS='1',
            SFM64_DIAGNOSTIC_SCENE_PREVIEW='1',SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1')
        blank_log=execute([candidate,'--control-test'],'blank-restore',blank_env)
        need('model_gpu_presentation_passed=1 frames=5' in blank_log,'Blank/restore five frames')
        clear=bytes(tuple(encoded(v) for v in [.05,.05,.08])+(255,))*(512*512)
        need((blank/'vulkan-retained-frame-2.rgba').read_bytes()==clear,'Blank frame reused prior exposure/output')
        need((blank/'vulkan-retained-frame-0.rgba').read_bytes()==(run/'session-0/vulkan-retained-frame-0.rgba').read_bytes(),'Blank restore changed initial exposure')
        result['blank_restore_byte_exact']=True
        # Benchmark reads an isolated session copy; installed content is read-only.
        content=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod')
        proof=json.loads((parent/'benchmark-input-proof.json').read_text());original=Path(proof['original'])
        need(sha(original)==proof['sha256'] and sha(parent/'benchmark-input.dmx')==proof['sha256'],'Benchmark identity changed')
        _,_,old_hashes=capture(baseline,'benchmark-default-baseline',parent/'benchmark-input.dmx',content)
        _,_,new_hashes=capture(candidate,'benchmark-default-candidate',parent/'benchmark-input.dmx',content)
        need(old_hashes==new_hashes,'Default benchmark pixels changed')
        manual,_,manual_hashes=capture(candidate,'benchmark-manual',parent/'benchmark-input.dmx',content,120,reference)
        actual,text,actual_hashes=capture(candidate,'benchmark-session',parent/'benchmark-input.dmx',content,120,mode)
        need(manual_hashes==actual_hashes and 'product=1 scope=admitted_lightmaps_only' in text,'Benchmark unity input route changed pixels')
        need(sha(original)==proof['sha256'],'Original benchmark changed')
        need(all(sha(owned/p)==digest for p,digest in source_before.items()),'Read-only fixture changed')
        result.update(status='VERIFIED bounded evaluated session/camera exposure on admitted HDR2 lightmaps',
            literal_gpu_pixel_checks=literal_checks,benchmark_default_byte_exact=True,benchmark_unity_exposure_byte_exact=True,
            original_benchmark_sha256=proof['sha256'],source_sha256={p:sha(ROOT/p) for p in before['editor_source_dependencies']},
            limits=['Input-product evidence is static analysis of intentionally modified installed IFM; original engine execution is absent.',
                    'Explicit session_camera profile option only; default preview and numeric exposure profiles preserved.',
                    'Only admitted plain/directional HDR2 lightmap batches consume this exposure. Fallback model/material paths and full native post effects remain unverified.',
                    'Zero, negative, nonfinite and unrepresentable products fail explicitly; native automatic exposure is not inferred.',
                    'Swapchain route executes but its pixels are not read back; no Vulkan validation layer is installed.'])
    except Exception as error:
        result.update(status='FAILED',error=str(error));raise
    finally:
        (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
        (parent/'latest.json').write_text(json.dumps(dict(receipt=str(run/'results.json')),indent=2)+'\n')
        print(run/'results.json',flush=True)


if __name__=='__main__':main()
