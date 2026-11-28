"""Verify common dispatch through the four accepted original world-trace corpora."""
import ast
import hashlib
import json
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
import verify_benchmark_world_hull as benchmark_ref
import verify_sliding_door_spawn as native_ref
import verify_stationary_hull_world_trace as stationary_hull_ref
import verify_stationary_point_world_trace as stationary_point_ref
import verify_swept_hull_world_trace as swept_hull_ref
import verify_swept_point_world_trace as swept_point_ref
from pe_image import Image

ENGINE, BSP = native_ref.ENGINE, native_ref.BSP
ENGINE_SHA = 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
BSP_SHA = '60d0afcae238bc4a2da64e5153ec34ffcc4ee4a34b5553fe0e23545fd34e93ea'
PERM_SHA = '35e914e111dddaa8e76930d29c4c2f46156130b7f2d00ff7734274cb947adbe6'
ACCEPTED = [
    ('stationary_hull', 'stationary_hull_world_trace',
     Path('20261128-stationary-world-trace/verify-ltm0hdiu/results.json'), 133192,
     'd71e19661c5bd52c468822423d5228b250d65821daa2bea1c0e8812b8001947e', 'table'),
    ('stationary_point', 'stationary_point_world_trace',
     Path('20261128-stationary-point-world-trace/verify-e2ae4vc_/results.json'), 133192,
     '4d95e057f11332fb3f8bdbbbef0968b8d19cd8a569b74f22e5e396d5337e5b14', 'point'),
    ('swept_hull', 'swept_hull_world_trace',
     Path('20261127-stationary-hull-normal/swept-regression/verify-w7v2jz67/results.json'), 19264,
     '35ca5376f7362f4bfe909a7636ec250fcab31596b1dfb284080ebb5b6e6c4c5d', 'table'),
    ('swept_point', 'swept_point_world_trace',
     Path('20261128-swept-point-world-trace/verify-r1dbiebk/results.json'), 53832,
     'b3d6513006607b089b30f8e4e4fe491bbfc30fffb205ea28c7a03c3bada82f5a', 'point'),
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def closure():
    files = {}
    def visit_cpp(name):
        path = ROOT/name
        if name in files:
            return
        files[name] = sha(path)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', path.read_text(errors='strict'), re.M):
            candidate = (path.parent/include).resolve()
            if candidate.exists() and candidate.is_relative_to(ROOT):
                visit_cpp(candidate.relative_to(ROOT).as_posix())
    for name in ('source_prepared_world_trace_candidate.h',
                 'probe_dispatch_stationary_hull_world_trace.cpp',
                 'probe_dispatch_stationary_point_world_trace.cpp',
                 'probe_dispatch_swept_hull_world_trace.cpp',
                 'probe_dispatch_swept_point_world_trace.cpp',
                 'build_prepared_world_trace_dispatch.cmd'):
        visit_cpp(name)
    py_pending = ['verify_prepared_world_trace_dispatch.py']
    while py_pending:
        name = py_pending.pop()
        key = 'PY:' + name
        if key in files:
            continue
        path = ROOT/name
        files[key] = sha(path)
        tree = ast.parse(path.read_text(encoding='utf-8-sig'))
        for node in ast.walk(tree):
            modules = []
            if isinstance(node, ast.Import):
                modules.extend(alias.name for alias in node.names)
            elif isinstance(node, ast.ImportFrom) and node.module:
                modules.append(node.module)
            for module in modules:
                candidate = ROOT/(module.split('.')[0]+'.py')
                if candidate.exists():
                    py_pending.append(candidate.name)
    return dict(sorted(files.items()))


def pe_info(path, machine):
    raw = Path(path).read_bytes()
    pe = struct.unpack_from('<I', raw, 60)[0]
    assert raw[pe:pe+4] == b'PE\0\0'
    actual_machine = struct.unpack_from('<H', raw, pe+4)[0]
    optional = struct.unpack_from('<H', raw, pe+24)[0]
    assert actual_machine == machine and optional == (0x10b if machine == 0x14c else 0x20b)
    return dict(executable=str(path), sha256=sha(path), machine=hex(machine), warnings=0)


def check_records(kind, payload, bsp):
    if kind == 'stationary_hull':
        return stationary_hull_ref.inspect_records(payload, stationary_hull_ref.Benchmark(bsp), bsp)
    if kind == 'stationary_point':
        return stationary_point_ref.inspect_records(payload, stationary_point_ref.Benchmark(bsp), bsp)
    if kind == 'swept_hull':
        texinfo, length = struct.unpack_from('<2I', bsp, 8+6*16)
        texdata_bytes, = struct.unpack_from('<I', bsp, 8+2*16+4)
        flags = [0]*(texdata_bytes//32)
        for i in range(length//72):
            sf, material = struct.unpack_from('<2I', bsp, texinfo+72*i+64)
            flags[material] |= sf & 65535
        return swept_hull_ref.inspect_records(payload, flags)
    return swept_point_ref.inspect_records(payload, swept_point_ref.Benchmark(bsp))


def main(directory):
    directory = Path(directory).resolve()
    assert directory.is_relative_to(Path('D:/SFM64-development-tests').resolve())
    directory.mkdir(parents=True, exist_ok=True)
    assert sha(ENGINE) == ENGINE_SHA and sha(BSP) == BSP_SHA
    before = closure()
    run = Path(tempfile.mkdtemp(prefix='verify-', dir=directory))
    build = subprocess.run(['cmd', '/c', str(ROOT/'build_prepared_world_trace_dispatch.cmd'), str(directory/'bin')],
                           capture_output=True, text=True, timeout=600)
    (run/'build.log').write_text(build.stdout+build.stderr, encoding='utf-8')
    assert build.returncode == 0, (build.returncode, build.stdout, build.stderr)
    for path in (directory/'bin').glob('*-build.log'):
        text = path.read_text(errors='replace')
        assert 'warning C' not in text and 'error C' not in text, path.name
    image = Image(str(ENGINE))
    table = image.b[image.offset(0x104129b0):image.offset(0x104129b0)+1024]
    assert hashlib.sha256(table).hexdigest() == PERM_SHA
    fixture = run/'private-permutation.local.bin'; fixture.write_bytes(table)
    bsp = BSP.read_bytes(); rows = []
    for prefix, accepted_stem, receipt_relative, cases, expected_sha, argstyle in ACCEPTED:
        accepted_path = directory.parents[1]/receipt_relative
        accepted = json.loads(accepted_path.read_text(encoding='utf-8'))
        assert accepted['record_sha256'] == expected_sha
        assert accepted['counts']['cases'] == cases
        assert accepted['repeats_per_arch'] == 2
        exes = {}
        for arch, machine in (('x86', 0x14c), ('x64', 0x8664)):
            path = directory/'bin'/f'{prefix}_dispatch_{arch}.exe'
            log = directory/'bin'/f'{prefix}-{arch}-build.log'
            assert log.exists()
            exes[arch] = pe_info(path, machine)
        baseline = None; checks = None; run_records = []
        for arch in ('x86', 'x64'):
            for repeat in (1, 2):
                output = run/f'{prefix}-{arch}-{repeat}.records'
                args = [exes[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP),
                        str(fixture), str(output)] if argstyle == 'table' else [
                        exes[arch]['executable'], str(ENGINE) if arch == 'x86' else '-', str(BSP), str(output)]
                result = subprocess.run(args, capture_output=True, text=True, timeout=300)
                (run/f'{prefix}-{arch}-{repeat}.log').write_text(result.stdout+result.stderr, encoding='utf-8')
                assert result.returncode == 0, (prefix, arch, repeat, result.stdout, result.stderr)
                payload = output.read_bytes()
                actual = hashlib.sha256(payload).hexdigest()
                assert actual == expected_sha, (prefix, arch, repeat, actual, expected_sha)
                independent = check_records(prefix, payload, bsp)
                if baseline is None:
                    baseline, checks = payload, independent
                assert payload == baseline and independent == checks
                stdout = json.loads(result.stdout)
                assert stdout['cases'] == cases
                if prefix == 'swept_hull':
                    assert stdout['finish_cases'] == 36
                independent_checks = independent[1] if isinstance(independent, tuple) else independent
                run_records.append(dict(architecture=arch, repeat=repeat, sha256=actual,
                                        independent_checks=independent_checks))
        # Verify the trace input covers every availability/flag/position mode;
        # these are included in and protected by the immutable full-record hash.
        rows.append(dict(component=prefix, accepted_receipt=str(accepted_path), cases=cases,
                         accepted_record_sha256=expected_sha, records=run_records,
                         builds=exes, all_complete_records_match=True))
    assert closure() == before, 'source or verifier closure changed during run'
    assert sha(ENGINE) == ENGINE_SHA and sha(BSP) == BSP_SHA
    editor = json.loads((ROOT.parent/'publication/stage/docs/build-status.json').read_text())
    assert len(editor['editor_source_dependencies']) == 543
    assert all(sha(ROOT.parent/name) == value for name, value in editor['editor_source_dependencies'].items())
    result = dict(status='VERIFIED PREPARED WORLD TRACE DISPATCH', cases={r['component']:r['cases'] for r in rows},
                  components=rows, source_closure_sha256=before, original_engine_sha256=ENGINE_SHA,
                  original_bsp_sha256=BSP_SHA, private_permutation_sha256=PERM_SHA,
                  originals_unchanged=True, editor_dependencies_unchanged=543,
                  warnings=0, repeats_per_arch=2,
                  limits=['Prepared single-thread world traces only; explicit storage/material fixtures remain.',
                          'Does not verify production work/cache allocation lifetime, full world loading, entity/static-prop/filter behavior, spawn, editor integration or complete SFM compatibility.'])
    receipt = run/'results.json'; receipt.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(status=result['status'], receipt=str(receipt), cases=result['cases'])))


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    main(parser.parse_args().directory)
