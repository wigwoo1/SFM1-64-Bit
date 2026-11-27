"""Verify local player resolution and server touch scan admission."""

from collections import Counter
from itertools import product
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from verify_sliding_door_spawn import GAME, IFM, SERVER, sha, verify_installed_ranges

CLIENT = GAME/'tf/bin/client.dll'
REFERENCES = {
    SERVER: '47a03f6bce8f519dafe1cfbcb9ba97cf0b0069d6eab57c2a52cffd61c66265d4',
    CLIENT: '91dd1062227b095f1338312aa7268121894970c3e0a5b0d314e42c77f2aba73b',
    IFM: '12504c248047e181d31a8500fb6693d1a2d3c805e841fc2227e41aa4e2d2b47e',
}
QUERY = struct.Struct('<4Ii13I')
REPLY = struct.Struct('<12I12f')


def cases():
    # Exercise the signed-in client's reference, empty slots, serial wrapping,
    # the one-client fallback, and disagreement between the two move types.
    for client, index, (serial, current), direct, count, local, direct_move, local_move in product(
        range(2), (0, 1, 2047, 2048, 65535), ((2, 2), (1, 2), (1026, 2), (2, 1026), (65535, 65535)),
        range(3), (-1, 0, 1, 2), range(5), (2, 8), (2, 8),
    ):
        yield (client, index | (serial << 16), current, direct, count, local, direct_move, local_move,
               0x80, 2, 0, 1, 0, 0, 0, 1, 2, 0)

    # Relink is invoked with an independent actor. A missing tool player does
    # not imply that the server lacks all other solids or moving triggers.
    for flags, solid, solid_flags, edict, world, model, previous, accurate, movement, queued in product(
        (0, 0x1, 0x80, 0x81), (0, 1, 2, 3, 6), (0, 4, 8, 12, 16, 24),
        range(2), range(2), range(3), range(2), range(2), (0, 2, 8, 10), range(2),
    ):
        yield (1, 0x00020001, 2, 2, 1, 4, movement, 2,
               flags, solid, solid_flags, edict, world, model, previous, accurate, movement, queued)


def expected(row, counts):
    (client, handle, serial, direct, max_clients, fallback, direct_move, local_move,
     flags, solid, solid_flags, edict, world, model, previous, accurate, movement, queued) = row
    index = handle % 65536
    client_serial = handle // 65536
    mapped = bool(client and direct == 2 and index <= 2047 and (client_serial - serial) % 1024 == 0)
    local = max_clients == 1 and fallback == 4
    if mapped:
        counts['direct_player'] += 1
        noclip = direct_move == 8
        if local and direct_move != local_move:
            counts['direct_overrides_fallback'] += 1
    elif local:
        counts['fallback_player'] += 1
        noclip = local_move == 8
    else:
        counts['no_player_reports_noclip'] += 1
        noclip = True

    scan = 0
    if edict and not world and (solid != 1 or model):
        if solid_flags in (8, 12, 24):
            scan = 2
        elif solid and solid_flags in (0, 16):
            scan = 1
    passes = flags in (0x80, 0x81)
    counts[('no_scan', 'solid_moved', 'trigger_moved')[scan]] += 1
    if scan == 1 and movement == 8:
        counts['noclip_actor_still_solid'] += 1
    if passes and not scan:
        counts['client_flag_without_scan'] += 1
    if not passes and scan:
        counts['scan_without_client_flag'] += 1
    present = mapped or local
    pose = (0.,)*12
    if present:
        if mapped:
            pose = (100.5, -20., 95., -25., 130., -11., 117., -218., 15.25)
        else:
            pose = (-100., 11.125, 75., 31.25, -240., 18., 117.625, -218.5, 33.75)
        pose += (-16., 177.5, 14.25)
    return (mapped, local, noclip, passes, scan, scan != 0, bool(scan and not queued),
            previous if scan == 1 else 0, accurate if scan else 0, present, present, present, *pose)


