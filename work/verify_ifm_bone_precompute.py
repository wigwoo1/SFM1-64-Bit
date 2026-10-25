"""Private original typed-model prepass/kernel; no SFM or live-process access."""
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
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM, IFM_SHA, SESSION, SESSION_SHA

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT/'outputs/precomputed-bone-palette-20261025'
IDENTITY = [1,0,0,0,0,1,0,0,0,0,1,0]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise RuntimeError(message)


def pose(parent=-1, flags=0x100, position=(1,2,3), q=(0,0,0,1), scale=1):
    return struct.pack('<iI8f', parent, flags, *position, *q, scale)


def matrix(x):
    m = IDENTITY.copy()
    m[3] = x
    return m


def encode(poses, refs, mask=0x100, flags=3, root=IDENTITY):
    require(len(poses) == len(refs), 'Fixture count')
    return struct.pack('<3I12f', len(poses), mask, flags, *root)+b''.join(poses)+b''.join(
        struct.pack('<2I', typed, len(entries))+b''.join(struct.pack('<I12f', f, *m) for f,m in entries)
        for typed,entries in refs)


def counts(raw):
    at = 0
    result = []
    while at < len(raw):
        count, mask, flags = struct.unpack_from('<3I', raw, at)
        at += 60+count*40
        for _ in range(count):
            typed, n = struct.unpack_from('<2I', raw, at)
            at += 8+n*52
        result.append((count, mask, flags))
    require(at == len(raw), 'Precompute input framing')
    return result


def decode(raw, shapes):
    at = 0
    result = []
    for count,mask,flags in shapes:
        selection = struct.unpack_from('<'+str(count)+'I', raw, at)
        at += count*4
        bits = struct.unpack_from('<8I', raw, at)
        calls = struct.unpack_from('<I', raw, at+32)[0]
        at += 36
        palette = raw[at:at+count*48]
        require(len(palette) == count*48, 'Precompute output truncated')
        at += count*48
        result.append(dict(selection=selection, bits=bits, calls=calls, palette=palette))
    require(at == len(raw), 'Precompute output framing')
    return result


