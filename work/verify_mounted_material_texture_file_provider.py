"""Read-only mounted VTF provider: disposable Windows IO + original CPU benchmark."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, re, struct, subprocess, tempfile, time
from verify_material_texture_file_resolver import sha, need, closure

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/mounted-material-texture-provider-20261028'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/material-texture-file-resolver-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == 'a57680469616575e7e40ee964239b820c2cf50c106056556ca84ccd1404b6628', 'accepted file resolver receipt')
    previous = json.loads(parent.read_text())
    allowed = ROOT / 'work/test_material_texture_file_resolver.cpp'
    original = allowed.read_bytes().decode('utf-8')
    original = original.replace('#include "mounted_material_texture_file_provider_candidate.h"\n', '')
    mounted_line = ' unsigned reads=0;std::vector<std::string>requests;auto mounted=MountedMaterialTextureFileProviderCandidate(mounts).provider();FileResolver files([mounted,&reads,&requests](const MaterialTextureFileRequestCandidate&r)->std::optional<std::vector<uint8_t>>{++reads;requests.push_back(r.file);return mounted(r);});auto resolver=files.resolver();'
    prior_line = ' unsigned reads=0;std::vector<std::string>requests;FileResolver files([&](const MaterialTextureFileRequestCandidate&r)->std::optional<std::vector<uint8_t>>{++reads;requests.push_back(r.file);for(const auto&mount:mounts){auto file=mount/r.file;if(std::filesystem::is_regular_file(file))return fileReadBytes(file);}return {};});auto resolver=files.resolver();'
    need(original.count(mounted_line) == 1, 'one exact benchmark integration replacement')
    original = original.replace(mounted_line, prior_line)
    need(hashlib.sha256(original.encode()).hexdigest() == previous['sources'][str(allowed)], 'only include and provider lambda changed in existing test')
    protected = {p: h for p, h in previous['sources'].items() if p != str(allowed)}
    protected[str(parent)] = sha(parent)
    for info in previous['executables'].values():
        protected[info['path']] = info['sha256']
    need(all(sha(p) == h for p, h in protected.items()), 'previous sources and artifacts unchanged before')
    sources = closure([ROOT / 'work/test_mounted_material_texture_file_provider.cpp', allowed, ROOT / 'work/test_material_pinned_proxy_profile.cpp', ROOT / 'work/build_mounted_material_texture_file_provider_x64.cmd', Path(__file__)])
    for name in ['material_trig_x87_candidate.inc', 'material_sine_double_x87_candidate.inc', 'material_cosine_double_x87_candidate.inc', 'material_multiply_scalar_x87_candidate.inc']:
        sources[str(ROOT / 'work' / name)] = sha(ROOT / 'work' / name)
    originals = previous['original_inputs']
    need(all(sha(p) == h for p, h in originals.items()), 'original benchmark/game inputs match accepted hashes')
    need(all(sha(p) == h for p, h in previous['editor_dependencies'].items()) and sha(previous['editor']) == previous['editor_sha256'], 'editor sources and executable unchanged before')
    editor_dependencies = closure([ROOT / 'work/probe_vulkan_win32_surface_x64.cpp'])
    need(str(ROOT / 'work/mounted_material_texture_file_provider_candidate.h') not in editor_dependencies, 'new provider not in complete editor include closure')

    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_mounted_material_texture_file_provider_x64.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=300)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh isolated mounted/resolver/profile builds')
    executables, warnings = {}, {}
    for tag in ['test_mounted_material_texture_file_provider', 'test_material_texture_file_resolver', 'test_material_pinned_proxy_profile']:
        exe = OUT / (tag + '_x64.exe')
        data = exe.read_bytes()
        pe = struct.unpack_from('<I', data, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', data, pe + 4)[0] == 0x8664 and struct.unpack_from('<H', data, pe + 24)[0] == 0x20b and b'asInvoker' in data, 'fresh AMD64 PE32+ ' + tag)
        log = (OUT / (tag + '-build.log')).read_text()
        (run / (tag + '-build.log')).write_text(log)
        warnings[tag] = re.findall(r'([^\r\n]+): warning (C\d+): ([^\r\n]+)', log)
        if tag == 'test_mounted_material_texture_file_provider':
            need(not warnings[tag], 'new provider test compiles without warnings')
        else:
            need(len(warnings[tag]) == 1 and warnings[tag][0][1] == 'C4458' and 'graph_transform_hierarchy.h(16)' in warnings[tag][0][0], 'only existing hierarchy warning ' + tag)
        imports = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        need(imports.returncode == 0, 'import inspection ' + tag)
        (run / (tag + '-imports.txt')).write_text(imports.stdout)
        names = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', imports.stdout, re.I | re.M)))
        need(names == ['KERNEL32.dll'], 'owned executable imports only system DLL ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), target='AMD64 PE32+', imports=names)

    def execute(tag, args, label):
        result = subprocess.run([executables[tag]['path'], *args], cwd=ROOT, capture_output=True, text=True, timeout=120)
        (run / (label + '.txt')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, label + ' exit code')
        return result.stdout

    fixtures = run / 'fixtures'
    mounted = execute('test_mounted_material_texture_file_provider', ['--fixtures', str(fixtures)], 'mounted-safety')
    need(re.search(r'PASS mounted_texture_checks=\d+ construction_allocation_failures=[1-9]\d* read_allocation_failures=[1-9]\d*', mounted), 'actual construction/read allocation-failure coverage')
    escape = 'PASS' if 'symlink_escape=PASS' in mounted else 'UNVERIFIED'
    junction_log = 'Not needed: file symlink escape covered.\n'
    if escape != 'PASS':
        link = fixtures / 'upper/materials/junction'
        target = fixtures / 'upper-evil/materials'
        lower_file = fixtures / 'lower/materials/junction/fixture/a.vtf'
        need(not link.exists() and target.is_dir() and fixtures.resolve().is_relative_to(OUT.resolve()), 'junction setup stays in dedicated workspace fixture')
        lower_file.parent.mkdir(parents=True)
        lower_file.write_bytes((fixtures / 'lower/materials/fixture/a.vtf').read_bytes())
        junction = subprocess.run(['cmd.exe', '/d', '/c', 'mklink', '/J', str(link), str(target)], cwd=ROOT, capture_output=True, text=True, timeout=30)
        junction_log = junction.stdout + junction.stderr
        if junction.returncode == 0:
            need(getattr(link.lstat(), 'st_reparse_tag', None) == 0xa0000003, 'actual disposable directory junction tag')
            junction_log += execute('test_mounted_material_texture_file_provider', ['--junction-fixture', str(fixtures)], 'junction-escape')
            need('PASS mounted_junction_escape=PASS lower_fallback=REFUSED' in junction_log, 'same-handle escaping directory junction refusal')
            escape = 'PASS junction (file symlink privilege unavailable)'
    (run / 'junction-setup.txt').write_text(junction_log)
    resolver = execute('test_material_texture_file_resolver', [], 'resolver-safety')
    need('PASS texture_file_resolver_checks=2469 actual_allocation_failures=7 transaction_allocation_failures=799' in resolver, 'unchanged resolver safety baseline')
    profile = execute('test_material_pinned_proxy_profile', [], 'profile-safety')
    need('PASS profile_safety_checks=997 actual_allocation_failures=479' in profile, 'unchanged pinned profile baseline')
    session = next(p for p in originals if p.endswith('mtt_soldier.dmx'))
    game = Path(session).parents[4]
    need((game / 'usermod/gameinfo.txt').is_file(), 'read-only benchmark game root')
    benchmark = execute('test_material_texture_file_resolver', ['--benchmark', session, str(game)], 'mounted-file-benchmark')
    need('PASS actual_model=13817 materials=3 profile_warnings=4 CPU_draws=1 setters=11 VTF_reads=2' in benchmark and 'texture_apply_restore=PASS rollback=PASS GPU=NOT_RUN profile=PINNED_CLIENT_IFM' in benchmark, 'real selected-model mounted-provider transaction')

    need(all(sha(p) == h for p, h in sources.items()), 'compiled sources unchanged throughout verification')
    need(all(sha(p) == h for p, h in protected.items()), 'prior sources and historical artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original inputs unchanged after read-only benchmark')
    need(all(sha(p) == h for p, h in editor_dependencies.items()) and sha(previous['editor']) == previous['editor_sha256'], 'complete editor source closure and executable unchanged after')
    need(all(sha(info['path']) == info['sha256'] for info in executables.values()), 'tested binaries unchanged')
    status = 'VERIFIED' if escape.startswith('PASS') else 'PARTIALLY VERIFIED'
    result = dict(status=status + ' mounted loose VTF provider and actual CPU model benchmark; full application INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', mounted=mounted, path_escape=escape, resolver=resolver, profile=profile, benchmark=benchmark, executables=executables, compiler_warnings=warnings, sources=sources, protected=protected, original_inputs=originals, editor=previous['editor'], editor_sha256=previous['editor_sha256'], editor_dependencies=editor_dependencies, editor_rebuilt=False, parent=str(parent), parent_sha256=sha(parent), fixture_directory=str(fixtures), limits=[
        'Opt-in absolute local-DOS loose-file GAME mounts. Normalized NT volume paths from the same root/file handles are compared; opening still uses the supplied absolute DOS mount paths. Existing directory identities/handles are frozen; absent directories require a new profile. UNC/device input paths, VPK and dynamic mounts are unsupported.',
        'Canonical path comparison deliberately requires exact handle-reported spelling and a complete directory boundary. No general filesystem security or atomic snapshot guarantee for arbitrary concurrent namespace/memory-mapped writes.',
        'Read-only file handles share reads/delete but deny write sharing during exact reads. Only absent path/file falls through; selected invalid, locked, wrong-type or oversize files remain unavailable.',
        'File-symlink creation is optional and reports the actual Windows error. The disposable junction test exercises an actual escaping reparse target and verifies no lower-priority substitution when available. No elevation requested.',
        'Truncated/empty VTF content tested; ordinary allocation and IO failure refusal paths checked. Concurrent in-flight OS short-read races were not forced.',
        'No SFM process launched/controlled/enumerated, original writes, serialization, history, proxy, cache, shader, GPU or renderer changes. Actual model uses explicit 0.125-second clock and limited pinned CLIENT+IFM profile; complete visual fidelity remains unverified.'
    ])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ['status', 'mounted', 'path_escape', 'resolver', 'profile', 'benchmark']}, indent=2), flush=True)

if __name__ == '__main__':
    main()
