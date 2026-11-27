"""Compare installed spawn placement requests with an owned x64 policy."""
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
    image,evidence=verify_installed_ranges(SERVER,run/'spawn-placement-reference.local.txt',[
        ('full game-rules spawn placement',0x102fb050,0x102fb240),
    ])
    table=0x10506404;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFGameRules@@'
    assert image.word(table+0x124)==0x102fb050
    for address,value in [(0x10446b50,500.0),(0x10445094,1.0)]:
        assert struct.unpack_from('<f',image.b,image.offset(address))[0]==value
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'placement_{arch}.exe';log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=72,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED SPAWN PLACEMENT REQUESTS',cases_per_run=72,repeats_per_arch=2,builds=builds,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['The full installed game-rules placement routine executes with a clean selected entity and supplied services.',
                  'It obtains hull bounds twice and requests a500-unit downward hull trace from selected cached world origin, mask0x200400b, ignored player and group0.',
                  'It requests local origin at supplied trace endpoint plus1 in Z, zero velocity and selected entity angles, then resets player vectors9A8/9B4 and requests eye angles.',
                  'Placement returns the selected entity. Callback order, all geometry/mask/owner arguments, reset notifications and finite vector values match the x64 helper.',
                  'Whole unrelated player bytes and selected entity bytes remain unchanged; actual transform setters are intercepted, so requested placement is not executed physics.'],
        limits=['Spawn selector, rule bounds, trace10019790, transform/velocity/eye setters and network changed-field hooks are supplied adapters.',
                'Four hardware breakpoints are used; after trace completion its breakpoint is reassigned to the final eye setter. Original executable bytes remain unchanged.',
                'No engine collision query or resulting map contact executes. Supplied trace endpoints are fixtures, not measured mtt_soldier positions.',
                'Dirty selected transforms, null selected entities, parent transforms, trace diagnostic fields and full Spawn/welcome composition are not covered.',
                'The owned host policy is isolated from editor actors. Live team/world state and benchmark placement remain unverified.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=72,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
