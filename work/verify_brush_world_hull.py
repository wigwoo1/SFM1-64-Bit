"""Verify composed BSP hull traversal and brush contacts without callee adapters."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_sliding_door_spawn import ENGINE, BSP, sha, verify_installed_ranges

ENGINE_SHA256 = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
BSP_SHA256 = '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'


def main(directory):
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    _, evidence = verify_installed_ranges(ENGINE, run/'brush-world-reference.local.txt', [
        ('complete swept hull BSP traversal', 0x10148c30, 0x10148f4a),
        ('complete hull leaf contact routine', 0x10148540, 0x10148867),
        ('complete optimized hull box kernel', 0x10146850, 0x10146c24),
        ('complete hull brush dispatch/general kernel', 0x10147920, 0x10147c27),
        ('complete runtime surface lookup', 0x10146440, 0x1014646a),
        ('complete original plane copy', 0x100e9720, 0x100e975d),
    ])
    baseline = counts = None
    builds = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = directory/f'brush_world_hull_{arch}.exe'
        log = (directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log, log
        data = exe.read_bytes()
        pe = struct.unpack_from('<I', data, 60)[0]
        assert data[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', data, pe+4)[0] == machine
        assert b'requestedExecutionLevel' in data and b'asInvoker' in data
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0)
        for repeat in (1, 2):
            record = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([str(exe), str(ENGINE) if arch == 'x86' else '-', str(BSP), str(record)],
                                    capture_output=True, text=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr)
            current = json.loads(result.stdout)
            assert current['passed'] and current['cases'] == 3584
            assert current['changed_fraction'] > 0 and current['start_solid'] > 0
            if counts is None:
                counts = current
            assert current == counts
            payload = record.read_bytes()
            assert len(payload) == current['cases']*44
            if baseline is None:
                baseline = payload
            assert payload == baseline, (arch, repeat, 'semantic record mismatch')
    assert sha(ENGINE) == ENGINE_SHA256 and sha(BSP) == BSP_SHA256
    root = Path(__file__).resolve().parent
    receipt = dict(status='VERIFIED COMPOSED SWEPT BRUSH-ONLY WORLD HULL TRACE', counts=counts,
                   benchmark_cases=896, controlled_cases=2688, repeats_per_arch=2, builds=builds,
                   source_sha256={name: sha(root/name) for name in ['source_bsp_brush_only_hull_trace_candidate.h',
                                  'probe_brush_world_hull.cpp', 'build_brush_world_hull.cmd', 'verify_brush_world_hull.py']},
                   semantic_record_bytes=len(baseline), semantic_record_sha256=hashlib.sha256(baseline).hexdigest(),
                   engine_sha256=ENGINE_SHA256, bsp_sha256=BSP_SHA256, originals_unchanged=True,
                   replaced_callees=[], software_code_patches=False,
                   verified_spans=[row['name'] for row in evidence],
                   findings=[
                       'Original10148c30 calls actual10148540, actual10147920/10146850, actual plane copy and actual10146440 surface lookup; no callees are intercepted.',
                       'The owned adapter composes existing hull traversal, leaf mask/dedup policy and both brush kernels. Point APIs are unchanged.',
                       'Final fraction/left-solid bits, solid flags, contents, original plane and all plane tags, and selected surface identity match across four runs.',
                       'Fixtures cover imported benchmark BSP brush geometry and controlled repeated/cross-leaf references, seven masks including0x2080/0x2081, surface flag0x80 profiles including default surfaces, varied hulls and initial fractions.',
                       'Private original executable sections and installed engine/BSP hashes remain unchanged.'],
                   limits=[
                       'Displacement spans are explicitly empty in native fixture. This is brush-only collision, not full benchmark world collision.',
                       'Trace work, runtime collision records and surface tables are supplied fixtures. Full original BSP import, trace wrapper setup/finish, and name/material binding are not executed.',
                       'Stationary query dispatch, static props/entities, player filters, measured spawn and editor integration remain unverified.',
                       'Records compare final public trace semantics; they do not independently compare every internal visit/call or all possible floating-point modes.'])
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(counts=counts, receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
