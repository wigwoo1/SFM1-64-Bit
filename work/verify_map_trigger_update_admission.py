"""Verify actor admission to engine trigger-update calls, not actual overlaps."""
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
    image,evidence=verify_installed_ranges(SERVER,run/'trigger-update-reference.local.txt',[
        ('movement command dispatch',0x101ced80,0x101cf0c9),
        ('move helper impact processing',0x101776a0,0x101778af),
        ('actor trigger update admission',0x100cddb0,0x100cdec5),
        ('mark for entity update',0x100bc4c0,0x100bc503),
        ('helper singleton publication',0x1041e820,0x1041e831),
        ('game movement initializer',0x1042be50,0x1042be69),
    ])
    assert image.word(0x105bb590)==0x104a9d84
    table=0x104a9d84;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCMoveHelperServer@@'
    assert image.word(table+0x10)==0x101776a0
    assert image.word(0x105ec508)==0x106cc730
    table=0x10504bcc;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFGameMovement@@'
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'trigger_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=1024,caller_cases=256,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED TRIGGER UPDATE ADMISSION',cases_per_run=1024,caller_cases_per_run=256,
        repeats_per_arch=2,builds=builds,server_unchanged=True,server_sha256=sha(SERVER),
        verified_spans=[row['name'] for row in evidence],
        findings=['Movement processor101ced80 invokes supplied move-helper virtual+10 after movement setup/processing/finish. Static singleton/vtable metadata identifies the actual helper entry101776a0.',
                  'Actual helper101776a0 calls actor trigger-update admission100cddb0 before checking non-solid bit4.',
                  'Missing edict or excluded-actor pointer returns without update. Otherwise solid type nonzero with neither flag4 nor flag8 selects engine+12c; flag8 selects engine+130.',
                  'Non-solid flag4 without trigger flag8 requests neither engine update. Actual helper non-solid early-return path is executed in256 cases.',
                  'Eligible updates execute original100bc4c0: increment actor84, set flag24 and notify entity update list only when flag24 was clear.',
                  'Engine callbacks receive original edict/collision pointer/motion argument and global byte. Actor storage outside update counter/flag is unchanged.'],
        limits=['Engine overlap enumeration, trigger outputs, collision solving, touch callbacks and doors do not execute; engine virtuals are supplied callbacks.',
                'Collision mode virtual returns2; mode1 diagnostics/model-parent lookup are not covered.',
                'Entity update-list service10111000 is intercepted; actual list mutation and engine reentrancy are not verified.',
                'Full movement processor and solid-actor impact loop are statically inspected, not executed. Only the helper non-solid path executes.',
                'The previous observer controller requested flag4; combined fixtures constrain this route, but do not establish live benchmark actor state or other trigger paths.',
                'Owned policy remains isolated; no editor integration or live SFM control.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=1024,caller_cases_per_arch=256,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
