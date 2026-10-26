"""Builtin declaration and real missing-file caller; no GPU or live SFM.

The owned comparison ends at the declaration. The caller run is original-only:
UnlitGeneric's complete owned initializer/resource lifetime is still absent.
"""
from pathlib import Path
from datetime import datetime, timezone
import ast, hashlib, json, re, shutil, struct, subprocess, tempfile, time
from verify_material_numeric_value import ROOT, LIB, PRIVATE, PIN, require, sha
from verify_material_keyvalues_parser import DUMP
from verify_material_fallback_reference import decode as decode_caller
from verify_ifm_draw_packet import check_ranges

OUT = ROOT / 'outputs/builtin-error-material-seed-20261026'
NEW = ['builtin_error_material_seed_candidate.h', 'probe_builtin_error_material_seed.cpp',
       'build_builtin_error_material_seed.cmd', 'verify_builtin_error_material_seed.py']


def decode_seeds(raw):
    at = 0
    def number(fmt='<I'):
        nonlocal at
        require(at + struct.calcsize(fmt) <= len(raw), 'Seed packet extent')
        value = struct.unpack_from(fmt, raw, at)[0]; at += struct.calcsize(fmt)
        return value
    def text():
        nonlocal at
        n = number(); require(n <= 4096 and at + n <= len(raw), 'Seed text extent')
        value = raw[at:at+n].decode('ascii'); at += n
        return value
    def tree(depth=0):
        require(depth <= 16, 'Seed nesting')
        result = dict(key=text(), kind=number(), bits=number(), text=text(), pointer=number('<Q'))
        n = number(); require(n <= 256, 'Seed children')
        result['children'] = [tree(depth+1) for _ in range(n)]
        return result
    n = number(); require(n <= 16, 'Seed record count')
    result = [dict(name=text(), source=tree()) for _ in range(n)]
    require(at == len(raw), 'Complete seed framing')
    return result


