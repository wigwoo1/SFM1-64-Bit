"""Offline benchmark composition; never opens, attaches to or controls SFM."""
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
from verify_benchmark_draw_binding import encode, decode_output, metadata
from verify_ifm_draw_frames import point, f32
from verify_ifm_draw_packet import check_ranges
from verify_session_illumination_dag import IFM, IFM_SHA, SESSION, SESSION_SHA
from verify_studio_vcollide_ready import DATACACHE, DATACACHE_SHA
from verify_dag_world_position import sha as path_sha, require

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/benchmark-draw-bounds-20261025'
GAME = IFM.parent.parent.parent


def sha(path):
    return path_sha(Path(path))


def fixtures(raw):
    at = 0
    rows = []
    while at < len(raw):
        h = struct.unpack_from('<11I', raw, at)
        v = struct.unpack_from('<49f', raw, at + 44)
        cursor = at + 240 + h[6] * 44
        sequences = raw[cursor:cursor + h[7] * 24]
        cursor += h[7] * 24
        palette = raw[cursor:cursor + h[8] * 48]
        cursor += h[8] * 48 + h[9] * 28
        require(cursor <= len(raw), 'Truncated bounds fixture')
        rows.append(dict(header=h, values=v, sequences=sequences, palette=palette))
        at = cursor
    require(at == len(raw), 'Bounds fixture framing')
    return rows


