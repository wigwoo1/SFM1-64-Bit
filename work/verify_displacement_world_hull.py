"""Compare outer world metadata plus real prepared hull sweeps with original code."""
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
    image, evidence = verify_installed_ranges(ENGINE, run/'world-hull-reference.local.txt', [
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
        exe = directory/f'world_hull_{arch}.exe'
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
            assert current['passed'] and current['cases'] == 41472 and current['meshes'] == 48
            for key in ['closer_hits', 'work_changes', 'zero_fraction_metadata', 'misses', 'first_property_hits', 'second_property_hits', 'retained_prior_marker', 'prior_displacement_surface']:
                assert current[key] > 0, key
            assert current['disabled'] == current['cases']//2
            assert current['work_changes'] > current['closer_hits']
            if counts is None:
                counts = current
            assert counts == current
            payload = records.read_bytes()
            assert len(payload) == current['cases']*0x220
            hits = changed = zero_metadata = first_property = second_property = retained_marker = prior_surfaces = 0
            input_bytes = set(range(12)) | set(range(0x30, 0x54))
            mutable_bytes = set(range(0x6c, 0x7c)) | set(range(0x80, 0x8a)) | set(range(0x90, 0x98)) | set(range(0x114, 0x118))
            retained_bytes = set(range(0x220))-input_bytes-mutable_bytes
            for case in range(current['cases']):
                cursor = case*0x220
                work = payload[cursor:cursor+0x220]
                mesh_index = case//864
                q, hull, mode, flag_index = (case//36)%24, (case//12)%3, (case//4)%3, case%4
                marker = (q+mode+hull+flag_index)&1
                prior_surface = bool(q&2)
                contents = [0, 1, 0x80000008][mesh_index%3]
                properties = [101+mesh_index, 501+mesh_index]
                initial_fraction = [1., .25, 0.][mode]
                final_fraction, = struct.unpack_from('<f', work, 0x80)
                assert 0 <= final_fraction <= initial_fraction
                closer = initial_fraction > final_fraction
                assert all(work[i] == 0xa5 for i in retained_bytes)
                initial = bytearray(work)
                struct.pack_into('<4f', initial, 0x6c, 99, 98, 97, 96)
                struct.pack_into('<fIH', initial, 0x80, initial_fraction, 0x24681357, 0xbeef)
                struct.pack_into('<IHH', initial, 0x90, 0x10350ee0 if prior_surface else 0x12345678, 0xabcd, 0xef01)
                struct.pack_into('<I', initial, 0x114, marker)
                if flag_index in (1, 3):
                    assert not closer and work == initial
                if closer:
                    assert struct.unpack_from('<I', work, 0x84)[0] == contents
                    assert struct.unpack_from('<I', work, 0x114)[0] == 1
                    name, prop, flags = struct.unpack_from('<IHH', work, 0x90)
                    slot = bool(struct.unpack_from('<H', work, 0x88)[0]&16)
                    assert name == 0x10350ee0 and flags == 0 and prop == properties[slot]
                    first_property += not slot
                    second_property += slot
                else:
                    for offset, length in [(0x84, 4), (0x90, 8), (0x114, 4)]:
                        assert work[offset:offset+length] == initial[offset:offset+length]
                    retained_marker += marker == 1
                hits += closer
                changed += work != initial
                zero_metadata += work != initial and mode == 2
                prior_surfaces += prior_surface
            assert (hits, changed, zero_metadata, first_property, second_property, retained_marker, prior_surfaces) == tuple(counts[k] for k in ['closer_hits', 'work_changes', 'zero_fraction_metadata', 'first_property_hits', 'second_property_hits', 'retained_prior_marker', 'prior_displacement_surface'])
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'whole outer work-buffer mismatch')
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

    for name in ['probe_displacement_world_hull.cpp', 'build_displacement_world_hull.cmd', 'verify_displacement_world_hull.py']:
        visit(name)
    receipt = dict(status='VERIFIED OUTER WORLD HULL ADAPTER WITH PREPARED DISPLACEMENT SWEEPS', counts=counts,
                   repeats_per_arch=2, builds=builds, source_sha256=sources,
                   record_bytes=len(baseline), record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, original_unchanged=True,
                   permutation_sha256=PERMUTATION_SHA256, production_permutation_bundled=False,
                   verified_spans=[row['name'] for row in evidence],
                   replaced_callees=['bucket insertion1015b150', 'plane append10001610', 'prepared cache acquire1015c760', 'cache release10261300'],
                   findings=[
                       'Full original1014d100 outer adapter,1015c840 hull sweep,1015abb0 contacts, clip helpers and direction normalization execute, with native generated caches checked before use.',
                       'Only fixed-storage insertion/append and prepared-cache acquire/release boundaries are intercepted; no geometry or contact routine is replaced.',
                       'Every byte of the544-byte work buffer matches owned semantic state across both repeated architecture runs, including untouched sentinels.',
                       'Closer hits set the displacement marker, mesh contents, displacement surface identity, zero surface flags and the material property selected by displacement flag16.',
                       'Misses/disabled queries preserve outer metadata and prior marker/surface identity. Inner zero-fraction contact changes survive while outer contents/material fields remain unchanged.',
                       'Both distinct material properties are selected by real contacts; supplied contents include zero, solid1 and high-bit patterns.',
                       'Each native query is also called with a reversed subinterval(.875,.125), producing the identical full buffer to(0,1). Leaf intervals are unused by this outer entry.',
                       'Native cache plane/index bytes remain unchanged, acquired/released services are paired, and original executable sections/installed engine hash remain unchanged.'],
                   limits=[
                       'Prepared mesh/cache/tree inputs and surface property pairs are supplied. This does not verify whole BSP load/material binding.',
                       'Global cache allocation/growth/lifetime/synchronization/invalidation and production permutation distribution remain separate gates.',
                       'Native surface-name addresses are normalized only in test output; owned state uses a semantic boolean with no original pointer dependency.',
                       'World leaf displacement culling/dedup and finish, full world composition, stationary dispatcher, benchmark spawn and editor integration remain unverified.',
                       'Zero-motion cases invoke the swept entry directly, not stationary dispatch.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