def installed_ranges(run):
    server, server_ranges = verify_installed_ranges(SERVER, run/'server-reference.local.txt', [
        ('Client tool handle lookup', 0x10343c60, 0x10343cce),
        ('Single-client local player fallback', 0x102637e0, 0x10263815),
        ('Server tool noclip query', 0x10343d50, 0x10343d81),
        ('Read live eye position and angles', 0x10343980, 0x103439f5),
        ('Snap player from eye position and view offset', 0x10343770, 0x10343803),
        ('Client-only trigger filter', 0x102563e0, 0x10256584),
        ('Player movement relink before nonsolid return', 0x101776a0, 0x101776ca),
        ('Touch relink and engine notification', 0x100cddb0, 0x100cdec5),
        ('Untouch state and scheduling', 0x100bc4c0, 0x100bc505),
    ])
    client, client_ranges = verify_installed_ranges(CLIENT, run/'client-reference.local.txt', [
        ('Local client player selection', 0x1028fe70, 0x1028fecd),
    ])
    ifm, ifm_ranges = verify_installed_ranges(IFM, run/'ifm-reference.local.txt', [
        ('Client and server tool query in mode restoration', 0x101b00f1, 0x101b0123),
        ('Client and server tool query at second restoration site', 0x10209ddb, 0x10209e0d),
        ('Local player snap from the selected camera transform', 0x10276790, 0x1027684b),
        ('Read live view, clear roll and restore FOV', 0x10276850, 0x102768b3),
    ])
    assert server.word(0x10525284 + 4) == 0x10343c60
    assert server.word(0x10525284 + 8) == 0x10343770
    assert server.word(0x10525284 + 12) == 0x10343980
    assert server.word(0x10525284 + 0x18) == 0x10343d50
    assert client.word(0x10579034 + 0x5c) == 0x1028fe70
    assert ifm.string(0x10ac9440) == 'noclip'
    return {'server': server_ranges, 'client': client_ranges, 'ifm': ifm_ranges}


def main(build):
    assert all(sha(path) == checksum for path, checksum in REFERENCES.items())
    run = Path(tempfile.mkdtemp(dir=build, prefix='actor-'))
    evidence = installed_ranges(run)
    rows = list(cases())
    counts = Counter()
    target = b''.join(REPLY.pack(*expected(row, counts)) for row in rows)
    query = run/'queries.bin'
    query.write_bytes(b''.join(QUERY.pack(*row) for row in rows))
    builds = {}
    for arch, machine in (('x86', 0x14c), ('x64', 0x8664)):
        exe = build/f'actor_{arch}.exe'
        image = exe.read_bytes()
        offset, = struct.unpack_from('<I', image, 0x3c)
        assert image[offset:offset+4] == b'PE\0\0'
        assert struct.unpack_from('<H', image, offset+4)[0] == machine
        log = (build/f'actor_{arch}-build.log').read_text()
        assert 'warning ' not in log and 'error ' not in log
        builds[arch] = {'executable': str(exe), 'sha256': sha(exe), 'warnings': 0}
        for repeat in (1, 2):
            output = run/f'{arch}-{repeat}.bin'
            result = subprocess.run([exe, query, output, SERVER if arch == 'x86' else '-'],
                                    capture_output=True, timeout=45)
            (run/f'{arch}-{repeat}.log').write_bytes(result.stdout+result.stderr)
            assert result.returncode == 0, (arch, result.returncode, result.stderr, str(run))
            actual = output.read_bytes()
            assert len(actual) == len(target), (arch, len(actual), len(target))
            if actual != target:
                position = next(i for i, (a, b) in enumerate(zip(actual, target)) if a != b) // REPLY.size
                raise AssertionError((arch, position, rows[position],
                                      REPLY.unpack_from(actual, position*REPLY.size),
                                      REPLY.unpack_from(target, position*REPLY.size)))
    assert len(counts) == 10 and all(counts.values()), counts
    assert all(sha(path) == checksum for path, checksum in REFERENCES.items())
    report = {
        'builds': builds, 'repeats_per_arch': 2, 'matched_rows': len(rows), 'coverage': dict(counts),
        'installed_reference_ranges': evidence, 'originals_unchanged': {str(k): v for k,v in REFERENCES.items()},
        'limitations': 'Local entity records and collision state are supplied by the fixture. Engine callbacks record scan requests. Runtime actor population, physical overlaps, live editor playback and door motion are outside this test.',
    }
    (run/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'rows': len(rows), 'coverage': dict(counts), 'receipt': str(run/'results.json')}))


if __name__ == '__main__':
    main(Path(sys.argv[1]).resolve())
