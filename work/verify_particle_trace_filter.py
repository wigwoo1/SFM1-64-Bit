"""Check the TF client particle filter with controlled entity and game rules."""
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_ifm_draw_packet import check_ranges


ROOT = Path(__file__).resolve().parent.parent
OUT = Path(os.environ.get('SFM64_ENGINE_POINT_TEST_DIRECTORY', ROOT/'outputs/particle-trace-filter'))
CLIENT = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/tf/bin/client.dll')
CLIENT_SHA = '91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def collision_rules(run, executables):
    groups = (-(2**31), *range(-2, 41), 2**31-1)
    stock_mask = 0x0200400B
    masks = (0, stock_mask, stock_mask | 0x04000000, 0xffffffff)
    rows = list(itertools.product(groups, groups, masks))
    inputs = run/'collision-group-queries.bin'
    inputs.write_bytes(b''.join(struct.pack('<iiI', *row) for row in rows))
    outputs = {}
    for arch, exe in executables.items():
        dest = run/f'collision-groups-{arch}.bin'
        completed = subprocess.run([exe['path'], CLIENT if arch=='x86' else '-', inputs, dest, '--groups-only'],
                                   capture_output=True, timeout=50)
        assert completed.returncode == 0, (arch, completed.stdout, completed.stderr)
        outputs[arch] = list(struct.iter_unpack('<3B', dest.read_bytes()))
        assert len(outputs[arch]) == len(rows)
    for row, native, owned in zip(rows, outputs['x86'], outputs['x64'], strict=True):
        assert native == owned, (row, native, owned)
        assert all(x in (0, 1) for x in native)
    truth = dict(zip(rows, outputs['x86'], strict=True))
    for first, second, mask in rows:
        assert truth[first, second, mask][:2] == truth[second, first, mask][:2]
    assert truth[5,26,stock_mask] == (1,1,1)
    assert truth[8,26,stock_mask] == (1,0,1)
    assert truth[13,26,stock_mask] == (1,1,1)
    assert truth[1,17,stock_mask] == (1,1,1)
    assert truth[0,1,stock_mask] == (1,1,0)
    assert truth[0,1,stock_mask | 0x04000000] == (1,1,1)
    assert truth[13,21,stock_mask] == (1,1,1)

    # Exercise the geometry and owner checks with the actual native vtables.
    # The independent expected output combines mask/exclusion checks with the
    # direct original matrix above.
    composed = [(mask,first,second,3,6,0,0,1,0,0,0) for first,second,mask in rows]
    composed.extend((mask,first,second,model,solid,move,translucent,exists,exclusion,0,0)
        for mask,first,second,model,solid,move,translucent,exists,exclusion in itertools.product(
            masks, (0,1,26), (1,4,5,8,13,17,26), (1,3), (0,1,6), (0,7), (0,1), (0,1), range(6)))
    composed_inputs = run/'composed-client-filter-queries.bin'
    composed_inputs.write_bytes(b''.join(struct.pack('<I5i5I', *row) for row in composed))
    expected = []
    for mask, first, second, model, solid, move, translucent, exists, exclusion, _, _ in composed:
        geometry = mask & 0x02000000 or model==1 and solid in (1,6)
        content = (not translucent or mask&2) and (move!=7 or mask&0x4000)
        eligible = exists and geometry and content and exclusion not in (1,2,3)
        _, group_allowed, entity_allowed = truth[first,second,mask]
        expected.append((int(bool(eligible and group_allowed and entity_allowed)), 0))
    for arch, exe in executables.items():
        dest = run/f'composed-client-filter-{arch}.bin'
        completed = subprocess.run([exe['path'], CLIENT if arch=='x86' else '-', composed_inputs,
                                   dest, '--sfm-rules'], capture_output=True, timeout=50)
        assert completed.returncode == 0, (arch, completed.stdout, completed.stderr)
        observed = list(struct.iter_unpack('<2I', dest.read_bytes()))
        assert len(observed) == len(expected)
        for row, actual, want in zip(composed, observed, expected, strict=True):
            assert actual == want, (arch, row, actual, want)
    return {
        'direct_rule_cases': len(rows),
        'group_values_covered': len(groups),
        'group_pairs_where_tf_overrides_base': sum(base != tf for base,tf,_ in outputs['x86'])//len(masks),
        'composed_real_rule_cases': len(composed),
        'composed_real_rule_hits': sum(hit for hit,_ in expected),
        'stock_rockets_player': bool(truth[5,26,stock_mask][1]),
        'stock_rockets_player_movement': bool(truth[8,26,stock_mask][1]),
        'stock_rockets_ordinary_projectile': bool(truth[13,26,stock_mask][1]),
        'debris_base_entity_without_debris_mask': bool(truth[0,1,stock_mask][2]),
    }


