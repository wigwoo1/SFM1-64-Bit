"""Compare mixed prepared BSP brush/displacement swept hull world queries."""
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
    image, evidence = verify_installed_ranges(ENGINE, run/'mixed-world-hull-reference.local.txt', [
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
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'prepared_world_hull_{arch}.exe'
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
            assert current['passed'] and current['cases'] == 64512 and current['grids'] == 16
            if arch == 'x86':
                evidence_counts = {key: current.pop(key) for key in ['native_displacement_then_brush', 'native_brush_then_displacement']}
                assert all(value > 0 for value in evidence_counts.values())
                native_evidence.append(evidence_counts)
                assert evidence_counts == native_evidence[0]
            for key in ['changed_fraction', 'brush_winners', 'displacement_winners', 'start_solid', 'marker_cleared', 'rounded_delta_cases']:
                assert current[key] > 0, key
            if counts is None:
                counts = current
            assert counts == current
            payload = records.read_bytes()
            assert len(payload) == current['cases']*64
            changed = brush = displacement = solid = cleared = 0
            bits = lambda v: struct.unpack('<I', struct.pack('<f', v))[0]
            for case in range(current['cases']):
                row = struct.unpack_from('<16I', payload, case*64)
                mode, q = case%8, (case//56)%24
                mask = [0, 1, 0x80, 0x2080, 0x2081, 0x4000, 0xffffffff][(case//8)%7]
                prior_fraction = 1. if mode >= 6 else [1., .25, 0.][mode%3]
                prior = [bits(prior_fraction), bits(.75 if mode&2 else 0), int(mode == 6), int(mode == 7), 0x12345678, bits(9), bits(8), bits(7), bits(6), 0x02030405, 1 if mode < 3 else 2, 77 if mode < 3 else 3, 0xbeef, 0 if mode < 3 else 0xffffffff, 0xabcd, 0xef01]
                fraction, left = struct.unpack('<2f', struct.pack('<2I', *row[:2]))
                assert 0 <= fraction <= 1 and 0 <= left <= 1
                assert row[2] in (0, 1) and row[3] in (0, 1)
                assert row[10] in (0, 1, 2) and row[12] < 65536 and row[14] < 65536 and row[15] < 65536
                if row[10] == 0:
                    assert row[11] == 0 and row[14] == 0x3344
                    assert row[15] == (0x80 if q%4&2 else 0)
                if row[10] == 1:
                    assert row[11] < 32768
                    if row[14] != 0xabcd:
                        assert row[14] == 1000+row[11]
                        assert row[15] == (0x80 if q%4&1 and row[11]&1 else 0)
                if row[10] == 2:
                    assert row[11] < 4
                if prior_fraction == 0:
                    assert list(row) == prior
                if mask == 0:
                    assert all(row[i] == prior[i] for i in range(16) if i not in (2, 3))
                is_changed = row[0] != prior[0]
                changed += is_changed
                brush += row[10] == 1 and is_changed
                displacement += row[10] == 2 and is_changed
                solid += row[2] != 0
                cleared += prior[13] != 0 and row[13] == 0
            assert (changed, brush, displacement, solid, cleared) == tuple(counts[k] for k in ['changed_fraction', 'brush_winners', 'displacement_winners', 'start_solid', 'marker_cleared'])
            assert counts['rounded_delta_cases'] == 2688
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'mixed world semantic trace mismatch')
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

    for name in ['probe_prepared_world_hull.cpp', 'build_prepared_world_hull.cmd', 'verify_prepared_world_hull.py']:
        visit(name)
    receipt = dict(status='VERIFIED MIXED PREPARED BRUSH/DISPLACEMENT SWEPT HULL WORLD GEOMETRY', counts=counts,
                   repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256, originals_unchanged=True,
                   native_winner_transfer_evidence=native_evidence,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence],
                   replaced_callees=['bucket insertion1015b150', 'plane append10001610', 'prepared cache acquire1015c760', 'cache release10261300'],
                   findings=[
                       'Full original10148c30 BSP traversal ->10148540 leaf runs actual optimized/general brush kernels and actual displacement culling/outer/tree/contact/finish routines in one query.',
                       'Only cache storage insertion/append at build time and prepared cache acquire/release at query time are adapted. Original geometry instructions remain unchanged.',
                       'Hardware entry/return observations around actual1014d100 identify selected displacement ordinal and demonstrate real displacement-then-brush and brush-then-displacement winner transfers.',
                       'Final fraction/left-solid, solid flags, contents, complete plane/tags, semantic surface kind/index, displacement flags/marker and material property/flags match across four runs.',
                       'Four powers/four surface shapes, three overlapping leaf layouts, box/general brushes, duplicate cross-leaf references, seven masks including0x2000 surface exclusions, four hull sizes and eight initial states are covered.',
                       'Retained nonzero delta with a rounded unchanged endpoint is included; direct zero-motion swept fixtures do not establish stationary dispatch.',
                       'Normal-entry brush contacts clear a prior displacement marker. Solid/exit paths preserve it, including default surface/material resets.',
                       'Native cache plane/index bytes and original executable sections remain unchanged, as do installed engine/BSP hashes.'],
                   corrected_issue='First native comparison case1416 exposed missing default material-property refresh on solid-exit reset. Native10146afa..10146b13 and10147b78..10147b93 identified full surface copy; one focused correction in new composed adapter passed the complete suite.',
                   limits=[
                       'Synthetic world tables, prepared displacement geometry/cache/culls and material properties are supplied. The installed BSP is read only to initialize container data, then relevant geometry/root/leaf arrays are replaced by controlled tables.',
                       'This is final semantic world geometry trace equality, not every internal work byte/visit or complete original BSP import/material binding.',
                       'Native global cache allocation/growth/lifetime/synchronization and production permutation distribution remain separate.',
                       'Public trace setup/finish, stationary dispatcher, static props/entities/player filtering, benchmark spawn and editor integration remain unverified.',
                       'Surface identities are normalized from fixture names and observed ordinals only in tests; owned code stores no original surface address.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
