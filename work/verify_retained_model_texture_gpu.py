"""Retained VTF -> immutable renderer inputs -> existing Vulkan image owner."""
from pathlib import Path
from datetime import datetime, timezone
import json, re, struct, subprocess, tempfile, time
from verify_material_texture_file_resolver import sha, need, closure

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/retained-model-texture-gpu-20261028'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/mounted-material-texture-provider-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == 'f6ab6ce0d418e27d262f3878747591697ec2281cf5b2ecf1ada3e7ce36929c72', 'authenticated mounted-provider milestone')
    previous = json.loads(parent.read_text())
    protected = dict(previous['sources'])
    protected[str(parent)] = sha(parent)
    for info in previous['executables'].values():
        protected[info['path']] = info['sha256']
    need(all(sha(p) == h for p, h in protected.items()), 'all prior code and tested artifacts unchanged before')
    sources = closure([ROOT / 'work/test_retained_model_texture_inputs.cpp', ROOT / 'work/probe_retained_model_texture_gpu_x64.cpp', ROOT / 'work/test_retained_scene_material_gpu_cache.cpp', ROOT / 'work/build_retained_model_texture_gpu_x64.cmd', ROOT / 'work/verify_material_texture_file_resolver.py', Path(__file__)])
    vendor = ROOT / 'work/dependencies/vulkan-headers-6802bb4733b6/Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1/include/vulkan'
    for name in ['vulkan.h', 'vulkan_core.h', 'vk_platform.h']:
        sources[str(vendor / name)] = sha(vendor / name)
    originals = previous['original_inputs']
    need(all(sha(p) == h for p, h in originals.items()), 'accepted original inputs unchanged before')
    editor_dependencies = closure([ROOT / 'work/probe_vulkan_win32_surface_x64.cpp'])
    need(str(ROOT / 'work/retained_model_texture_inputs_candidate.h') not in editor_dependencies, 'new opt-in bridge not included by editor')
    need(all(sha(p) == h for p, h in previous['editor_dependencies'].items()) and sha(previous['editor']) == previous['editor_sha256'], 'editor unchanged before')

    started = time.time()
    build = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_retained_model_texture_gpu_x64.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=300)
    (run / 'build-driver.log').write_text(build.stdout + build.stderr)
    need(build.returncode == 0, 'fresh x64 input/cache/Vulkan probe builds')
    executables, warnings = {}, {}
    for tag in ['test_retained_model_texture_inputs', 'test_retained_scene_material_gpu_cache', 'probe_retained_model_texture_gpu_x64']:
        exe = OUT / (tag + '.exe')
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', raw, pe + 4)[0] == 0x8664 and struct.unpack_from('<H', raw, pe + 24)[0] == 0x20b and b'asInvoker' in raw, 'fresh native x64 ' + tag)
        log = (OUT / (tag + '-build.log')).read_text()
        (run / (tag + '-build.log')).write_text(log)
        warnings[tag] = re.findall(r'([^\r\n]+): warning (C\d+): ([^\r\n]+)', log)
        need(len(warnings[tag]) == 3 and all(code == 'C4996' and "'getenv'" in message and 'model_render_inputs_candidate.h(' in path for path, code, message in warnings[tag]) and {int(re.search(r'\((\d+)\)', path).group(1)) for path, _, _ in warnings[tag]} == {28, 30, 32}, 'only existing getenv warnings in unchanged renderer input header ' + tag)
        imports = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        need(imports.returncode == 0, 'imports ' + tag)
        (run / (tag + '-imports.txt')).write_text(imports.stdout)
        names = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', imports.stdout, re.I | re.M)))
        need(names == ['KERNEL32.dll'], 'only system imports ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), target='AMD64 PE32+', imports=names)

    def execute(path, args, label):
        result = subprocess.run([str(path), *args], cwd=ROOT, capture_output=True, text=True, timeout=90)
        (run / (label + '.txt')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, label + ' exit code')
        return result.stdout

    safety = execute(executables['test_retained_model_texture_inputs']['path'], [], 'input-safety')
    need('PASS retained_model_texture_checks=71 actual_allocation_failures=24' in safety, 'new texture projection/ownership/allocation coverage')
    regression = execute(executables['test_retained_scene_material_gpu_cache']['path'], [], 'cache-regression')
    need('owner_lifetime=1' in regression and 'discarded_recording_retried=1' in regression, 'unchanged retained-cache behavior')
    gpu = executables['probe_retained_model_texture_gpu_x64']['path']
    synthetic = execute(gpu, [], 'synthetic-gpu')
    need('PASS retained_texture_gpu_readbacks=3 created_profiles=2 bytes_a=16 bytes_b=16' in synthetic and 'source=SYNTHETIC_ANIMATED_VTF' in synthetic and synthetic.count('exact=1') == 3, 'real GPU synthetic frame A-B-A byte equality')
    session = next(p for p in originals if p.endswith('mtt_soldier.dmx'))
    game = Path(session).parents[4]
    actual = execute(gpu, ['--actual', str(game)], 'original-vtf-gpu')
    need('PASS retained_texture_gpu_readbacks=3 created_profiles=2' in actual and 'source=ORIGINAL_VTF_READ_ONLY' in actual and 'input_owners=PASS restore_reuse=PASS' in actual and actual.count('exact=1') == 3, 'real pilot VTF GPU upload/restore byte equality')
    # Reuse the exact authenticated prior benchmark executable; production
    # material and mounted-provider sources did not change in this milestone.
    benchmark_path = previous['executables']['test_material_texture_file_resolver']['path']
    benchmark = execute(benchmark_path, ['--benchmark', session, str(game)], 'prior-cpu-model-benchmark')
    need('materials=3 profile_warnings=4 CPU_draws=1 setters=11 VTF_reads=2' in benchmark and 'texture_apply_restore=PASS rollback=PASS' in benchmark, 'actual model CPU path regression')

    need(all(sha(p) == h for p, h in sources.items()), 'compiled sources unchanged throughout')
    need(all(sha(p) == h for p, h in protected.items()), 'all previous sources and historical artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original files unchanged after read-only runs')
    need(all(sha(p) == h for p, h in editor_dependencies.items()) and sha(previous['editor']) == previous['editor_sha256'], 'editor source closure/executable unchanged after')
    need(all(sha(info['path']) == info['sha256'] for info in executables.values()), 'tested executable identities unchanged')
    result = dict(status='VERIFIED opt-in retained texture renderer-input bridge and real Vulkan image upload/readback; full application INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', safety=safety, cache_regression=regression, synthetic_gpu=synthetic, original_vtf_gpu=actual, prior_cpu_model_benchmark=benchmark, prior_benchmark_executable=benchmark_path, prior_benchmark_sha256=sha(benchmark_path), executables=executables, compiler_warnings=warnings, sources=sources, protected=protected, original_inputs=originals, editor=previous['editor'], editor_sha256=previous['editor_sha256'], editor_dependencies=editor_dependencies, editor_rebuilt=False, parent=str(parent), parent_sha256=sha(parent), limits=[
        'New helper projects explicitly supplied renderer material-slot texture references into independently owned decoded RGBA inputs. It does not infer MDL texture-index/store-ID/per-bind mapping, material proxy clocks or other parameter writes.',
        'Requested mip clamps to the existing final available mip; animation frame is explicit and strict. Null/environment/unsupported decoder/duplicate channel bindings refuse before publication; base/second texture decode is CPU-tested.',
        'Same decoded profile identity is reused for A-B-A; existing retained GPU cache owns caller-released inputs and resources through fence completion. Each profile uploaded once. No unbounded per-frame profile cache introduced; caller must retain/reuse profiles on integration.',
        'Real Vulkan tests use the existing VulkanMaterialImagesCandidate and retained scene material cache, unmodified. Descriptor allocation/image upload/shader-read layout roundtrip and byte readback execute on the logged device. No shader sampling or model draw performed by these tests; second texture channel is CPU-only in this milestone.',
        'Actual GPU inputs are the two original hash-verified pilot VTFs from the benchmark content; CPU model benchmark is an authenticated existing executable rerun, not freshly rebuilt here. No source assets copied to public code.',
        'No UI/editor change, SFM launch/control/enumeration, original writes, document/serialization/proxy/shader change or publication. Full original-renderer equivalence, visual fidelity, scene draw binding and editor workflow integration remain unverified.',
        'The preceding mounted-provider real escaping-symlink/junction test remains unverified because Windows denied fixture reparse creation. It is not resolved by this GPU milestone.'
    ])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ['status', 'safety', 'synthetic_gpu', 'original_vtf_gpu', 'prior_cpu_model_benchmark']}, indent=2), flush=True)

if __name__ == '__main__':
    main()