def main():
    assert sha(CLIENT) == CLIENT_SHA
    ranges = [
        ('Owner serial resolution', 0x100124b0, 0x100124dc),
        ('Particle filter constructor call with no optional callback', 0x100165af, 0x100165f9),
        ('Standard geometry and mask rules', 0x1019c4e0, 0x1019c586),
        ('Entity and owner exclusion', 0x1019c880, 0x1019c8e0),
        ('Filter constructor and predicate', 0x1019c8e0, 0x1019c9b3),
        ('Base entity ShouldCollide', 0x10015370, 0x1001538f),
        ('Base game ShouldCollide', 0x100f8d50, 0x100f8e4e),
        ('TF game ShouldCollide', 0x102379a0, 0x10237a60),
    ]
    image, evidence = check_ranges(CLIENT, 'native-client-scale-disassembly.txt', ranges)
    assert image.word(0x1050dc34) == 0x1019c910
    assert image.word(0x1050dc38) == 0x1038ece0
    assert image.word(0x104f1324+0x70) == 0x100f8d50
    assert image.word(0x1053a4b4+0x70) == 0x102379a0
    assert image.word(0x104deec4+0x280) == image.word(0x104c7974+0x280) == 0x10015370
    run = Path(tempfile.mkdtemp(prefix='client-filter-', dir=OUT))

    mask_values = [1 | (0x2 if window else 0) | (0x4000 if moveable else 0) |
                   (0x02000000 if monster else 0)
                   for window, moveable, monster in itertools.product((0, 1), repeat=3)]
    rows = list(itertools.product(mask_values, (0, 1, 26), (0, 4, 26),
                                  (1, 3), (0, 1, 6), (0, 7), (0, 1),
                                  (0, 1), range(6), (0, 1), (0, 1)))
    inputs = run/'queries.bin'
    inputs.write_bytes(b''.join(struct.pack('<I5i5I', *row) for row in rows))
    expected = []
    for mask, _, _, model, solid, move, translucent, exists, exclusion, entity, rules in rows:
        blocked_geometry = (
            model != 1 or solid not in (1, 6)
        ) and mask & 0x02000000 == 0
        blocked_contents = translucent and mask & 2 == 0 or move == 7 and mask & 0x4000 == 0
        if not exists or blocked_geometry or blocked_contents or exclusion in (1, 2, 3):
            expected.append((0, 0))
        elif not entity:
            expected.append((0, 1))
        else:
            expected.append((rules, 3))

    executables = {}
    for arch, machine in [('x86', 0x14c), ('x64', 0x8664)]:
        exe = OUT/f'particle_filter_{arch}.exe'
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        assert struct.unpack_from('<H', raw, pe+4)[0] == machine
        executables[arch] = {'path': str(exe), 'sha256': sha(exe)}
        result = run/f'{arch}.bin'
        completed = subprocess.run([exe, CLIENT if arch == 'x86' else '-', inputs, result],
                                   capture_output=True, timeout=50)
        assert completed.returncode == 0, (arch, completed.stdout, completed.stderr)
        observed = list(struct.iter_unpack('<2I', result.read_bytes()))
        assert len(observed) == len(expected)
        for row, actual, want in zip(rows, observed, expected, strict=True):
            assert actual == want, (arch, row, actual, want)
    real = collision_rules(run, executables)
    assert sha(CLIENT) == CLIENT_SHA
    receipt = {
        'status': 'PASS', 'cases': len(rows), 'accepted': sum(hit for hit, _ in expected),
        'entity_rule_calls': sum(bool(calls & 1) for _, calls in expected),
        'group_rule_calls': sum(bool(calls & 2) for _, calls in expected),
        'client_sha256': CLIENT_SHA, 'client_unchanged': True, 'executables': executables,
        'original_instruction_bytes_checked': sum(row['bytes'] for row in evidence),
        'scope': 'Original client filter, owner lookup, base entity and TF game rules with controlled model and entity data.',
        **real,
    }
    (run/'results.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt))
    print('Receipt', run/'results.json')


if __name__ == '__main__':
    main()
