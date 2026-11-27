"""Verify original neutral-command construction and ownership/clock ordering."""
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
    image,evidence=verify_installed_ranges(SERVER,run/'neutral-command-reference.local.txt',[
        ('neutral command generation and submission',0x101c5650,0x101c57b2),
        ('command processor pointer getter',0x101775f0,0x101775f6),
        ('last command field copy',0x101baf00,0x101baf9e),
        ('player command virtual prefix',0x101bed70,0x101bee10),
    ])
    table=0x1050eaac;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFPlayer@@'
    assert image.word(table+0x700)==0x101bed70
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'neutral_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
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
    receipt=dict(status='VERIFIED BOUNDED NEUTRAL COMMAND ROUTING',cases_per_run=72,repeats_per_arch=2,builds=builds,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Original neutral-command routine resets player input accumulator BB8, constructs a zero command and uses supplied view angles.',
                  'Tick field F6C uses truncation of single-precision serverTime / tickInterval + 0.5. Changed value invokes the supplied notification before storing.',
                  'Command processor host is set to player before supplied virtual +700 execution. Original processor getter executes.',
                  'Actual command copy stores post-execution semantic fields, preserving destination vtable/padding. Mutation of command number/buttons is covered.',
                  'Original routine restores server current/frame time after command storage and before clearing processor host. Supplied execution time mutation is covered.',
                  'Installed CTFPlayer command virtual +700 resolves 101bed70; only its prefix is inspected, not executed.'],
        limits=['View-angle getter, change notification, command processor host setter and player command virtual are supplied callbacks.',
                'Command execution, collision, touch generation, live command queues and door activation are not verified.',
                'Clock mutations are finite test values; NaN/overflow tick conversion and callbacks replacing global service pointers are outside scope.',
                'The x64 helper preserves service ordering. Its host supplies command storage and tick conversion; it is not a native player movement implementation or editor integration.',
                'The fixture treats native command padding as unspecified and does not claim layout compatibility of x86 and x64 vtable pointers.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=72,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
