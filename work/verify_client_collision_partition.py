"""Compare the owned partition update and list requests with the installed client."""

from collections import Counter
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_ifm_draw_packet import check_ranges
from verify_translated_brush_point_trace import benchmark_doors


ROOT = Path(__file__).resolve().parent.parent
OUT = Path(os.environ.get('SFM64_CLIENT_COLLISION_TEST_DIRECTORY',
                          ROOT / 'outputs/client-collision-partition'))
CLIENT = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/tf/bin/client.dll')
BSP = CLIENT.parents[2] / 'tf_movies/maps/mtt_soldier.bsp'
QUERY = struct.Struct('<7I2i25f')
REPLY = struct.Struct('<7Ii18f')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def expected(row):
    source_flags, handle, solid_flags, solid_type, mode, bloat, extra, model, kind, radius = row[:10]
    origin, angles, box, specified, cached = row[10:13], row[13:16], row[16:22], row[22:28], row[28:34]
    solid = solid_type != 0 and not solid_flags & 4
    no_rotate = solid_type in (0, 2) or solid_flags & 64
    result_angles = (0., 0., 0.) if no_rotate else angles
    insert_mask = 0x80 + (0x200 if extra else 0)
    if solid and model and kind in (1, 3) and (model == 1 or radius != 0.):
        insert_mask += 4
    active = bool(source_flags & 0x8000 and handle != 0xffff and
                  (solid or solid_flags & 8 or source_flags & 0x40000))
    flags = source_flags & ~0x8000
    move = (0.,)*6
    coverage = 'no update'
    if active:
        move = origin*2
        coverage = 'point'
        if radius != 0.:
            if flags & 0x4000:
                flags &= ~0x4000
                if solid_type == 5:
                    coverage = 'point with client padding'
                elif mode == 3:
                    coverage = 'specified bounds'
                    move = tuple(f32(a+b) for a,b in zip(move, specified))
                else:
                    assert mode == 0 and solid_type != 6 and not any(result_angles)
                    coverage = 'translated OBB'
                    move = tuple(f32(a+b) for a,b in zip(move, box))
                    if solid_flags & 128:
                        coverage = 'translated OBB with trigger bounds'
                        move = tuple(f32(a+b) for a,b in zip(move, (-bloat,-bloat,0,bloat,bloat,bloat/2)))
                cached = tuple(f32(a-b) for a,b in zip(move, origin*2))
            else:
                coverage = 'cached bounds'
                move = tuple(f32(a+b) for a,b in zip(move, cached))
            move = tuple(f32(a+b) for a,b in zip(move, (-1.,)*3+(1.,)*3))
    return coverage, REPLY.pack(flags, 1, handle, 0x28c, insert_mask, int(active), handle if active else 0,
                                model, *origin, *result_angles, *move, *cached)


def queries(doors):
    for model, origin, box_min, box_max, _ in doors:
        box = box_min + box_max
        specified = tuple(f32(v + (-0.125 if i < 3 else 0.25)) for i, v in enumerate(box))
        for shift in [(0, 0, 0), (512.125, -17.875, 83.5), (-0.25, 122.5, -520.0625)]:
            moved = tuple(f32(v+d) for v,d in zip(origin, shift))
            for mode, solid_type, solid_flags, entity_flags, handle, radius in itertools.product(
                    (0, 3), (0, 1, 2, 5, 6), (0, 4, 8, 12, 64, 128, 136),
                    (0, 0x8000, 0xc000, 0x48000, 0x4c000), (model, 0xffff), (0., -0., 14.5)):
                if mode == 0 and solid_type == 6 and entity_flags & 0x4000 and radius != 0.:
                    continue
                angles = (0., 0., 0.) if mode == 0 else (15.25, -25.5, 60.125)
                yield (entity_flags | 0x0500000a, handle, solid_flags, solid_type, mode, 7, 0,
                       model+1, 1, radius, *moved, *angles, *box, *specified, *box)

    for model, kind, extra, radius, handle in itertools.product(
            (-32768, -1, 0, 1, 2, 32767), (-1, 0, 1, 2, 3, 17), (0, 1, 255),
            (0., -0., 2.**-126, -2.**-126, float('nan')), (0, 0xfffe, 0xffff)):
        yield (0x8000, handle, 0x1040, 1, 7, 0, extra, model, kind, radius,
               -0., -58.25, 16384., 20., -9.5, 320.,
               -1., -2., -3., 1., 2., 3., -1., -2., -3., 1., 2., 3., -1., -2., -3., 1., 2., 3.)


