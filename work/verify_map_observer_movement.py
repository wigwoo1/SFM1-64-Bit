"""Bounded player movement requests, with explicit physics/list service adapters."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import SERVER, REFERENCES, sha
from verify_sliding_door_spawn import verify_installed_ranges


def main(directory):
    assert sha(SERVER)==REFERENCES[SERVER], 'Unrecognized installed server'
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    image,evidence=verify_installed_ranges(SERVER,run/'movement-reference.local.txt',[
        ('movement setter',0x100c2710,0x100c28ee),
        ('physics collision notification',0x100ccb40,0x100ccbae),
        ('simulation context refresh',0x10199170,0x101991de),
        ('context membership query',0x101997e0,0x101997fc),
        ('context enable',0x101998f0,0x10199927),
        ('context disable',0x10199930,0x1019996f),
        ('moving list exclusion refresh',0x100cd200,0x100cd27d),
        ('moving list wrapper',0x10110d80,0x10110d93),
        ('installed player predicate',0x102468e0,0x102468e3),
    ])
    table=0x1050eaac;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFPlayer@@'
    assert image.word(table+0x158)==0x102468e0
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'movement_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=90)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=12288,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED PLAYER MOVEMENT',cases_per_run=12288,repeats_per_arch=2,builds=builds,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Same movement type only updates the narrowed collision byte; identical bytes return without downstream refresh.',
                  'Leaving type 8 clears flag bit 4. Changed types 0 and 10 notify supplied physics objects, refresh prediction/simulation bytes and simulation-context bit 2.',
                  'Prediction byte can be dirtied by an intermediate value even when its final value equals its initial value.',
                  'Installed player predicate returns true: both requested types clear moving-list exclusion bit 23 when set, then invoke the entity-list refresh wrapper.',
                  'Actual context enable/disable routines run against supplied manager callbacks. Original caller fields and callback order match the owned x64 helper.'],
        limits=['Only requested movement 0 and 10 are implemented; other requests are rejected by the owned helper.',
                'Physics simulation predicate returns false through an observer; physics enumeration/notifications are supplied callbacks, not real physics objects.',
                'Entity-list manager mutation is intercepted; broadphase relinking, map touches, real collision and door activation are not verified.',
                'Network dirty-byte and no-edict paths only; external edict bookkeeping is not executed.',
                'Host callbacks do not mutate unrelated actor state. This remains an isolated helper, not editor player integration.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=12288,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
