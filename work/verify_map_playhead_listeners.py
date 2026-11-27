"""Identify installed time listeners and exercise their explicitly bounded callbacks.

This is original x86 reference research, not an x64/editor integration test.
Disassembly and receipts must stay in the supplied private test directory.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_simulation_clock import IFM, REFERENCES, sha, verify_installed_ranges

# Class, listener subobject offset, vtable, constructor store, registration call,
# callback, exclusive end. Addresses are metadata, never embedded instructions.
LISTENERS = [
    ('CQChannelGraphWidget', 0x1c, 0x10ab0fbc, 0x100e41e8, 0x100e42fb, 0x100e4110, 0x100e411c),
    ('CQMotionEditorController', 0x24, 0x10ab8fa0, 0x1011bfb0, 0x1011c06d, 0x10118eb0, 0x10118ec0),
    ('CQMotionEditorView', 0x14, 0x10ab90a0, 0x1011e610, 0x1011e73d, 0x1011d860, 0x1011d86c),
    ('CQTimeHelper', 0xc, 0x10ac2954, 0x10165a6f, 0x10165ab3, 0x101654b0, 0x10165623),
    ('CQTimelineTrackView', 0x18, 0x10ac6684, 0x1018c750, 0x1018ce97, 0x1018b420, 0x1018b45d),
    ('CQTimelineOverlayView', 0x14, 0x10ac7348, 0x1019a731, 0x1019a79e, 0x1019a670, 0x1019a6df),
    ('CQViewportArea', 0x38, 0x10ac8b94, 0x101a86d5, 0x101a8ac6, 0x101a1130, 0x101a113c),
    ('CQViewportEditor', 0x18, 0x10ac9f7c, 0x101b2384, 0x101b240f, 0x101b2bb0, 0x101b2c7c),
]


def imported_symbol(image, address):
    optional = image.u32(60)+24
    descriptor = image.offset(image.base+image.u32(optional+104))
    while any(image.b[descriptor:descriptor+20]):
        original, _, _, name, iat = struct.unpack_from('<5I', image.b, descriptor)
        index = 0
        while image.word(image.base+(original or iat)+4*index):
            value = image.word(image.base+(original or iat)+4*index)
            if image.base+iat+4*index == address:
                assert not value & 0x80000000
                return image.string(image.base+name), image.string(image.base+value+2)
            index += 1
        descriptor += 20
    raise AssertionError('Missing import '+hex(address))


def main(directory):
    assert sha(IFM) == REFERENCES[IFM], 'Installed IFM differs from the pinned reference'
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    spans = [('time dispatcher', 0x102beef0, 0x102bef4f),
             ('tracked listener cleanup', 0x101b05d0, 0x101b0729),
             ('filtered listener removal', 0x1035ef90, 0x1035efec),
             ('track filtered subscription', 0x101aac20, 0x101aac8c),
             ('filtered listener registration', 0x1035df40, 0x1035dfcd),
             ('root clip getter', 0x10278e90, 0x10278ea5),
             ('clip timeframe getter', 0x104fe730, 0x104fe742),
             ('active clip child getter', 0x10190e90, 0x10190ea5),
             ('element handle getter', 0x10335830, 0x10335834),
             ('viewport clip changed callback', 0x101afe80, 0x101afe8b)]
    for name, _, _, store, registration, callback, end in LISTENERS:
        # A store through [reg] is six bytes; through [reg+offset] is seven.
        store_length = 6 if name in ('CQChannelGraphWidget', 'CQTimeHelper', 'CQTimelineOverlayView', 'CQViewportEditor') else 7
        spans.extend([(name+' constructor vtable store', store, store+store_length),
                      (name+' registration', registration, registration+5),
                      (name+' callback', callback, end)])
    image, evidence = verify_installed_ranges(IFM, run/'listeners-reference.local.txt', spans)
    repaint_import = imported_symbol(image, 0x10a7bac0)
    assert repaint_import == ('QtGui4.dll', '?update@QWidget@@QAEXXZ')
    cleanup_interfaces = ['IDmAttributeChangedListener', 'IDmAttributeAddedListener',
                          'IDmAttributeRemovedListener', 'IDmElementDeletedListener']
    for address, interface in zip((0x10e79f5c,0x10e79fdc,0x10e79fb0,0x10e79f88),cleanup_interfaces):
        assert image.string(address+8) == '.?AV'+interface+'@@'
    assert [image.word(0x101b072c+4*i) for i in range(5)] == [
        0x101b063f,0x101b0664,0x101b069d,0x101b06c0,0x101b06e3]
    registrations = set()
    # Find all file-backed relative-call encodings to the registration method.
    # The eight matches below are also validated as decoded call instructions.
    # This does not enumerate indirect calls or prove runtime construction.
    for rva, size, raw in image.sections:
        data = image.b[raw:raw+size]
        start = 0
        while True:
            index = data.find(b'\xe8', start)
            if index < 0: break
            start = index+1
            if index+5 <= len(data):
                address = image.base+rva+index
                if (address+5+struct.unpack_from('<i', data, index+1)[0]) & 0xffffffff == 0x102c9430:
                    registrations.add(address)
    assert registrations == {row[4] for row in LISTENERS}, registrations
    rows = []
    for name, offset, table, store, registration, callback, _ in LISTENERS:
        locator = image.word(table-4)
        assert image.word(locator+4) == offset
        assert image.string(image.word(locator+12)+8) == '.?AV'+name+'@@'
        assert image.word(table) == callback
        store_span = next(e for e in evidence if e['name'] == name+' constructor vtable store')
        instruction = store_span['instructions'][0]
        assert instruction['instruction'].startswith('mov')
        assert int.from_bytes(bytes.fromhex(instruction['bytes'])[-4:], 'little') == table
        call_span = next(e for e in evidence if e['name'] == name+' registration')
        assert call_span['instructions'][0]['instruction'].lower() == 'call        102c9430'
        rows.append(dict(name=name,subobject_offset=hex(offset),vtable=hex(table),
                         callback=hex(callback),registration_call=hex(registration)))
    executable = directory/'listeners_x86.exe'
    data = executable.read_bytes()
    pe = struct.unpack_from('<I', data, 60)[0]
    assert data[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', data, pe+4)[0] == 0x14c
    log = (directory/'x86-build.log').read_text(errors='replace')
    assert 'warning C' not in log and 'error C' not in log, log
    results = []
    for repeat in (1, 2):
        p = subprocess.run([str(executable.resolve()), str(IFM)], capture_output=True, text=True, timeout=45)
        (run/f'run-{repeat}.log').write_text(p.stdout+p.stderr)
        assert p.returncode == 0, (p.returncode,p.stderr)
        results.append(json.loads(p.stdout))
    assert results[0] == results[1]
    assert results[0]['cases'] == 360 and results[0]['coverage'] == [48,48,48,24,48,48,48,48]
    assert results[0]['cleanup_cases'] == 360
    assert results[0]['subscription_cases'] == 336
    assert sha(IFM) == REFERENCES[IFM], 'Original changed'
    receipt = dict(status='VERIFIED BOUNDED REFERENCE CALLBACKS',listeners=rows,
        repaint_import=repaint_import,
        cleanup_rtti_interfaces=cleanup_interfaces,
        reference_sha256=sha(IFM),executable=str(executable.resolve()),executable_sha256=sha(executable),
        cases_per_run=results[0]['cases'],cleanup_cases_per_run=results[0]['cleanup_cases'],
        subscription_cases_per_run=results[0]['subscription_cases'],
        repeated_runs=2,coverage=results[0]['coverage'],
        compiler_warnings=0,original_unchanged=True,verified_spans=[e['name'] for e in evidence],
        limits=['Original x86 only; no owned x64 behavior or editor integration verified.',
               'Qt update import and track/overlay virtual callees are supplied observers.',
               'Base matrix observes viewport information refresh, tracked-listener cleanup and root getter. Added cleanup matrix executes the actual cleanup and type-0 filtered removal; information refresh and pool frees remain observed.',
               'Cleanup matrix covers missing/rejected elements, null/present owner, empty/absent/head/tail/duplicate callback matches, zero/one/two records, packed flags, and unknown record kind 5. Kinds 1-4 remain unexecuted.',
               'Base/cleanup matrices use a null root. The subscription matrix supplies root/active clips, timeframes and virtual-slot-5c children. Active-clip selection, child lookup and tracking-vector insertion are adapters, not original selection/container execution.',
               'Subscription matrix executes original registration and clip-change callback; pool allocation is supplied, including one failed allocation. The common viewport-information refresh callee remains observed, so its transitive effects remain unverified.',
               'Time helper exercises flagged notification only; ordinary auto-scroll and nested callbacks remain unverified.',
               'Direct registrations identified statically; no claim all listeners are instantiated in a live session.',
               'No conclusion that transitive callees cannot affect the server or live player.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases=results[0]['cases'],cleanup_cases=results[0]['cleanup_cases'],
                         subscription_cases=results[0]['subscription_cases'],
                         repeated_runs=2,listeners=len(rows),receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory.resolve())
