"""Authenticated Phong instance ambient binding and complete receiver, uploads observed.

Private original shader extraction is local evidence only. No SFM process/GPU use.
"""
from datetime import datetime, timezone
import hashlib
import itertools
import json
from pathlib import Path
import random
import re
import struct
import subprocess
import tempfile

from verify_session_preview_aspect import ROOT, VS, source_closure
from verify_ifm_draw_packet import check_ranges
from vcs6_shader_cache_candidate import Vcs6ShaderCache
from inspect_generic_ambient_shader import disassemble

GAME = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
OUT = ROOT / 'outputs/phong-diffuse-contract-20261030'


def need(value, reason):
    if not value:
        raise RuntimeError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def expected(row):
    serial, order, present, flags, index, count, *values = row
    rgb, cached = values[:18], values[18:]
    ops = [[(1,index),(0,21)], [(0,21),(1,index)], [(1,index),(1,index+1)], [(0,21)], [(1,index)], []][order]
    events = []
    expansions = 0
    for pixel, register in ops:
        if not present:
            continue
        if not flags & 1:
            cached = []
            for face in range(6):
                first = rgb[3*face]
                if first & 0x7f800000 == 0x7f800000 and first & 0x7fffff:
                    first |= 0x400000
                cached.extend([first, *rgb[3*face+1:3*face+3], 0x3f800000])
            flags |= 1
            expansions += 1
        events.append([pixel, register, *cached])
    return struct.pack('<6I', serial, flags, expansions, int(bool(events)), 1, len(events)) + struct.pack('<24I', *cached) + b''.join(struct.pack('<26I', *e) for e in events)


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    (OUT / 'active-run.txt').write_text(str(run), encoding='utf-8')
    prior_path = ROOT / 'outputs/default-lighting-handoff-20261030/latest-reference-policy.json'
    prior_latest = json.loads(prior_path.read_text())
    need(sha(prior_latest['receipt']) == prior_latest['receipt_sha256'], 'reference-profile receipt')
    prior = json.loads(Path(prior_latest['receipt']).read_text())
    protected = dict(prior['protected_sha256'])
    for group in ('protected_sha256', 'source_sha256'):
        need(all(sha(p) == h for p, h in prior[group].items()), 'current prior evidence: ' + group)
    api = GAME / 'bin/shaderapidx9.dll'
    std = GAME / 'bin/stdshader_dx9.dll'
    cache_path = GAME / 'platform/shaders/fxc/phong_ps20b.vcs'
    protected[str(cache_path)] = sha(cache_path)
    need(protected[str(cache_path)] == 'e22f0dedc743bd61590044032835e56382fbf92a2fa1d57decac70cc541238cc', 'PS20b cache pin')
    ranges = {}
    for path, dump, spans in [
        (std, 'stdshader_dx9-disassembly.txt', [('Phong_pixel_register_base',0x1003846b,0x10038470), ('Phong_instance_requests',0x10038fb1,0x1003901f),
             ('begin_instance_list',0x100654a0,0x100654b2), ('ambient_pixel_request',0x100654c0,0x100654eb),
             ('pixel_lights_request',0x100654f0,0x1006551b), ('ambient_vertex_request',0x10065520,0x10065533)]),
        (api, 'shaderapidx9-disassembly.txt', [('receiver_prologue',0x10021430,0x10021586),
             ('pixel_ambient_case7',0x10021724,0x10021793), ('vertex_ambient_case6',0x10021793,0x10021800),
             ('ambient_expansion',0x10012140,0x1001220d), ('receiver_finish',0x10025158,0x10025184),
             ('vertex_upload_boundary',0x10010d70,0x10010e8c), ('pixel_upload_boundary',0x10010e90,0x10010fa2)])]:
        image, rows = check_ranges(path, dump, spans)
        ranges[str(path)] = [{k:v for k,v in row.items() if k != 'instructions'} for row in rows]
        if path == api:
            need(image.word(0x10025184+7*4) == 0x10021724 and image.word(0x10025184+6*4) == 0x10021793, 'instance opcode dispatch')
    # Extract one explicit primary block without decoding all cache permutations.
    data = cache_path.read_bytes()
    count = struct.unpack_from('<I',data,20)[0]
    entries = [struct.unpack_from('<II',data,28+8*i) for i in range(count)]
    need(entries[0][0] == 0, 'controlled primary0 exists')
    payload = data[entries[0][1]:entries[1][1]]
    fixture = data[:20]+struct.pack('<II',2,struct.unpack_from('<I',data,24)[0])+struct.pack('<II',0,48)+struct.pack('<II',0xffffffff,48+len(payload))+struct.pack('<I',0)+payload
    shader = Vcs6ShaderCache(fixture).shader(0,0)
    need(hashlib.sha256(shader).hexdigest() == '451e14fc8a8a82c0f3b9d1b82baf432d3d718f7bc7d0985969b9779e6717debc', 'controlled shader identity')
    asm = disassemble(shader)
    need(re.search(r'//\s+cAmbientCube\s+c4\s+6\s*$',asm,re.M), 'compiled pixel ambient c4..c9')
    need(re.search(r'//\s+NormalMapSampler\s+s3\s+1\s*$',asm,re.M) and 'nrm r2.xyz, r1' in asm, 'normal-mapped pixel shader input')
    (run/'controlled-phong-ps20b-s0-d0.bin').write_bytes(shader)
    (run/'controlled-phong-ps20b-s0-d0.asm').write_text(asm,encoding='utf-8')
    sources = source_closure()
    for name in ['probe_phong_ambient_receiver.cpp',Path(__file__).name,'generic_ambient_expansion_candidate.h',
                 'native_private_world_image_fixture.h','private_x86_boundary_observer_fixture.h','vcs6_shader_cache_candidate.py','inspect_generic_ambient_shader.py',
                 'verify_ifm_draw_packet.py','pe_image.py','verify_dag_world_position.py']:
        p = ROOT/'work'/name
        sources[str(p)] = sha(p)
    commands = ['@echo off',f'cd /d "{ROOT}"']
    for arch, vcvars in [('x86', VS.parent/'vcvars32.bat'),('x64',VS)]:
        commands += [f'call "{vcvars}" >nul','if errorlevel 1 exit /b 1',
            f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_phong_ambient_receiver.cpp /Fo:"{run/(arch+".obj")}" /Fe:"{run/("ambient_receiver_"+arch+".exe")}" >"{run/(arch+"-build.log")}" 2>&1',
            'if errorlevel 1 exit /b 1']
    commands += ['exit /b 0','']
    batch = run/'build.cmd'
    batch.write_text('\n'.join(commands),encoding='utf-8')
    def execute(args, log):
        p = subprocess.run([str(x) for x in args],cwd=ROOT,capture_output=True,text=True,timeout=90)
        log.write_text(p.stdout+p.stderr,encoding='utf-8')
        need(p.returncode == 0,f'{log}: exit {p.returncode}: {(p.stdout+p.stderr)[-1500:]}')
        return p.stdout
    execute(['cmd.exe','/d','/c',batch],run/'build-driver.log')
    rng = random.Random(2026103007)
    patterns = [0,0x80000000,1,0x007fffff,0x3f000000,0x7f800000,0xff800000,0x7f800001,0xff800001,0x7fc12345,0xffffffff]
    rows = []
    def append(order,present,flags,index,count):
        serial=len(rows)
        rgb=[patterns[(serial+k)%len(patterns)] for k in range(18)] if serial%3==0 else [rng.getrandbits(32) for _ in range(18)]
        cached=[rng.getrandbits(32) for _ in range(24)]
        rows.append([serial,order,present,flags,index,count&0xffffffff,*rgb,*cached])
    for order,present,flags,index,count in itertools.product(range(6),range(2),[0,1,2,3,0x80,0xff],[0,4,95,217],[-2147483648,-1,0,4]):
        append(order,present,flags,index,count)
    for flags,order,present in itertools.product(range(256),[0,1],range(2)):
        append(order,present,flags,4,rng.getrandbits(32))
    for _ in range(2048):
        append(rng.randrange(6),rng.randrange(2),rng.randrange(256),rng.randrange(218),rng.getrandbits(32))
    query = run/'queries.bin'
    query.write_bytes(b''.join(struct.pack('<48I',*row) for row in rows))
    want=b''.join(expected(row) for row in rows)
    binaries={}
    for arch,machine,magic in [('x86',0x14c,0x10b),('x64',0x8664,0x20b)]:
        exe=run/('ambient_receiver_'+arch+'.exe')
        raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(struct.unpack_from('<H',raw,pe+4)[0]==machine and struct.unpack_from('<H',raw,pe+24)[0]==magic,'binary architecture')
        binaries[arch]=dict(path=str(exe),sha256=sha(exe))
        for repeat in range(2):
            output=run/(arch+'-'+str(repeat)+'.bin')
            execute([exe,api if arch=='x86' else run/'nonexistent-original.dll',query,output],run/(arch+'-'+str(repeat)+'.log'))
            actual=output.read_bytes()
            need(actual==want,f'{arch} repeat{repeat} full decision/ordering/upload trace mismatch')
    need(all(sha(p)==h for p,h in sources.items()),'source closure unchanged')
    need(all(sha(p)==h for p,h in protected.items()),'protected originals/runtime/evidence preserved')
    warnings={arch:re.findall(r'warning (C\d+): ([^\r\n]+)',(run/(arch+'-build.log')).read_text(errors='replace')) for arch in binaries}
    need(not any(warnings.values()),'warning-free final probe builds')
    result=dict(status='VERIFIED',time_utc=datetime.now(timezone.utc).isoformat(),cases=len(rows),trace_bytes=len(want),
        trace_sha256=hashlib.sha256(want).hexdigest(),executables=binaries,warnings=warnings,
        original_receiving_function='shaderapidx910021430, complete receiver admitted only flat opcodes7/6/0',
        observed_uploads='10010E90 pixel and10010D70 vertex are boundary observers; their bodies and GPU are not executed',
        cube_binding='instance7(4) -> pixel c4..9; instance6 -> vertex c21..26; shared expanded cube flag bit0',
        shader=dict(source=str(cache_path),cache_sha256=sha(cache_path),static=0,dynamic=0,bytecode_sha256=hashlib.sha256(shader).hexdigest(),
                    scope='explicit controlled PS20b shader, active benchmark permutation UNVERIFIED'),
        authenticated_ranges=ranges,source_sha256=sources,protected_sha256=protected,
        limitations=['Native Phong producer callsites authenticated statically; whole100380F0 draw not executed.',
                     'Receiver preamble service, flags, payloads and instance records are explicit private fixtures.',
                     'Only ambient expansion executes through upload boundaries; device caching/queue/upload bodies and pixels remain UNVERIFIED.',
                     'No editor integration/rebuild, native SFM process, computer control, original writes or publication.'])
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    (OUT/'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:result[k] for k in ['status','cases','trace_bytes','trace_sha256','executables','warnings']},indent=2),flush=True)


if __name__=='__main__':main()
