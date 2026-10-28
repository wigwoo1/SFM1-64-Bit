"""Fresh owned x64 file resolver checks; original game inputs are read-only."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, re, struct, subprocess, tempfile, time

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/material-texture-file-resolver-20261028'
GAME = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
SESSION = GAME / 'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def need(condition, message):
    if not condition:
        raise RuntimeError(message)

def closure(paths):
    result = {}
    def visit(path):
        path = Path(path).resolve()
        if str(path) in result:
            return
        result[str(path)] = sha(path)
        for name in re.findall(r'^\s*#\s*include\s*"([^"\r\n]+)"', path.read_text(errors='replace'), re.M):
            child = path.parent / name
            if child.is_file():
                visit(child)
    for path in paths:
        visit(path)
    return result

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    parent_pointer = json.loads((ROOT / 'outputs/material-pinned-proxy-profile-20261027/latest-verification.json').read_text())
    parent = Path(parent_pointer['receipt'])
    need(sha(parent) == parent_pointer['receipt_sha256'] == '845fb34051bd684fb3b899df81c5f8c7a1d9e994e268b7e02bb09e208fe7c186', 'authenticated preceding profile receipt')
    previous = json.loads(parent.read_text())
    factory = ROOT / 'work/material_pinned_client_ifm_proxy_factory_candidate.h'
    need(sha(factory) == previous['source'][str(factory)], 'pinned factory profile unchanged')
    owned_pointer = json.loads((ROOT / 'outputs/material-texture-ownership-20261027/latest-verification.json').read_text())
    need(sha(owned_pointer['receipt']) == owned_pointer['receipt_sha256'] == '3f892dab99b70a7221dbc3b7bec31404a8974c16bfd7a15d38d8150b0bb84b77', 'authenticated retained texture receipt')

    sources = closure([ROOT / 'work/test_material_texture_file_resolver.cpp', ROOT / 'work/test_material_pinned_proxy_profile.cpp', ROOT / 'work/build_material_texture_file_resolver_x64.cmd', Path(__file__)])
    for name in ['material_trig_x87_candidate.inc', 'material_sine_double_x87_candidate.inc', 'material_cosine_double_x87_candidate.inc', 'material_multiply_scalar_x87_candidate.inc']:
        sources[str(ROOT / 'work' / name)] = sha(ROOT / 'work' / name)
    editor_dependencies = closure([ROOT / 'work/probe_vulkan_win32_surface_x64.cpp'])
    need(str(ROOT / 'work/material_texture_file_resolver_candidate.h') not in editor_dependencies, 'new resolver is not an editor dependency yet')
    editor = ROOT / 'outputs/sfm64-editor-x64/sfm64_session_ui.exe'
    editor_hash = sha(editor) if editor.is_file() else None

    inputs = [SESSION, GAME / 'usermod/gameinfo.txt']
    inputs += [GAME / ('usermod/models/weapons/w_models/w_flamethrower' + suffix) for suffix in ['.mdl', '.vvd', '.dx90.vtx']]
    inputs += [GAME / ('tf/materials/' + name) for name in ['models/weapons/c_items/c_flamethrower.vmt', 'models/weapons/c_items/c_flamethrower_blue.vmt', 'models/effects/pyro/pilotlight.vmt', 'models/effects/pyro/pilotlight.vtf', 'models/effects/pyro/pilotlight_motion.vtf']]
    originals = {str(path): sha(path) for path in inputs}
    need(originals[str(SESSION)] == '5a08cf8eeb086586f05dc856fb5469dd238fde90315ecf17c2dd4ab5f0742aee', 'benchmark session identity')
    need(originals[str(GAME / 'tf/materials/models/effects/pyro/pilotlight.vtf')] == '9110b924c73a5f4b0e902ede1af2f74d6c3f6c9ca98e414665e7723de22ac5d0', 'pilot VTF identity')
    need(originals[str(GAME / 'tf/materials/models/effects/pyro/pilotlight_motion.vtf')] == 'd1374117bd848a32b2afae97419e2315e2692bca51ced717c406dd693a6d9caa', 'pilot motion VTF identity')

    started = time.time()
    build = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_material_texture_file_resolver_x64.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=300)
    (run / 'build-driver.log').write_text(build.stdout + build.stderr)
    need(build.returncode == 0, 'fresh owned resolver/profile builds')
    executables, warnings = {}, {}
    for tag in ['test_material_texture_file_resolver', 'test_material_pinned_proxy_profile']:
        exe = OUT / (tag + '_x64.exe')
        data = exe.read_bytes()
        pe = struct.unpack_from('<I', data, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', data, pe + 4)[0] == 0x8664 and struct.unpack_from('<H', data, pe + 24)[0] == 0x20b and b'asInvoker' in data, 'fresh AMD64 PE32+ ' + tag)
        log = (OUT / (tag + '-build.log')).read_text()
        (run / (tag + '-build.log')).write_text(log)
        warnings[tag] = re.findall(r'([^\r\n]+): warning (C\d+): ([^\r\n]+)', log)
        need(len(warnings[tag]) == 1 and warnings[tag][0][1] == 'C4458' and 'graph_transform_hierarchy.h(16)' in warnings[tag][0][0], 'only existing hierarchy warning ' + tag)
        imports = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        need(imports.returncode == 0, 'import inspection ' + tag)
        (run / (tag + '-imports.txt')).write_text(imports.stdout)
        names = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', imports.stdout, re.I | re.M)))
        need(names == ['KERNEL32.dll'], 'only system imports in owned test ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), machine='AMD64 PE32+', imports=names)

    def execute(tag, args, name):
        result = subprocess.run([executables[tag]['path'], *args], cwd=ROOT, capture_output=True, text=True, timeout=120)
        (run / (name + '.txt')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, name + ' exit code')
        return result.stdout

    safety = execute('test_material_texture_file_resolver', [], 'resolver-safety')
    match = re.search(r'PASS texture_file_resolver_checks=(\d+) actual_allocation_failures=(\d+) transaction_allocation_failures=(\d+)', safety)
    need(match and all(int(n) > 0 for n in match.groups()), 'resolver and cold-cache transaction allocation coverage')
    baseline = execute('test_material_pinned_proxy_profile', [], 'profile-safety')
    need('PASS profile_safety_checks=997 actual_allocation_failures=479' in baseline, 'unchanged profile baseline')
    args = ['--benchmark', str(SESSION), str(GAME)]
    profile_benchmark = execute('test_material_pinned_proxy_profile', args, 'original-profile-benchmark')
    need('PASS actual_model=13817 CPU_draws=1 setters=11 rollback=PASS clock=EXPLICIT GPU=NOT_RUN' in profile_benchmark, 'full existing pinned-profile CPU benchmark')
    benchmark = execute('test_material_texture_file_resolver', args, 'file-texture-benchmark')
    need('PASS actual_model=13817 materials=3 profile_warnings=4 CPU_draws=1 setters=11 VTF_reads=2' in benchmark and 'texture_apply_restore=PASS rollback=PASS GPU=NOT_RUN profile=PINNED_CLIENT_IFM' in benchmark, 'real selected-model VTF apply and restore')
    need(all(sha(path) == value for path, value in sources.items()), 'all compiled source inputs unchanged during verification')
    need(all(sha(path) == value for path, value in originals.items()), 'all original inputs unchanged')
    need(all(sha(path) == value for path, value in editor_dependencies.items()) and (sha(editor) if editor.is_file() else None) == editor_hash, 'editor sources and executable unchanged')
    need(all(sha(info['path']) == info['sha256'] for info in executables.values()), 'tested executables unchanged')
    result = dict(status='VERIFIED bounded file resolver and actual benchmark CPU texture transaction; full application INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', safety=safety, baseline=baseline, profile_benchmark=profile_benchmark, benchmark=benchmark, executables=executables, compiler_warnings=warnings, sources=sources, original_inputs=originals, editor=str(editor), editor_sha256=editor_hash, editor_dependencies=editor_dependencies, editor_rebuilt=False, parent=str(parent), parent_sha256=sha(parent), limits=[
        'Opt-in owner-thread CPU service with injected read-only file provider. No default native texture-manager replacement, GPU resource or renderer admission.',
        'File admission is an explicitly bounded safe relative loose-file subset, not proof of original LoadVTF path behavior. GAME mounts remain supplied by caller; VPK, dynamic mounts, native error textures and render targets are unavailable.',
        'Cache keeps successful resources for the service lifetime; missing/invalid reads are retryable. A failed material transaction may retain successful cache reads but preserves exact original parameter state.',
        'Real model used explicit 0.125-second proxy clock, pinned CLIENT+IFM factory profile, actual loose VMT/MDL/VVD/VTX inputs and two hash-verified pilot VTFs. Four registration-miss warnings are expected for the two weapon materials under that profile; dynamic provider completeness remains unverified.',
        'Earlier base-factory model benchmark refusal did not describe the newer pinned profile. The unchanged newer profile was freshly rebuilt and passed the full CPU draw here.',
        'No SFM launch/control, original writes, session serialization, UI, renderer, shader changes or publication. Full visual/workflow fidelity remains unverified.'
    ])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps({key: result[key] for key in ['status', 'safety', 'baseline', 'profile_benchmark', 'benchmark']}, indent=2), flush=True)

if __name__ == '__main__':
    main()
