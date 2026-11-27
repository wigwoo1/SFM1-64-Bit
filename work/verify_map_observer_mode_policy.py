"""Compare original observer-mode decisions to an owned x64 service policy."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import SERVER,REFERENCES,sha
from verify_sliding_door_spawn import verify_installed_ranges


def main(directory):
    assert sha(SERVER)==REFERENCES[SERVER], 'Unrecognized installed server'
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    image,evidence=verify_installed_ranges(SERVER,run/'observer-policy-reference.local.txt',[
        ('TF observer mode setter',0x1031c950,0x1031cbb0),
        ('team field getter',0x100b6440,0x100b6447),
        ('life-state comparison',0x101bb470,0x101bb47d),
        ('restriction variable getter',0x102422c0,0x102422cf),
        ('target handle resolution',0x101dbe80,0x101dbea8),
    ])
    table=0x1050eaac;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFPlayer@@'
    assert image.word(table+0x63c)==0x1031c950
    assert [image.word(0x1031cbb0+4*i) for i in range(7)]==[
        0x1031cac6,0x1031cac6,0x1031cb4b,0x1031cac6,0x1031caf5,0x1031caf5,0x1031cb0b]
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        executable=directory/f'policy_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=executable.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(executable.resolve()),sha256=sha(executable),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(executable),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=4608,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED OBSERVER MODE POLICY',cases_per_run=4608,repeats_per_arch=2,
        builds=builds,server_unchanged=True,server_sha256=sha(SERVER),
        verified_spans=[row['name'] for row in evidence],
        findings=['Modes outside 0..6 are rejected without field changes or service calls.',
                  'Team/life-state/camera-variable constraints can change the requested mode; the target restriction can replace it again.',
                  'Previous mode greater than 1 is cached, mode timestamp is refreshed for every valid request, and network dirty bits reflect intermediate mode changes.',
                  'Resulting modes 0/1/3 request FOV reset, zero view offset and movement 0; modes 2/6 additionally apply the resolved target and request movement 10; modes 4/5 apply the target and request movement 10.',
                  'The original handles absent or serial-validated target references, then invokes the supplied settings check.'],
        limits=['FOV and movement implementations, target getter/eligibility/application, view reset and final settings check are adapters.',
                'Original entity lists, spatial relinking, network notification services and map touch generation do not run.',
                'Network proxy dirty-byte and no-edict paths are covered; external edict change bookkeeping is not executed.',
                'Target callbacks do not mutate the mode, team or target reference in this matrix.',
                'The helper is isolated from the editor. A valid policy comparison is not proof of a real live observer, movement or door collision behavior.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=4608,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
