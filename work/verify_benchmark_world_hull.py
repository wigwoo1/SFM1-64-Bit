"""Compare actual benchmark-backed prepared hull world queries in LDR/HDR modes."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from verify_sliding_door_spawn import ENGINE, BSP, sha, verify_installed_ranges

ENGINE_SHA256 = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
BSP_SHA256 = '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
PERMUTATION_SHA256 = '35e914e111dddaa8e76930d29c4c2f46156130b7f2d00ff7734274cb947adbe6'


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'benchmark-world-hull-reference.local.txt', [
        ('complete swept hull BSP traversal', 0x10148c30, 0x10148f4a),
        ('complete optimized hull box kernel', 0x10146850, 0x10146c24),
        ('complete hull brush dispatch/general kernel', 0x10147920, 0x10147c27),
        ('complete runtime surface lookup', 0x10146440, 0x1014646a),
        ('complete original plane copy', 0x100e9720, 0x100e975d),
        ('complete non-ray leaf routine', 0x10148540, 0x10148867),
        ('complete displacement finish', 0x1014cfa0, 0x1014cfe9),
        ('complete outer world hull adapter', 0x1014d100, 0x1014d1ec),
        ('complete mesh cache build', 0x1015c630, 0x1015c6a1),
        ('complete triangle cache geometry', 0x1015c3b0, 0x1015c563),
        ('complete plane intern', 0x1015b9f0, 0x1015badb),
        ('complete hull tree and contact dispatch', 0x1015c840, 0x1015cc1f),
        ('complete direction normalization', 0x10244060, 0x102440cd),
        ('complete hull triangle contact', 0x1015abb0, 0x1015afa3),
        ('complete shared plane clip', 0x10158cb0, 0x10158d70),
        ('complete face plane clip', 0x10158d70, 0x10158e77),
        ('complete projected edge clip YZ', 0x10159520, 0x10159641),
        ('complete projected edge clip XZ', 0x10159650, 0x10159770),
        ('complete projected edge clip XY', 0x10159770, 0x1015988d),
    ])
    offset = image.offset(0x104129b0)
    data = image.b[offset:offset+1024]
    assert hashlib.sha256(data).hexdigest() == PERMUTATION_SHA256
    fixture = run/'permutation-reference.local.bin'
    fixture.write_bytes(data)
    baseline = counts = None
    native_evidence = []
    bsp = BSP.read_bytes()
    texinfo, texinfo_bytes = struct.unpack_from('<2I', bsp, 8+6*16)
    texdata_bytes, = struct.unpack_from('<I', bsp, 8+2*16+4)
    material_count = texdata_bytes//32
    mapping = [struct.unpack_from('<I', bsp, texinfo+72*i+68)[0] for i in range(texinfo_bytes//72)]
    material_flags = [0]*material_count
    for i, material in enumerate(mapping):
        assert material < material_count
        material_flags[material] |= struct.unpack_from('<I', bsp, texinfo+72*i+64)[0]&65535
    assert len(mapping) == 2478 and material_count == 87
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'benchmark_world_hull_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            records = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(fixture), str(records)], capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 16512 and current['face_modes'] == 2 and current['meshes_per_mode'] == 43
            if arch == 'x86':
                evidence_counts = {key: current.pop(key) for key in ['native_displacement_then_brush', 'native_brush_then_displacement']}
                assert evidence_counts['native_displacement_then_brush'] > 0 and evidence_counts['native_brush_then_displacement'] >= 0
                native_evidence.append(evidence_counts)
                assert evidence_counts == native_evidence[0]
            for key in ['changed_fraction', 'brush_winners', 'displacement_winners', 'start_solid', 'marker_cleared', 'rounded_delta_cases']:
                assert current[key] > 0, key
            if counts is None:
                counts = current
            assert counts == current
            assert current['materials'] == 87 and current['texinfo'] == 2478 and current['triangles_total'] == 10624
            assert current['planes_total'] > 0 and current['leaf_refs_total'] > 0
            payload = records.read_bytes()
            block_bytes = 16+8256*64
            assert len(payload) == 2*block_bytes
            bodies = []
            changed = brush = displacement = solid = cleared = 0
            bits = lambda v: struct.unpack('<I', struct.pack('<f', v))[0]
            for hdr in range(2):
                begin = hdr*block_bytes
                assert struct.unpack_from('<4I', payload, begin) == (hdr, 58 if hdr else 7, 43, 5312)
                body = payload[begin+16:begin+block_bytes]
                bodies.append(body)
                for case in range(8256):
                    row = struct.unpack_from('<16I', body, case*64)
                    mode = [0, 1, 3, 4][case%4]
                    mask = [0, 1, 0x2081, 0xffffffff][(case//4)%4]
                    prior_fraction = [1., .25, 0.][mode%3]
                    prior = [bits(prior_fraction), bits(.75 if mode&2 else 0), 0, 0, 0x12345678, bits(9), bits(8), bits(7), bits(6), 0x02030405, 1 if mode < 3 else 2, mapping[77] if mode < 3 else 3, 0xbeef, 0 if mode < 3 else 0xffffffff, 0xabcd, 0xef01]
                    fraction, left = struct.unpack('<2f', struct.pack('<2I', *row[:2]))
                    assert 0 <= fraction <= 1 and 0 <= left <= 1
                    assert row[2] in (0, 1) and row[3] in (0, 1) and row[10] in (0, 1, 2)
                    assert row[12] < 65536 and row[14] < 65536 and row[15] < 65536
                    if row[10] == 0:
                        assert row[11] == 0 and row[14:16] == (0x3344, 0)
                    if row[10] == 1:
                        assert row[11] < material_count
                        if row[14] == 0xabcd:
                            assert mode < 3 and row[11] == mapping[77] and row[15] == 0xef01
                        else:
                            assert row[14] == 1000+row[11] and row[15] == material_flags[row[11]]
                    if row[10] == 2:
                        assert row[11] < 43
                        assert row[14] in (0xabcd, 100+row[11], 500+row[11])
                    if mask == 0:
                        assert all(row[i] == prior[i] for i in range(16) if i not in (2, 3))
                    is_changed = row[0] != prior[0]
                    changed += is_changed
                    brush += row[10] == 1 and is_changed
                    displacement += row[10] == 2 and is_changed
                    solid += row[2] != 0
                    cleared += prior[13] != 0 and row[13] == 0
            assert bodies[0] == bodies[1], 'LDR/HDR benchmark trace geometry differs'
            assert (changed, brush, displacement, solid, cleared) == tuple(counts[k] for k in ['changed_fraction', 'brush_winners', 'displacement_winners', 'start_solid', 'marker_cleared'])
            assert counts['rounded_delta_cases'] >= 1376
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'benchmark world semantic trace mismatch')
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        source = root/name
        sources[name] = sha(source)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', source.read_text(), re.M):
            visit(include)

    for name in ['probe_benchmark_world_hull.cpp', 'build_benchmark_world_hull.cmd', 'verify_benchmark_world_hull.py']:
        visit(name)
    receipt = dict(status='VERIFIED BENCHMARK-BACKED PREPARED SWEPT HULL WORLD GEOMETRY', counts=counts,
                   repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256, originals_unchanged=True,
                   native_winner_transfer_evidence=native_evidence,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence],
                   replaced_callees=['bucket insertion1015b150', 'plane append10001610', 'prepared cache acquire1015c760', 'cache release10261300'],
                   findings=[
                       'Actual benchmark BSP brush/node/leaf tables are retained. SourceBspDisplacementCollisionCandidate imports43meshes/5312triangles per face mode; no synthetic geometry override is used.',
                       'Both LDR7 and HDR58 face-table modes produce identical final trace records, matching prior imported mesh observations.',
                       'New derived-table helper generates owned caches, culls and world leaf assignments. Native fixture supports all43runtime objects, generates each original cache and byte-compares its plane/index data.',
                       'Full original BSP/leaf/optimized-general brush/cull/outer displacement/tree/contact/finish routines execute together. Only prepared cache storage/acquire/release services are adapted.',
                       'Actual2478texinfo-to87material mapping and OR-merged BSP material flags are used on both paths; native general sides are translated to runtime material IDs, while owned semantic surfaces resolve through the checked catalog.',
                       'Material property numbers are explicitly supplied distinguishing fixtures, not asserted to be original effective IMaterial results.',
                       'Queries sample each real displacement bounds/triangle centroid, both sides, axial/oblique/tiny/zero movement, varied hull sizes, masks and prior metadata.',
                       'Final trace planes/tags/fraction/solids/contents/material identity/flags and supplied property selection match across four repeated architecture runs. Native selected displacement ordinals are observed at real outer entry/return.',
                       'Native cached geometry/executable sections and installed engine/BSP bytes remain unchanged; owned x64 execution opens no original engine DLL.'],
                   corrected_issue='Initial fixture surface name used raw texture-info77 instead of its runtime material18. Corrected only native initial name binding; no production collision algorithm correction was needed.',
                   limits=[
                       'Original engine whole BSP loader orchestration is not executed. Native query routines receive owned-imported prepared mesh/tree/leaf records, whose component compatibility has separate earlier evidence.',
                       'Effective material defaults/patches/mount precedence and original physics property IDs remain unverified; this task uses explicit property fixtures with actual BSP identities and flags.',
                       'Final public geometry semantics are compared, not every internal work byte/visit sequence.',
                       'Native global cache allocation/growth/lifetime/synchronization and production permutation distribution remain separate.',
                       'Public Ray_t trace setup/finish, stationary dispatcher, entities/static props/player filtering, measured benchmark spawn and editor integration remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
