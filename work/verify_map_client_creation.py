"""Verify the installed client-creation boundary against the owned x64 policy.

Actual entity allocation/edict attachment, activation and spawn selection are
outside this fixture. Decoded private reference evidence stays in its test folder.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import SERVER,REFERENCES,sha
from verify_sliding_door_spawn import verify_installed_ranges


def main(directory):
    assert sha(SERVER) == REFERENCES[SERVER], 'Installed server differs from the pinned reference'
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    image,evidence=verify_installed_ranges(SERVER,run/'client-creation-reference.local.txt',[
        ('client interface registration',0x1041d390,0x1041d3a5),
        ('client interface singleton factory',0x10142a20,0x10142a26),
        ('client put-in-server dispatch',0x101435f0,0x101436d9),
        ('default client creation',0x102f18e0,0x102f1901),
        ('edict before entity allocation',0x10314960,0x1031497d),
        ('player name assignment',0x101bab40,0x101bab60),
        ('bounded terminated name copy',0x10381bc0,0x10381be6),
    ])
    assert image.string(0x1049613c) == 'ServerGameClients004'
    assert image.string(0x104463fc) == 'player'
    table=image.word(0x105b1ca0)
    assert table == 0x10495584 and image.word(table+5*4) == 0x101435f0
    locator=image.word(table-4)
    assert image.word(locator+4) == 0
    assert image.string(image.word(locator+12)+8) == '.?AVCServerGameClients@@'
    builds={}
    outputs=[]
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        executable=directory/f'creation_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=executable.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0] == machine
        builds[arch]=dict(executable=str(executable.resolve()),sha256=sha(executable),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(executable),str(SERVER) if arch=='x86' else '-'],
                capture_output=True,text=True,timeout=40)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode == 0,(arch,result.returncode,result.stderr)
            output=json.loads(result.stdout)
            assert output == dict(cases=24,passed=True)
            outputs.append(output)
    assert all(output == outputs[0] for output in outputs)
    assert sha(SERVER) == REFERENCES[SERVER], 'Installed server changed'
    receipt=dict(status='VERIFIED BOUNDED CREATION POLICY',cases_per_run=24,repeats_per_arch=2,
        builds=builds,server_sha256=sha(SERVER),server_unchanged=True,
        installed_interface='ServerGameClients004',installed_slot=5,
        verified_spans=[row['name'] for row in evidence],
        findings=['Creation override suppresses the default player factory and default naming.',
                  'Default route publishes the supplied edict before requesting class player, forced index -1, final argument 1.',
                  'A nonnull name invokes the 32-byte name-copy wrapper; null names leave name storage untouched.',
                  'The supplied name-copy primitive fills/truncates; original wrapper enforces termination at byte 31.'],
        limits=['Entity factory allocation/body and actual edict attachment are supplied, not reconstructed.',
                'Reference fixture forces post-creation entity resolution to null; valid-player initialization and HLTV handling are not executed.',
                'The low-level name-copy primitive is an adapter; this is not native CRT-copy parity.',
                'Owned helper verifies routing and arguments; it is not integrated into the editor and does not implement a live player.',
                'Current Valve public eiface.h is context only; installed interface slots are separately verified.',
                'Activation, spawn choice, movement and live map/door effects remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=24,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
