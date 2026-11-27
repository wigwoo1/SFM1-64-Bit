"""Compare original player command filtering with an owned x64 policy."""
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
    image,evidence=verify_installed_ranges(SERVER,run/'command-filter-reference.local.txt',[
        ('full player command filter',0x101bed70,0x101bee4b),
        ('movement processor getter',0x103293d0,0x103293d6),
        ('movement processor initializer',0x1042da10,0x1042da30),
        ('movement processor dispatch prefix',0x101ced80,0x101cee26),
    ])
    table=0x1050eaac;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFPlayer@@'
    assert image.word(table+0x700)==0x101bed70
    moveTable=0x10510a2c;locator=image.word(moveTable-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFPlayerMove@@'
    assert image.word(moveTable+4)==0x101ced80
    assert struct.unpack_from('<f',image.b,image.offset(0x10445678))[0]==3.0
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'filter_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=512,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED PLAYER COMMAND FILTER',cases_per_run=512,repeats_per_arch=2,builds=builds,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Full installed CTFPlayer command virtual +700 executes, including command/player field writes and actual movement processor getter.',
                  'Player D1C resets. BB8 zero caches current command view angles before filtering; nonzero BB8 preserves prior cached angles.',
                  'Flag120 bit0x20 suppresses command input. Otherwise primary option/global state can bypass the startup time<3 filter.',
                  'Suppression zeros movement, buttons and impulse and restores cached angles. It still forwards the command to movement processor virtual +4.',
                  'On the unsuppressed path F4C can force button bit4, or clear itself before requesting secondary option zero.',
                  'The fixed movement object is 106d8898; initializer installs the CTFPlayerMove primary vtable10510a2c, whose virtual +4 is101ced80. This concrete implementation is inspected, not executed.'],
        limits=['Movement processor virtual +4 is supplied; no movement, collision, map contact or door activation runs.',
                'Secondary cvar setter10386700 is intercepted with exact owner/argument checks; actual cvar mutation and callbacks do not execute.',
                'Finite times0/2.999/3/4, option flags, cached/current views and adapter command mutations are covered; native NaN exception behavior is not verified.',
                'Whole unrelated player and command bytes are checked unchanged. This does not establish live SFM player fields or command sources.',
                'The owned typed command/host policy is isolated from the editor and does not reproduce native object memory layout.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=512,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