def source_closure(paths):
    found = set()
    def visit(p):
        p = p.resolve()
        if p in found: return
        require(p.is_relative_to(ROOT), 'Workspace source closure')
        found.add(p)
        text = p.read_text(encoding='utf-8')
        if p.suffix in ['.h', '.cpp', '.inc']:
            for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', text, re.M):
                child = p.parent / name
                if child.is_file(): visit(child)
        if p.suffix == '.py':
            for node in ast.walk(ast.parse(text)):
                names = [node.module] if isinstance(node, ast.ImportFrom) else [n.name for n in node.names] if isinstance(node, ast.Import) else []
                for name in names:
                    child = ROOT / 'work' / ((name or '').split('.')[0] + '.py')
                    if child.is_file(): visit(child)
    for p in paths: visit(p)
    return {str(p): sha(p) for p in sorted(found)}


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Run ' + str(run), flush=True)
    previous = json.loads((ROOT/'outputs/material-directive-failures-20261026/summary.json').read_text())
    prior_path = Path(previous['receipt']); prior = json.loads(prior_path.read_text())
    require(sha(prior_path) == previous['receipt_sha256'], 'Accepted receipt identity')
    for p, h in prior['sources'].items(): require(sha(Path(p)) == h, 'Accepted source changed ' + p)
    for p, h in prior['original_hashes'].items(): require(sha(Path(p)) == h, 'Protected original ' + p)
    require(sha(LIB) == sha(PRIVATE) == PIN, 'Private original material identity')
    libs = ROOT/'outputs/material-fallback-owned-20261026/verification-0mhr77s9/copied-libraries'
    libraries = {}
    for name in ['tier0.dll', 'vstdlib.dll', 'materialsystem.dll', 'stdshader_dx9.dll', 'filesystem_stdio.dll']:
        require(sha(libs/name) == sha(LIB.parent/name), 'Copied dependency ' + name)
        libraries[str(libs/name)] = sha(libs/name)
    sources = source_closure([Path(p) for p in prior['sources']] + [ROOT/'work'/p for p in NEW])
    build_info = json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
    editor_graph = set()
    def visit_editor(p):
        if p in editor_graph: return
        editor_graph.add(p)
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', p.read_text(errors='replace'), re.M):
            child = p.parent/name
            if child.is_file(): visit_editor(child.resolve())
    visit_editor((ROOT/'work/probe_vulkan_win32_surface_x64.cpp').resolve())
    require({p.relative_to(ROOT).as_posix() for p in editor_graph} == set(build_info['editor_source_dependencies']), 'Exact editor graph')
    require(not any(ROOT/'work'/p in editor_graph for p in NEW), 'Research outside editor graph')
    image, ranges = check_ranges(LIB, 'materialsystem-disassembly.txt', [('whole_builtin_initializer', 0x1000c430, 0x1000c8f2), ('whole_fallback_caller', 0x10006f10, 0x10007355)])
    _, shader_ranges = check_ranges(LIB.parent/'stdshader_dx9.dll', 'stdshader_dx9-disassembly.txt', [('actual_InitParams_wrapper', 0x10064a30, 0x10064a57), ('UnlitGeneric_concrete_InitParams', 0x1004ca60, 0x1004cab4)])
    literal_addresses = {0x10096aac:'UnlitGeneric', 0x10096aa4:'$model', 0x10096a94:'$decalscale', 0x10096a8c:'error', 0x10096a7c:'$basetexture', 0x10096a6c:'$gammacolorread', 0x10096a5c:'$linearwrite', 0x10096a4c:'___error.vmt'}
    require(all(image.string(a) == s for a, s in literal_addresses.items()) and image.word(0x10096aa0) == 0x3d4ccccd, 'Exact original declaration literals')
    started = time.time()
    build = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT/'work/build_builtin_error_material_seed.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=120)
    (run/'build-driver.log').write_text(build.stdout+build.stderr)
    require(build.returncode == 0, 'Fresh complete bounded probe build')
    logs = {}
    for name in ['reference-build.log', 'owned-build.log', 'asm-build.log']:
        p = OUT/name
        # The target directory itself contains "builtin-error". Match actual
        # compiler/assembler diagnostic syntax, not that ordinary path token.
        require(p.stat().st_mtime >= started-2 and not re.search(r'\b(?:fatal\s+)?(?:warning|error)\s+[A-Z]+\d+\s*:|^\s*(?:warning|error)\s*:', p.read_text(), re.I|re.M), 'Fresh clean build log ' + name)
        logs[str(p)] = sha(p)
    executables = []
    for name, machine in [('reference_x86.exe', 0x14c), ('seed_owned_x64.exe', 0x8664)]:
        p = OUT/name; raw = p.read_bytes(); pe = struct.unpack_from('<I', raw, 60)[0]
        require(p.stat().st_mtime >= started-2 and struct.unpack_from('<H', raw, pe+4)[0] == machine and struct.unpack_from('<H', raw, pe+24)[0] == (0x20b if machine == 0x8664 else 0x10b) and b'asInvoker' in raw, 'Fresh architecture/manifest ' + name)
        imports = subprocess.run([str(DUMP), '/imports', str(p)], capture_output=True, text=True, check=True).stdout
        modules = re.findall(r'^\s+([\w.\-]+\.dll)\s*$', imports, re.M|re.I)
        require({m.lower() for m in modules} == {'kernel32.dll'}, 'Direct dependency boundary ' + name)
        executables.append(dict(path=str(p), sha256=sha(p), machine=hex(machine), direct_imports=modules))
    def execute(exe, args, label):
        p = subprocess.run([str(exe), *map(str, args)], cwd=run, capture_output=True, text=True, timeout=30)
        (run/(label+'.log')).write_text(p.stdout+p.stderr)
        require(p.returncode == 0, label + ' exit=' + str(p.returncode) + '; see log')
        return p.stdout
    mount = run/'mount'; mount.mkdir()
    native = execute(OUT/'reference_x86.exe', [PRIVATE, libs, mount, run/'native-seed.bin', run/'all-builtins.bin', run/'caller.bin'], 'native')
    require('UNLIT parameters=68 init_rva=64a30 concrete_init_rva=4ca60' in native, 'Actual registry initializer boundary')
    require('builtin_seed_creations=10 conversions=10 flushes=1 guard_checks=2 seed_allocations=47 caller_parameters=69' in native, 'Original complete builder/guard/caller boundary')
    counts = re.search(r'allocations=(\d+) releases=(\d+)\s*$', native)
    require(counts and counts[1] == counts[2] == '73', 'Tracked original cleanup')
    execute(OUT/'seed_owned_x64.exe', [run/'owned-seed.bin', run/'no-original-runtime.dll'], 'owned')
    require((run/'native-seed.bin').read_bytes() == (run/'owned-seed.bin').read_bytes(), 'Exact owned/native declaration bytes')
    prior_discovery = OUT/'discovery-d8ihax0x'
    require(sha(run/'native-seed.bin') == sha(prior_discovery/'native-seed.bin') and sha(run/'all-builtins.bin') == sha(prior_discovery/'all-builtins.bin'), 'Initial declaration capture unchanged')
    seeds = decode_seeds((run/'native-seed.bin').read_bytes()); all_seeds = decode_seeds((run/'all-builtins.bin').read_bytes())
    def leaf(key, kind, bits=0, text=''): return dict(key=key, kind=kind, bits=bits, text=text, pointer=0, children=[])
    expected = leaf('UnlitGeneric', 0)
    expected['children'] = [leaf('$model', 2, 1), leaf('$decalscale', 3, 0x3d4ccccd), leaf('$basetexture', 1, text='error'), leaf('$gammacolorread', 2, 1), leaf('$linearwrite', 2, 1)]
    require(seeds == [dict(name='___error.vmt', source=expected)] and len(all_seeds) == 10 and all_seeds[0] == seeds[0], 'Independent exact seed field/order/type expectations')
    caller = decode_caller((run/'caller.bin').read_bytes())
    require(len(caller) == 1, 'Original caller record count'); row = caller[0]
    require(row['shader'] == 'UnlitGeneric' and row['lookups'] == ['Wireframe', 'UnlitGeneric'] and row['selected'] == 0 and row['warnings'] == 1 and row['root'] == expected and row['aggregate'] == leaf('patchAccumulator', 0), 'Original error fallback selection/root/diagnostic')
    require(len(row['values']) == 69 and row['values'][68]['name'] == '$decalscale' and row['values'][68]['words'][3:7] == [0x3d4ccccd]*4, '68 declared parameters plus retained extra decalscale')
    require(row['values'][6]['name'] == '$basetexture' and row['values'][6]['text'] == 'error', 'Actual error texture name remains a string, not a loaded texture claim')
    safety = execute(OUT/'seed_owned_x64.exe', ['--test'], 'safety').strip()
    require(safety == 'PASS builtin_seed_checks=4 unlit_parameter_path=REFUSED', 'Owned ownership/bit/refusal checks')
    require(not any(mount.iterdir()) and not (run/'no-original-runtime.dll').exists(), 'Empty disposable mount/absent owned engine path')
    for p, h in sources.items(): require(sha(Path(p)) == h, 'Source changed during verification ' + p)
    for p, h in prior['original_hashes'].items(): require(sha(Path(p)) == h, 'Original changed ' + p)
    for p, h in libraries.items(): require(sha(Path(p)) == h, 'Private library changed ' + p)
    for p, h in build_info['editor_source_dependencies'].items(): require(sha(ROOT/p) == h, 'Editor dependency changed ' + p)
    editor = ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe'
    require(sha(editor) == build_info['executable_sha256'] == prior['editor_sha256'], 'Production editor unchanged')
    limits = ['Owned comparison is the builtin ERROR DECLARATION only (261 bytes); nine other builtin declarations are captured original evidence, not reconstructed implementations', 'Whole original builtin builder executes unchanged with controlled slot76 CreateMaterial, slot93 opaque handle conversion and slot12 final flush. Handles are not normally constructed IMaterial objects', 'One real original missing-file parameter caller consumes the published captured error source and initializes UnlitGeneric:68 declared plus one extra decalscale variable. This is original reference evidence, not owned caller equality', 'Owned UnlitGeneric initializer/mapping/resources/textures/proxies/render integration remain absent; existing owned fallback refuses the real seed transactionally', 'Previous633 fallback comparisons/14429 strict regression cases not rerun; all69 accepted source hashes preserved. No editor integration/rebuild/run, live SFM control/launch/capture, original writes, GPU or Blender']
    result = dict(status='PASS', utc=datetime.now(timezone.utc).isoformat(), scope='VERIFIED builtin error declaration; original-only error caller; application incomplete', prior_goal_turn='progress', owned_seed_cases=1, exact_seed_bytes=(run/'native-seed.bin').stat().st_size, seed_output_sha256=sha(run/'native-seed.bin'), captured_builtin_count=len(all_seeds), all_builtin_bytes=(run/'all-builtins.bin').stat().st_size, all_builtin_sha256=sha(run/'all-builtins.bin'), original_caller_cases=1, original_caller_bytes=(run/'caller.bin').stat().st_size, original_caller_sha256=sha(run/'caller.bin'), caller=caller, seeds=all_seeds, safety=safety, allocations=int(counts[1]), releases=int(counts[2]), executables=executables, build_logs=logs, sources=sources, original_hashes=prior['original_hashes'], private_library_hashes=libraries, static_ranges=ranges+shader_ranges, literal_addresses={hex(a):s for a,s in literal_addresses.items()}, prior_receipt=str(prior_path), prior_receipt_sha256=sha(prior_path), editor_dependencies_unchanged=len(editor_graph), editor_sha256=sha(editor), limits=limits)
    receipt = run/'results.json'; receipt.write_text(json.dumps(result, indent=2)+'\n')
    replay = OUT/('replay-'+run.name.removeprefix('verification-')); replay.mkdir()
    for p in [Path(p) for p in sources] + [Path(e['path']) for e in executables] + [Path(p) for p in logs] + [run/'native-seed.bin', run/'owned-seed.bin', run/'all-builtins.bin', run/'caller.bin', run/'native.log', run/'owned.log', run/'safety.log', receipt]:
        dest = replay/p.relative_to(ROOT); dest.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(p, dest)
    manifest = {p.relative_to(replay).as_posix():sha(p) for p in replay.rglob('*') if p.is_file()}
    (replay/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    pointer = dict(receipt=str(receipt), receipt_sha256=sha(receipt), replay=str(replay), replay_manifest_sha256=sha(replay/'manifest.json'))
    (OUT/'latest-verification.json').write_text(json.dumps(pointer, indent=2)+'\n')
    print(json.dumps(dict(status='PASS', **pointer, seed_bytes=result['exact_seed_bytes'], captured_builtin_count=len(all_seeds), original_caller_parameters=len(row['values']), allocations=result['allocations'], releases=result['releases'], safety=safety), indent=2))


if __name__ == '__main__': main()
