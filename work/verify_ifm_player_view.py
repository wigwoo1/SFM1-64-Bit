"""Verify IFM's local-player view reset using its original tool dispatch."""

import argparse
from itertools import product
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import IFM, REFERENCES, sha

QUERY = struct.Struct('<3I6f')
REPLY = struct.Struct('<4I6f')


def main(build):
    assert sha(IFM) == REFERENCES[IFM]
    run = Path(tempfile.mkdtemp(prefix='ifm-view-',dir=build))
    rng = random.Random(276850)
    poses = [(0., -0., 15., -0., 90., -0.), (1., 2., 3., -4., 5., 6.)]
    poses.extend(tuple(rng.randrange(-12000,12000)/32 for _ in range(6)) for _ in range(128))
    cases = [(*flags,*pose) for flags,pose in product(product(range(2),repeat=3),poses)]
    path = run/'queries.bin'
    path.write_bytes(b''.join(QUERY.pack(*item) for item in cases))
    expected = b''.join(REPLY.pack(1,1,1,90,*item[3:8],0.) if item[1]
                       else REPLY.pack(1,1,0,0,*([0.]*6)) for item in cases)
    builds = {}
    for arch in ('x86','x64'):
        exe = build/f'ifm_view_{arch}.exe'
        log = (build/f'ifm_view_{arch}-build.log').read_text()
        assert 'warning ' not in log and 'error ' not in log
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), warnings=0)
        for repeat in (1,2):
            result = run/f'{arch}-{repeat}.bin'
            subprocess.run([exe,path,result,IFM if arch=='x86' else '-'],check=True,timeout=45)
            actual = result.read_bytes()
            assert len(actual)==len(expected), (arch,len(actual),len(expected))
            if actual != expected:
                row = next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)//REPLY.size
                raise AssertionError((arch,row,cases[row],REPLY.unpack_from(actual,row*REPLY.size),
                                      REPLY.unpack_from(expected,row*REPLY.size)))
    assert sha(IFM) == REFERENCES[IFM]
    (run/'results.json').write_text(json.dumps(dict(matched_rows=len(cases),
        resets=sum(bool(item[1]) for item in cases), reads_without_pose=sum(not item[1] for item in cases),
        failed_snap_still_sets_fov=sum(bool(item[1]) and not item[2] for item in cases),
        repeats_per_arch=2, original_ifm_unchanged=True, builds=builds),indent=2)+'\n',encoding='utf-8')
    print(run/'results.json')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build',type=Path)
    main(parser.parse_args().build.resolve())
