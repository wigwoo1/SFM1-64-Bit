"""Compare actual server hull Ray_t/trace-wrapper boundary with native x64 fields."""
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
    image,evidence=verify_installed_ranges(SERVER,run/'server-hull-reference.local.txt',[
        ('server hull trace wrapper',0x10019790,0x10019835),
        ('full hull Ray_t constructor',0x10015cb0,0x10015e7c),
        ('simple trace filter construction',0x10267770,0x10267791),
    ])
    assert struct.unpack_from('<d',image.b,image.offset(0x10446288))[0]==1e-6
    builds={};reference=None
    summary=dict(cases=640,non_ray=384,stationary=320,underflow=80,passed=True)
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'hull_{arch}.exe';log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            output=run/f'{arch}-{repeat}.bin'
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-',str(output)],capture_output=True,text=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==summary
            records=output.read_bytes();assert len(records)==640*56
            if reference is None:reference=records
            assert records==reference, 'Cross-architecture hull fields differ'
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED SERVER HULL TRACE BOUNDARY',cases_per_run=640,repeats_per_arch=2,builds=builds,
        coverage=summary,semantic_record_bytes=640*56,semantic_records_byte_identical=True,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Full original hull wrapper, Ray_t constructor and filter constructor execute to supplied engine virtual+14.',
                  'Center, retained delta, negative hull midpoint offset, half extents and isRay/isSwept flags match x64 byte for byte, including signed zero and underflow cases.',
                  'Constructor classifies hulls using FLOAT squared-length arithmetic followed by a double1e-6 comparison; another trigger constructor uses different arithmetic and is not automatically interchangeable.',
                  'Engine boundary receives aligned ray with null axis-transform pointer, original mask, ignored entity, collision group and null extra filter callback.',
                  'Supplied engine output passes back unchanged with diagnostic overlay disabled. No trace collision implementation executes.'],
        limits=['Engine trace implementation, filter ShouldHitEntity bodies, entity enumeration and world collision results are unverified.',
                'Finite ordered hulls, axis-aligned rays and diagnostic toggle0 only; invalid bounds and NaN/overflow behavior are outside scope.',
                'Existing general SourcePreparedWorldRayCandidate/world trace helpers are zero-extent; translated-box hull support is restricted to a validated single-brush inline model.',
                'Neither existing path establishes general player hull tracing through world brushes/displacements and entities. Do not replace the spawn hull with a point ray.',
                'Owned ray preparation is isolated; no editor collision integration or measured benchmark placement.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=640,repeats_per_arch=2,identical_semantic_bytes=640*56,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