def main():
    protected = {
        CLIENT: '91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b',
        BSP: '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea',
    }
    assert all(digest(path) == value for path, value in protected.items())
    image, evidence = check_ranges(CLIENT, 'native-client-scale-disassembly.txt', [
        ('Door collision accessor', 0x10011e60, 0x10011e67),
        ('Base client list update', 0x1003aa10, 0x1003aa64),
        ('Base client solid class', 0x1003ca30, 0x1003caa9),
        ('Base model index getter', 0x10038f60, 0x10038f68),
        ('Base absolute pose getters', 0x1003f010, 0x1003f030),
        ('Base absolute transform early read', 0x1003d150, 0x1003d194),
        ('Collision origin and model lookups', 0x100cdce0, 0x100cdcf8),
        ('Collision angle choice', 0x100ce0f0, 0x100ce111),
        ('Client partition refresh', 0x100ce830, 0x100ce967),
        ('Client world bounds cache', 0x100cf290, 0x100cf38d),
        ('Surrounding bounds dispatch and point route', 0x100ced70, 0x100cedec),
        ('Surrounding bounds OBB and settings', 0x100cedec, 0x100cee0f),
        ('Specified surrounding bounds', 0x100ceffc, 0x100cf069),
        ('OBB surrounding bounds', 0x100ceb40, 0x100ceb9f),
        ('World-aligned OBB transform', 0x100ce300, 0x100ce3f3),
        ('Trigger bounds expansion', 0x100cea90, 0x100ceb38),
    ])
    expected_slots = {
        0x104c6b94: {0x0c:0x10011e60, 0x2c:0x1003f010, 0x30:0x1003f020,
                     0x168:0x1003aa10, 0x18c:0x1003ca30},
        0x104eb984: {0x0c:0x100cea90, 0x18:0x100cdcf0, 0x20:0x100cdce0,
                     0x24:0x100ce0f0, 0x2c:0x100cded0, 0x30:0x1003bb30, 0x3c:0x100cf290},
        0x100cf26c: {0x00:0x100cedec, 0x0c:0x100ceffc},
    }
    assert all(image.word(table+offset) == address for table, slots in expected_slots.items()
               for offset,address in slots.items())
    assert [image.word(a) for a in (0x104b9ee0,0x104ba584,0x104c055c)] == [0,0x3f800000,0x3f000000]
    doors = list(benchmark_doors())
    assert [door[0] for door in doors] == [1,3,5,8,12]
    run = Path(tempfile.mkdtemp(prefix='client-partition-', dir=OUT))
    rows = [QUERY.unpack(QUERY.pack(*row)) for row in queries(doors)]
    query_file = run / 'input.bin'
    query_file.write_bytes(b''.join(QUERY.pack(*row) for row in rows))
    predictions = [expected(row) for row in rows]
    expected_bytes = b''.join(record for _,record in predictions)
    (run / 'expected.bin').write_bytes(expected_bytes)
    builds = {}
    for arch, machine in [('x86',0x14c),('x64',0x8664)]:
        executable = OUT / f'partition_{arch}.exe'
        raw = executable.read_bytes()
        pe, = struct.unpack_from('<I', raw, 0x3c)
        assert struct.unpack_from('<H', raw, pe+4)[0] == machine
        assert 'warning' not in (OUT / f'partition_{arch}-build.log').read_text().lower()
        for repetition in range(2):
            reply = run / f'{arch}-{repetition}.bin'
            result = subprocess.run([executable, CLIENT if arch=='x86' else '-', query_file, reply],
                                    timeout=40, capture_output=True)
            (run / f'{arch}-{repetition}.log').write_bytes(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr.decode(errors='replace'), run)
            observed = reply.read_bytes()
            assert len(observed) == len(expected_bytes)
            if observed != expected_bytes:
                mismatch = next(i for i, (a,b) in enumerate(zip(observed,expected_bytes)) if a!=b) // REPLY.size
                got = REPLY.unpack_from(observed, mismatch*REPLY.size)
                raise AssertionError((arch, mismatch, rows[mismatch], got, REPLY.unpack(predictions[mismatch][1]), run))
        builds[arch] = {'path':str(executable), 'sha256':digest(executable), 'warnings':0}
    assert all(digest(path) == value for path,value in protected.items())
    result = dict(status='VERIFIED client partition update and base lists', doors=[door[0] for door in doors],
                  cases=len(rows), counts=Counter(name for name,_ in predictions), repetitions=2,
                  owned_spatial_snapshot_and_refusal_checks='PASS', builds=builds,
                  installed_client_bytes_checked=sum(record['bytes'] for record in evidence), originals_unchanged=True,
                  scope='Verified client logic with resolved poses and controlled external services. Dynamic partition history and live entity state are unverified.')
    (run / 'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(run / 'results.json')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
