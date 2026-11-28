"""Freeze, build and run the all-flags unavailable-world dispatcher check."""
import hashlib
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ENGINE = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/engine.dll')
BSP = Path('F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/tf_movies/maps/mtt_soldier.bsp')
ENGINE_SHA = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
BSP_SHA = '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def closure():
    result = {}
    def visit(name):
        if name in result:
            return
        path = ROOT/name
        result[name] = sha(path)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', path.read_text(), re.M):
            target = (path.parent/include).resolve()
            if target.exists() and target.is_relative_to(ROOT):
                visit(target.relative_to(ROOT).as_posix())
    visit('probe_prepared_world_trace_empty_tables.cpp')
    result['build_prepared_world_trace_empty_tables.cmd'] = sha(ROOT/'build_prepared_world_trace_empty_tables.cmd')
    result['verify_prepared_world_trace_empty_tables.py'] = sha(__file__)
    return dict(sorted(result.items()))


def main(output):
    output = Path(output).resolve()
    assert output.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    output.mkdir(parents=True, exist_ok=True)
    assert sha(ENGINE) == ENGINE_SHA and sha(BSP) == BSP_SHA
    before = closure()
    build = subprocess.run(['cmd', '/c', str(ROOT/'build_prepared_world_trace_empty_tables.cmd'), str(output/'bin')],
                           capture_output=True, text=True, timeout=240)
    (output/'build.log').write_text(build.stdout+build.stderr, encoding='utf-8')
    assert build.returncode == 0, build.stdout+build.stderr
    builds = {}
    for arch, machine in (('x86', 0x14c), ('x64', 0x8664)):
        log = output/'bin'/f'empty-{arch}-build.log'
        text = log.read_text(errors='replace')
        assert 'warning C' not in text and 'error C' not in text
        exe = output/'bin'/f'empty_tables_{arch}.exe'; data = exe.read_bytes()
        pe, = struct.unpack_from('<I', data, 60)
        assert data[pe:pe+4] == b'PE\0\0'
        actual, = struct.unpack_from('<H', data, pe+4)
        optional, = struct.unpack_from('<H', data, pe+24)
        assert (actual, optional) == (machine, 0x10b if arch == 'x86' else 0x20b)
        run = subprocess.run([str(exe), str(BSP)], capture_output=True, text=True, timeout=120)
        assert run.returncode == 0, run.stdout+run.stderr
        assert json.loads(run.stdout) == dict(cases=8, unpopulated_tables=True, callbacks=0, passed=True)
        builds[arch] = dict(executable=str(exe), sha256=sha(exe), machine=hex(machine), warnings=0,
                            result=json.loads(run.stdout))
    assert closure() == before
    assert sha(ENGINE) == ENGINE_SHA and sha(BSP) == BSP_SHA
    result = dict(status='VERIFIED EMPTY-TABLE UNAVAILABLE-WORLD DISPATCH', builds=builds,
                  source_closure_sha256=before, original_engine_sha256=ENGINE_SHA,
                  original_bsp_sha256=BSP_SHA, originals_unchanged=True)
    (output/'results.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(status=result['status'], receipt=str(output/'results.json'), builds=builds)))


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory)