def main():
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    originals = {str(p):sha(p) for p in [IFM, SESSION]}
    require(originals[str(IFM)] == IFM_SHA and originals[str(SESSION)] == SESSION_SHA, 'Original pins')
    sources = {p:sha(ROOT/'work'/p) for p in [
        'ifm_bone_world_palette_candidate.h','ifm_bone_precompute_candidate.h',
        'session_ifm_bone_references_candidate.h','probe_ifm_bone_precompute.cpp',
        'audit_benchmark_bone_references.cpp','test_ifm_bone_precompute.cpp',
        'build_ifm_bone_precompute.cmd','verify_ifm_bone_precompute.py',
        'probe_ifm_bone_world_palette.cpp','probe_vulkan_win32_surface_x64.cpp']}
    editor = ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe'
    editor_sha = sha(editor)
    require(editor_sha == '2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2', 'Prior editor')
    image,evidence = check_ranges(IFM,'ifm-disassembly.txt',[
        ('complete_palette_kernel',0x102411b0,0x1024181f),
        ('transform_to_referencing_DAG',0x104d2940,0x104d2987),
        ('reverse_reference_iteration',0x10223170,0x102231f9),
        ('reference_filter_and_jump_table',0x10222de0,0x10222e88),
        ('override_disable_and_parent_gate',0x104c3e00,0x104c3e23),
        ('typed_override_parent_getter',0x101e3170,0x101e31f5),
        ('typed_bone_transform_getter',0x105c3580,0x105c35ca),
        ('bone_array_count_getter',0x100d7c80,0x100d7c87),
        ('element_handle_accessor',0x10335830,0x10335834),
        ('typed_reference_storage_accessor',0x10002780,0x100027e1),
    ])
    require(image.string(0x10acfaa8) == 'transform' and image.string(0x10b8e468) == 'overrideParent', 'Original attribute names')
    require(image.word(0x10222e78) == 0x10222e33, 'kind0 does not filter reference flag bits')
    executables = []
    warnings = {}
    for tag,name,machine in [('native','precompute_native_x86',0x14c),('owned','precompute_owned_x64',0x8664),
            ('normal-native','normal_native_x86',0x14c),('normal-owned','normal_owned_x64',0x8664),
            ('audit','benchmark_bone_references_x64',0x8664),('safety','precompute_safety_x64',0x8664)]:
        log = (OUT/(tag+'-build.log')).read_text()
        warnings[tag] = [s for s in log.splitlines() if 'warning ' in s.lower()]
        require('error C' not in log and all(tag == 'audit' and 'C4458' in s and 'graph_transform_hierarchy.h' in s for s in warnings[tag]), 'Build diagnostics '+tag)
        path = OUT/(name+'.exe')
        b = path.read_bytes()
        pe = struct.unpack_from('<I', b, 60)[0]
        require(b[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H',b,pe+4)[0] == machine
                and struct.unpack_from('<H',b,pe+24)[0] == (0x10b if machine == 0x14c else 0x20b), 'PE architecture')
        executables.append(dict(path=str(path),sha256=sha(path),machine=hex(machine)))

    def execute(name,args,label=None):
        p = subprocess.run([str(OUT/(name+'.exe')),*map(str,args)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (run/((label or name)+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode == 0, name+' exit='+str(p.returncode)+' '+p.stdout+p.stderr)
        return p.stdout

    safety = execute('precompute_safety_x64',[])
    require('checks=6 refusals=7' in safety, 'Safety coverage')
    cases = []
    cases.append(encode([pose()]*4,[(3,[(255,matrix(10)),(223,matrix(20))]),
        (3,[(207,matrix(30)),(223,matrix(40))]),(3,[(215,matrix(50)),(223,matrix(60))]),
        (3,[(223,matrix(70)),(223,matrix(80))])]))
    # A precomputed out-of-mask parent supplies a requested child's basis;
    # ignored nonfinite local pose must not be evaluated for that parent.
    cases.append(encode([pose(flags=0,position=(float('nan'),0,0)),pose(parent=0)],
                        [(3,[(223,matrix(100))]),(3,[])]))
    cases.append(encode([],[]))
    for ref,typed,model,mask in itertools.product(range(256),range(4),range(8),[0,0x100,0xff,0x7ff00]):
        cases.append(encode([pose()],[(typed,[(ref,matrix(13))])],mask,model))
    rng = random.Random(2026102504)
    for _ in range(512):
        n = rng.randrange(1,97)
        poses = [pose(parent=-1 if i == 0 or rng.random()<.15 else rng.randrange(i),
                      position=[rng.uniform(-100,100) for _ in range(3)],
                      q=[rng.uniform(-1,1) for _ in range(4)],scale=rng.choice([-2,-1,.5,1,2])) for i in range(n)]
        refs = []
        for i in range(n):
            entries = []
            for j in range(rng.randrange(6)):
                world = [rng.uniform(-10,10) for _ in range(12)]
                entries.append((rng.choice([223,255,207,rng.randrange(256)]),world))
            refs.append((rng.randrange(4),entries))
        cases.append(encode(poses,refs,mask=rng.choice([0,0x100,0x7ff00]),flags=rng.randrange(8)))
    cases.append(encode([pose() for _ in range(256)],[(3,[(223,matrix(i))]) for i in range(256)],mask=0))
    corpus = run/'queries.bin'
    corpus.write_bytes(b''.join(cases))
    shapes = counts(corpus.read_bytes())
    raw = []
    for name in ['precompute_native_x86','precompute_owned_x64']:
        target = run/(name+'.bin')
        execute(name,[IFM,corpus,target])
        raw.append(target.read_bytes())
    require(raw[0] == raw[1], 'Precompute/kernel mismatch; preserve first mismatch')
    results = decode(raw[0],shapes)
    require(results[0]['selection'] == (0,1,1,0) and results[0]['bits'][0] == 14 and results[0]['calls'] == 3, 'Independent first-match/stop/file/name expectations')
    require(results[1]['bits'][0] == 1 and results[1]['calls'] == 1 and struct.unpack_from('<f',results[1]['palette'],48+12)[0] == 101, 'Independent out-of-mask parent/NaN-bypass expectation')
    require(results[-1]['bits'] == (0xffffffff,)*8 and results[-1]['calls'] == 256, '256-bone/bitset-boundary prepass')
    prior_dir = ROOT/'outputs/benchmark-draw-bounds-20261025/verification-czdinbt0'
    prior = json.loads((prior_dir/'results.json').read_text())
    source = prior_dir/'benchmark-copy.dmx'
    normal_query = prior_dir/'palettes.bin'
    require(sha(source) == SESSION_SHA and sha(normal_query) == prior['palette_query_sha256'], 'Retained benchmark fixture provenance')
    models = prior['models']
    for m in models:
        require(sha(m['path']) == m['sha256'], 'Mounted model changed since retained benchmark inputs')
        originals[m['path']] = m['sha256']
    actor_paths = [next(m['path'] for m in models if m['path'].replace('\\','/').endswith('/hwm/pyro.mdl')),
                   next(m['path'] for m in models if m['path'].replace('\\','/').endswith('/w_flamethrower.mdl'))]*2
    normal_bytes = normal_query.read_bytes()
    offset = 0
    for path in actor_paths:
        n,mask = struct.unpack_from('<2I',normal_bytes,offset)
        offset += 56
        mdl = Path(path).read_bytes()
        count,start = struct.unpack_from('<2I',mdl,156)
        require(count == n and mask == 0x7ff00, 'Mounted bone count/mask')
        for i in range(n):
            parent = mdl[start+i*216+4:start+i*216+8]
            flags = mdl[start+i*216+160:start+i*216+164]
            require(normal_bytes[offset+i*40:offset+i*40+8] == parent+flags and not(struct.unpack('<I',flags)[0]&4), 'Retained local pose does not match current MDL parent/flags')
        offset += n*40
    require(offset == len(normal_bytes), 'Retained normal query framing')
    benchmark = run/'benchmark-queries.bin'
    audit = execute('benchmark_bone_references_x64',[source,normal_query,benchmark])
    require(audit.count('bones=68 referencing_dags=68 enabled_overrides=0 ambiguous=0') == 2
            and audit.count('precomputed=0') == 4 and 'PASS benchmark_bone_reference_frames=2' in audit, 'Benchmark gate evidence')
    benchmark_shapes = counts(benchmark.read_bytes())
    require([n for n,_,_ in benchmark_shapes] == [65,3,65,3] and all(f == 7 for _,_,f in benchmark_shapes), 'Actual benchmark procedural flag export')
    benchmark_outputs = []
    for name in ['precompute_native_x86','precompute_owned_x64']:
        target = run/('benchmark-'+name+'.bin')
        execute(name,[IFM,benchmark,target],'benchmark-'+name)
        benchmark_outputs.append(target.read_bytes())
    require(benchmark_outputs[0] == benchmark_outputs[1], 'Typed benchmark prepass differs')
    benchmark_rows = decode(benchmark_outputs[0],benchmark_shapes)
    require(all(r['selection'] == (0,)*n and r['bits'] == (0,)*8 and r['calls'] == 0 for r,(n,_,_) in zip(benchmark_rows,benchmark_shapes)), 'Benchmark unique-reference/no-write gate')
    # Fresh normal reference and modified owned normal API on the same retained
    # benchmark inputs; no older executable is used for this equality check.
    normal_outputs = []
    for name in ['normal_native_x86','normal_owned_x64']:
        target = run/('benchmark-'+name+'.bin')
        execute(name,[IFM,normal_query,target],'benchmark-'+name)
        normal_outputs.append(target.read_bytes())
    require(normal_outputs[0] == normal_outputs[1] == b''.join(r['palette'] for r in benchmark_rows), 'Typed benchmark palette differs from normal path')
    old = ROOT/'outputs/bone-world-palette-20261024/verification-0v8wip82'
    old_receipt = json.loads((old/'results.json').read_text())
    require(sha(old/'queries.bin') == old_receipt['query_sha256'], 'Retained normal corpus pin')
    normal_regression = []
    for name in ['normal_native_x86','normal_owned_x64']:
        target = run/('regression-'+name+'.bin')
        log = execute(name,[IFM,old/'queries.bin',target],'regression-'+name)
        require('bone_world_cases=2074' in log, 'Retained normal case count')
        normal_regression.append(target.read_bytes())
    require(normal_regression[0] == normal_regression[1] and len(normal_regression[0]) == 1660800, 'Normal palette regression')
    require(all(sha(p) == h for p,h in originals.items()) and sha(source) == SESSION_SHA, 'Original/fixture changed')
    require(all(sha(ROOT/'work'/p) == h for p,h in sources.items())
            and all(sha(e['path']) == e['sha256'] for e in executables) and sha(editor) == editor_sha, 'Source/binary changed')
    result = dict(status='PASS',checked_at_utc=datetime.now(timezone.utc).isoformat(),cases=len(shapes),
        exact_output_bytes=len(raw[0]),selected_reference_cases=sum(any(i != 0xffffffff for i in r['selection']) for r in results),
        precomputed_matrices=sum(r['calls'] for r in results),benchmark_cases=4,benchmark_bones=136,
        benchmark_precomputed_writes=0,exact_benchmark_bytes=len(benchmark_outputs[0]),normal_palette_bytes=len(normal_outputs[0]),
        normal_regression_cases=2074,exact_normal_regression_bytes=1660800,safety=safety.strip(),warnings=warnings,
        sources_sha256=sources,original_sha256=originals,models=models,executables=executables,
        query_sha256=sha(corpus),benchmark_query_sha256=sha(benchmark),retained_normal_query_sha256=sha(old/'queries.bin'),
        static_instruction_bytes=sum(e['bytes'] for e in evidence),static_ranges=[{k:v for k,v in e.items() if k != 'instructions'} for e in evidence],
        prior_editor=dict(path=str(editor),sha256=editor_sha,rebuilt=False,run=False),
        limitations=[
            'Original full palette kernel, typed bone getter, reverse-reference iteration/filter and override gate execute privately. Host controls DM services, type results, reference order/file identities and attribute lookup. World-matrix getter supplies fixture matrices; native DAG world math is not re-executed in this probe.',
            'Native helper searches first same-file DAG reference named transform; overridesDisabled or absent/untyped overrideParent then suppresses precomputation without trying another reference. Owned preserves these gates. Native prepass writes out-of-mask slots and computed bits; normal descendants can consume them.',
            'Retained graph adapter covers a single loaded serialized graph with exact matching bone array length. Multiple matching DAG owners and external references are rejected because native reverse order/file contexts are absent from DMX.',
            'Current owned benchmark evaluation at two frames yields one DAG per bone and no enabled override. Native typed prepass confirms zero writes under that snapshot; normal palettes match with actual saved evaluateProceduralBones=true and MDL procedural-flag4 absent. This is not a measurement of mutable live SFM state.',
            'No general procedural-bone implementation, full SetupBones cache/pose blending or runtime flags verified. No editor dependency changed/rebuild/run; no viewport, lighting or pixel parity claimed.',
            'No computer use, SFM process access/launch/control/close, original writes, Blender, GPU job, dependency install or publication.',
        ])
    (run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','cases','exact_output_bytes','precomputed_matrices','benchmark_bones','benchmark_precomputed_writes','normal_regression_cases']},indent=2))
    print('receipt='+str(run/'results.json'))


if __name__ == '__main__':
    main()
