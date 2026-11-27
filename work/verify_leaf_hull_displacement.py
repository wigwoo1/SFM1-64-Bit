"""Compare hull leaf filtering/visits/finish with actual native displacement contacts."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from verify_sliding_door_spawn import ENGINE, sha, verify_installed_ranges

ENGINE_SHA256 = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
PERMUTATION_SHA256 = '35e914e111dddaa8e76930d29c4c2f46156130b7f2d00ff7734274cb947adbe6'


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    image, evidence = verify_installed_ranges(ENGINE, run/'leaf-hull-reference.local.txt', [
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
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'leaf_hull_displacement_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        assert binary[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', binary, pe+4)[0] == machine
        assert struct.unpack_from('<H', binary, pe+24)[0] == (0x10b if arch == 'x86' else 0x20b)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            records = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(fixture), str(records)], capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 30720 and current['grids'] == 16
            for key in ['outer_calls', 'empty_calls', 'closer_hits', 'new_start_solid', 'zero_fraction_metadata', 'visit_changes', 'duplicate_calls']:
                assert current[key] > 0, key
            if counts is None:
                counts = current
            assert counts == current
            payload = records.read_bytes()
            cursor = total_calls = empty_calls = hits = new_solid = zero_metadata = visit_changes = duplicates = 0
            orders = [[0, 1, 0, 2, 3, 1, 2, 3], [3, 2, 1, 0, 1, 3], [], [1, 1, 0], [0, 1, 2, 3]]
            cull_contents, visit_indices = [1, 0x80, 0x2080, 1], [2, 0, 3, 1]
            for case in range(current['cases']):
                q, mode, policy = (case//120)%16, (case//8)%3, case%8
                mask = [0, 1, 0x80, 0x2080, 0xffffffff][(case//24)%5]
                generation = 13+q%3
                swept, brushes, start_solid = policy not in (4, 6), policy in (1, 3), policy in (2, 3)
                all_solid = policy == 4 and q%3 == 0
                initial_fraction = [1., .25, 0.][mode]
                initial_visits = [generation if policy in (5, 6) or policy == 7 and q&(1 << i) else generation^0x12345 for i in range(4)]
                count, = struct.unpack_from('<I', payload, cursor)
                cursor += 4
                assert count <= 8
                calls = list(struct.unpack_from('<'+str(count)+'I', payload, cursor))
                cursor += 4*count
                work = payload[cursor:cursor+0x220]
                cursor += 0x220
                visits = list(struct.unpack_from('<4I', payload, cursor))
                brush_visit, = struct.unpack_from('<I', payload, cursor+16)
                cursor += 20
                assert all(i < 4 and cull_contents[i]&mask for i in calls)
                assert brush_visit == (generation if brushes else generation^0x12345)
                assert struct.unpack_from('<I', work, 0xfc)[0] == mask
                assert work[0x100:0x102] == bytes([0, swept])
                assert struct.unpack_from('<I', work, 0x104)[0] == 0x11111111
                assert struct.unpack_from('<II', work, 0x11c) == (0, generation)
                assert struct.unpack_from('<I', work, 0x128)[0] == 0x22222222
                assert struct.unpack_from('<I', work, 0x150)[0] == 0x33333333
                final_fraction, = struct.unpack_from('<f', work, 0x80)
                assert 0 <= final_fraction <= initial_fraction
                if swept:
                    assert len(set(calls)) == len(calls)
                if mode == 2:
                    assert len(calls) <= 1
                if brushes and start_solid or not orders[q%5]:
                    assert not calls and visits == initial_visits
                    assert work[0x8a:0x8c] == bytes([all_solid, start_solid])
                else:
                    predicted = initial_visits.copy()
                    consumed = 0
                    for i in orders[q%5]:
                        if not cull_contents[i]&mask:
                            continue
                        if swept:
                            if predicted[visit_indices[i]] == generation:
                                continue
                            predicted[visit_indices[i]] = generation
                        if i in calls:
                            assert calls[consumed] == i
                            consumed += 1
                            if consumed == len(calls) and final_fraction == 0:
                                break
                    assert consumed == len(calls) and predicted == visits
                    marker, = struct.unpack_from('<I', work, 0x114)
                    if marker:
                        normal = struct.unpack_from('<3f', work, 0x6c)
                        delta = struct.unpack_from('<3f', work, 0x3c)
                        f32 = lambda v: struct.unpack('<f', struct.pack('<f', v))[0]
                        dot = f32(f32(f32(normal[1]*delta[1])+f32(normal[0]*delta[0]))+f32(normal[2]*delta[2]))
                        if dot > 0:
                            assert work[0x8a:0x8c] == b'\x01\x01'
                total_calls += count
                empty_calls += count == 0
                hits += final_fraction < initial_fraction
                new_solid += not start_solid and work[0x8b] != 0
                zero_metadata += mode == 2 and work[0x6c:0x7c] != struct.pack('<4f', 0, 0, 1, 96)
                visit_changes += visits != initial_visits
                duplicates += len(calls)-len(set(calls))
            assert cursor == len(payload)
            assert (total_calls, empty_calls, hits, new_solid, zero_metadata, visit_changes, duplicates) == tuple(counts[k] for k in ['outer_calls', 'empty_calls', 'closer_hits', 'new_start_solid', 'zero_fraction_metadata', 'visit_changes', 'duplicate_calls'])
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'whole hull leaf work/visit/call mismatch')
    assert sha(ENGINE) == ENGINE_SHA256
    root = Path(__file__).resolve().parent
    sources = {}

    def visit(name):
        if name in sources:
            return
        source = root/name
        sources[name] = sha(source)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', source.read_text(), re.M):
            visit(include)

    for name in ['probe_leaf_hull_displacement.cpp', 'build_leaf_hull_displacement.cmd', 'verify_leaf_hull_displacement.py']:
        visit(name)
    receipt = dict(status='VERIFIED HULL LEAF DISPLACEMENT PHASE WITH ACTUAL CONTACTS', counts=counts,
                   repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence],
                   replaced_callees=['bucket insertion1015b150', 'plane append10001610', 'prepared cache acquire1015c760', 'cache release10261300'],
                   findings=[
                       'Full original10148540 leaf routine executes mask/visit order, hull-expanded1014e790 culling, actual1014d100 ->1015c840 ->1015abb0 contacts and1014cfa0 finish.',
                       'Caches are generated by intact original1015c630 and byte-checked. During generation only fixed-capacity insert/append services are intercepted. During queries only prepared cache acquire/release are intercepted.',
                       'Hardware observations at bounds/outer entries execute through unchanged code, verify native expanded min/max/query arguments and record actual candidate order.',
                       'Every544-byte work buffer, four displacement visit slots, dummy-brush visit and ordered outer calls match owned composition across repeated x86/x64 runs. Only three fixture pointers are normalized.',
                       'Contents filtering precedes displacement visit marking. Swept visits suppress repeats; nonswept flag bypasses marks and permits duplicate calls. Visit indices are permuted relative to mesh ordinals.',
                       'Zero-brush leaves bypass startSolid shortcut; nonempty brush spans retain it after the zero-contents dummy brush is marked. Empty displacement spans skip finish.',
                       'Bounds are expanded by hull extents before the0.03125-padded native gate. Masked, previsited, culled/far and runtime-disabled candidates are covered.',
                       'After an actual outer call, fraction zero stops the candidate loop. Finish can set all/startSolid from retained marker and positive normal dot delta even with no new contact.',
                       'Four powers/four surface shapes/four meshes per grid cover real contact changes, both surface property branches from prior verified adapter, zero metadata changes, duplicates and visited-state changes.',
                       'Original executable sections and installed engine hash stay unchanged; native cached planes and indices remain byte-identical.'],
                   limits=[
                       'Brush phase uses an empty span or one zero-contents brush. Real brush contact plus displacement composition is the next gate, not proven here.',
                       'Prepared geometry/tree/cache/cull records and material properties are supplied, not whole original BSP import/material binding.',
                       'Native global cache allocation/growth/lifetime/synchronization and production permutation distribution are separate.',
                       'World BSP traversal with mixed brush/displacement contacts, trace setup/finish, stationary dispatcher, benchmark spawn and editor integration remain unverified.',
                       'Nonswept flag cases invoke the hull leaf entry directly to test visit policy; they do not verify stationary dispatch.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