def main():
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    originals = {str(p): sha(p) for p in [IFM, DATACACHE, SESSION, GAME/'usermod/gameinfo.txt']}
    require(originals[str(IFM)] == IFM_SHA and originals[str(DATACACHE)] == DATACACHE_SHA
            and originals[str(SESSION)] == SESSION_SHA
            and originals[str(GAME/'usermod/gameinfo.txt')] == 'ef7f43f76306e6cf2852257d3f9ef068a4fa498343a935c54d91d0a9912247de', 'Original pins')
    tracked = ['ifm_draw_bounds_geometry_candidate.h', 'ifm_virtual_studio_bounds_candidate.h',
               'export_benchmark_draw_bounds.cpp', 'probe_ifm_bounds_composite.cpp',
               'test_ifm_draw_bounds_geometry.cpp', 'build_benchmark_draw_bounds.cmd',
               'verify_benchmark_draw_bounds.py', 'ifm_draw_frames_candidate.h',
               'ifm_bounds_geometry_services_candidate.h', 'ifm_renderable_bounds_candidate.h',
               'ifm_bone_world_palette_candidate.h', 'probe_ifm_bone_world_palette.cpp',
               'session_ifm_bone_inputs_candidate.h', 'source_mdl_virtual_sequences_candidate.h',
               'source_sequence_name_cache_candidate.h', 'probe_vulkan_win32_surface_x64.cpp']
    sources = {p: sha(ROOT/'work'/p) for p in tracked}
    editor = ROOT/'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe'
    editor_sha = sha(editor)
    require(editor_sha == '2e79aed74e3831196a609346251680132f7beeadcc0cb810318f1fb9f5156ed2', 'Previous editor pin')
    _, evidence = check_ranges(IFM, 'ifm-disassembly.txt', [
        ('complete_frame_update', 0x102402c0, 0x1024051d),
        ('complete_decomposition', 0x1023ba60, 0x1023bbe5),
        ('raw_renderable_world_getter', 0x1023bc80, 0x1023bcc9),
        ('complete_inner_bounds', 0x1023f310, 0x1023f698),
        ('complete_animated_bounds', 0x1023ef30, 0x1023f310),
        ('normal_bone_root_caller', 0x10241bf9, 0x10241d74),
        ('normal_palette_mask_and_call', 0x10241d74, 0x10241dcb),
        ('complete_normal_bone_kernel', 0x102411b0, 0x1024181f),
    ])
    builds = [('draw-native', 'draw_bounds_native_x86', 0x14c),
              ('bounds-native', 'bounds_native_x86', 0x14c),
              ('palette-native', 'palette_native_x86', 0x14c),
              ('draw-owned', 'draw_bounds_owned_x64', 0x8664),
              ('bounds-owned', 'bounds_owned_x64', 0x8664),
              ('palette-owned', 'palette_owned_x64', 0x8664),
              ('export', 'benchmark_draw_bounds_x64', 0x8664),
              ('safety', 'draw_bounds_safety_x64', 0x8664)]
    warnings = {}
    executables = []
    for tag, name, machine in builds:
        log = (OUT/(tag+'-build.log')).read_text()
        warnings[tag] = [s for s in log.splitlines() if 'warning ' in s.lower()]
        require('error C' not in log and all(tag in ['draw-owned', 'bounds-owned', 'export']
                and 'C4458' in s and 'graph_transform_hierarchy.h' in s for s in warnings[tag]), 'Build diagnostics '+tag)
        exe = OUT/(name+'.exe')
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        require(raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', raw, pe+4)[0] == machine
                and struct.unpack_from('<H', raw, pe+24)[0] == (0x10b if machine == 0x14c else 0x20b), 'Architecture '+name)
        executables.append(dict(path=str(exe), sha256=sha(exe), machine=hex(machine), freshly_built=True))

    def execute(exe, args, label):
        p = subprocess.run([str(exe), *map(str, args)], cwd=ROOT, capture_output=True, text=True, timeout=120)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode == 0, label+' exit='+str(p.returncode)+' '+p.stdout+p.stderr)
        return p.stdout

    def local(name, args, label=None):
        return execute(OUT/(name+'.exe'), args, label or name)

    safety = local('draw_bounds_safety_x64', [])
    require('checks=6 refusals=4' in safety, 'Safety coverage')
    source = run/'benchmark-copy.dmx'
    shutil.copyfile(SESSION, source)
    query = run/'bounds.bin'
    palettes = run/'palettes.bin'
    audit = local('benchmark_draw_bounds_x64', [source, GAME/'usermod/gameinfo.txt', query, palettes])
    require('PASS cases=32 models=4' in audit and 'normal_no_precomputed' in audit, 'Export coverage')
    rows = fixtures(query.read_bytes())
    require(len(rows) == 32, '32 benchmark configurations')
    assets = sorted(set(re.findall(r'^asset_path=(.+)$', audit, re.M)))
    pins = {'450f8b480e0b85ed79b8747dae79c6d537cde1342bff4e77e5216d3008eabdd1',
            'cf688be14884e63535e313e9a894e75a928f037f604f5193c426dcec9ae1eae6',
            '3f5bb351433ecc1339476f28d840d8b0000281f8792be80179ac122f5ef77963'}
    require(len(assets) == 3 and {sha(p) for p in assets} == pins, 'Mounted model pins')
    models = [dict(path=p, sha256=sha(p)) for p in assets]
    pyro = next(p for p in assets if p.replace('\\', '/').endswith('/hwm/pyro.mdl'))
    included = next(p for p in assets if p.replace('\\', '/').endswith('/pyro_animations.mdl'))
    flame = next(p for p in assets if p.replace('\\', '/').endswith('/w_flamethrower.mdl'))
    meta = {p: metadata(Path(p)) for p in assets}
    group_paths = [[pyro, included], [flame]] * 2
    groups = [[meta[p][0] for p in paths] for paths in group_paths]
    seq_query = run/'sequence-queries.bin'
    seq_query.write_bytes(b''.join(map(encode, groups)))
    # Unchanged reference dependencies are reused ONLY after checking their
    # binary and source hashes against the immediately preceding receipt.
    prior = json.loads((ROOT/'outputs/virtual-sequence-cache-20261025/verification-__xnibe0/results.json').read_text())
    for p in ['probe_virtual_sequence_append.cpp', 'source_mdl_virtual_sequences_candidate.h', 'source_sequence_name_cache_candidate.h']:
        require(sha(ROOT/'work'/p) == prior['sources_sha256'][p], 'Reference dependency changed '+p)
    tables = []
    for name in ['sequence_uncached_native_x86', 'sequence_cached_native_x86']:
        ref = next(e for e in prior['executables'] if Path(e['path']).stem == name)
        require(sha(ref['path']) == ref['sha256'], 'Reference binary changed')
        executables.append(dict(**ref, freshly_built=False, provenance='previous verified name-cache receipt'))
        target = run/(name+'.bin')
        execute(ref['path'], [DATACACHE, seq_query, target], name)
        tables.append(decode_output(target.read_bytes(), groups))
    require(tables[0] == tables[1], 'Actual benchmark cache modes differ')
    selections = []
    for i, paths in enumerate(group_paths):
        for cache in range(2):
            table = tables[cache][i]
            expected = b''.join(struct.pack('<6f', *meta[paths[e[2]]][1][e[3]]) for e in table['entries'])
            raw_root = Path(paths[0]).read_bytes()
            for row in rows[i*8+cache*4:i*8+cache*4+4]:
                require(row['sequences'] == expected, 'Flattened bounds differ from actual original virtual table')
                require(struct.pack('<12f', *row['values'][6:18]) == raw_root[128:152]+raw_root[104:128], 'Root hull/view changed')
        seq = 57 if i % 2 == 0 else 0
        e = tables[0][i]['entries'][seq]
        selections.append(dict(model=13207 if i % 2 == 0 else 13817, tick=50000 if i < 2 else 50417,
                               sequence=seq, group=e[2], local=e[3], label=meta[paths[e[2]]][0][e[3]][0]))
    palette_outputs = []
    for name in ['palette_native_x86', 'palette_owned_x64']:
        target = run/(name+'.bin')
        log = local(name, [IFM, palettes, target])
        require('bone_world_cases=4 procedural=disabled precomputed_bones=0' in log, 'Normal palette coverage')
        palette_outputs.append(target.read_bytes())
    require(palette_outputs[0] == palette_outputs[1], 'Normal kernel mismatch')
    offset = 0
    for i in range(4):
        count = rows[i*8]['header'][8]
        expected = palette_outputs[0][offset:offset+count*48]
        require(all(r['palette'] == expected for r in rows[i*8:i*8+8]), 'Bounds palette not original normal-kernel result')
        offset += count*48
    require(offset == len(palette_outputs[0]), 'Normal palette output framing')
    outputs = []
    for name in ['draw_bounds_native_x86', 'draw_bounds_owned_x64']:
        target = run/(name+'.bin')
        local(name, [IFM, query, target])
        outputs.append(target.read_bytes())
    require(outputs[0] == outputs[1] and len(outputs[0]) == 32*160, 'Full frame/bounds composition mismatch')
    centers = []
    for i, row in enumerate(rows):
        raw = outputs[0][i*160:(i+1)*160]
        frames = struct.unpack_from('<12f', raw)
        queried = struct.unpack_from('<I', raw, 48)[0]
        intermediate = struct.unpack_from('<12f', raw, 52)
        bounds = struct.unpack_from('<6f', raw, 100)
        events = struct.unpack_from('<9I', raw, 124)
        h, v = row['header'], row['values']
        mode, flags, tick = h[3], h[4], h[10]
        root_position = v[33:42:4]
        require(queried == mode and frames[6:9] == root_position, 'Root mode/position gate')
        if not mode:
            require(frames[0:3] == root_position and raw[52:] == bytes(108), 'Mode0 bypass touched bounds')
        else:
            require(events[:3] == (1, 0, 0) and events[8] == 0, 'Unexpected bounds services')
            animated = bool(flags & 0x20)
            require(events[3] == int(animated) and events[7] == int(animated), 'Animated gate')
            if animated:
                expected_time = struct.unpack('<I', struct.pack('<f', f32(tick * f32(0.0001))))[0]
                require(events[4:7] == (256, 0x100, expected_time), 'SetupBones arguments')
            require(intermediate[0:3] == root_position and intermediate[3:] == frames[3:], 'Intermediate/final frame ordering')
            center = [f32(f32(bounds[j]+bounds[j+3])*0.5) for j in range(3)]
            require(tuple(point(center, v[30:42])) == frames[0:3], 'Independent midpoint/world transform')
            if (i % 8) < 4:
                centers.append(dict(**selections[i//8], render_flags=flags, position=list(frames[:3]),
                                    inner_bounds=list(bounds), palette_contract='normal, procedural disabled, no precomputed DAG bones'))
        # Both native modes must produce identical values for these actual assets.
        if i % 8 < 4:
            require(raw == outputs[0][(i+4)*160:(i+5)*160], 'Benchmark cache-mode result differs')
    old = ROOT/'outputs/ifm-bounds-composite-20261015/verification-r9b3o81r'
    old_receipt = json.loads((old/'results.json').read_text())
    require(sha(old/'queries.bin') == old_receipt['query_sha256'], 'Retained bounds corpus provenance')
    regressions = []
    for name in ['bounds_native_x86', 'bounds_owned_x64']:
        target = run/(name+'.bin')
        local(name, [IFM, old/'queries.bin', target])
        regressions.append(target.read_bytes())
    require(regressions[0] == regressions[1] and len(regressions[0]) == 28932*240, 'Existing bounds regression')
    require(all(sha(p) == h for p, h in originals.items()) and sha(source) == SESSION_SHA
            and all(sha(m['path']) == m['sha256'] for m in models), 'Original/disposable input changed')
    require(all(sha(ROOT/'work'/p) == h for p, h in sources.items())
            and all(sha(e['path']) == e['sha256'] for e in executables) and sha(editor) == editor_sha, 'Sources/executables changed')
    result = dict(status='PASS', checked_at_utc=datetime.now(timezone.utc).isoformat(),
        benchmark_configurations=32, exact_composite_bytes=len(outputs[0]), normal_palette_cases=4,
        exact_palette_bytes=len(palette_outputs[0]), retained_bounds_cases=28932, retained_bounds_comparisons=115728,
        exact_regression_bytes=len(regressions[0]), safety=safety.strip(), candidate_centers=centers,
        original_sequence_selections=selections, models=models, warnings=warnings, executables=executables,
        original_sha256=originals, sources_sha256=sources, query_sha256=sha(query), palette_query_sha256=sha(palettes),
        static_instruction_bytes=sum(e['bytes'] for e in evidence),
        static_ranges=[{k:v for k,v in e.items() if k != 'instructions'} for e in evidence],
        prior_editor=dict(path=str(editor), sha256=editor_sha, rebuilt=False, run=False),
        limitations=[
            'Stable two-frame benchmark snapshot; actual runtime mode and renderable flag0x20 remain UNVERIFIED. Both configurations are tested, not assumed active.',
            'Complete original frame update, raw-world getter and inner/animated bounds run privately; one checked CALL bridges to the unmodified inner bounds for intermediate observations. Original cache/type/header/clock/SetupBones/spatial services remain controlled.',
            'SetupBones supplies separately compared NORMAL kernel palettes. Procedural processing is disabled and precomputed DAG bones excluded. Full native SetupBones, bone cache, sequence pose blending, referencing DAG overrides and mutable lifecycle are NOT verified.',
            'Fresh native cached/uncached tables select raw model descriptor bounds; the bounds fixture supplies flattened descriptors. Native virtual group/header construction is not executed inside the bounds image.',
            'No owned editor dependency changed. No complete editor rebuild/run, GPU image, lighting improvement or original SFM pixel parity claimed.',
            'No computer use, live-process probes, SFM launch/control/close, original file writes, Blender, downloads or publication.',
        ])
    (run/'results.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k:result[k] for k in ['status','benchmark_configurations','exact_composite_bytes','normal_palette_cases','exact_palette_bytes','retained_bounds_cases','retained_bounds_comparisons']}, indent=2))
    print('receipt='+str(run/'results.json'))


if __name__ == '__main__':
    main()
