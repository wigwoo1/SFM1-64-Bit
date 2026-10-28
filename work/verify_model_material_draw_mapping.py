"""Draw association without guessing native queued snapshot/value lifetime."""
from pathlib import Path
from datetime import datetime, timezone
import json, re, struct, subprocess, tempfile, time
from verify_material_texture_file_resolver import sha, need, closure
from pe_image import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/model-material-draw-mapping-20261028'
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')

def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    print('Verification directory: ' + str(run), flush=True)
    pointer = json.loads((ROOT / 'outputs/retained-model-texture-gpu-20261028/latest-verification.json').read_text())
    parent = Path(pointer['receipt'])
    need(sha(parent) == pointer['receipt_sha256'] == '651e3c7325e3b7b67b1be08c485555735a44b1128d4527355f14bc2a6924796d', 'accepted texture GPU milestone')
    previous = json.loads(parent.read_text())
    protected = {**previous['protected'], **previous['sources']}
    protected[str(parent)] = sha(parent)
    for info in previous['executables'].values():
        protected[info['path']] = info['sha256']
    need(all(sha(p) == h for p, h in protected.items()), 'all previous sources and artifacts unchanged before')
    sources = closure([ROOT / 'work/test_model_material_draw_mapping.cpp', ROOT / 'work/test_model_material_prebind_draw.cpp', ROOT / 'work/build_model_material_draw_mapping_x64.cmd', ROOT / 'work/verify_material_texture_file_resolver.py', ROOT / 'work/pe_image.py', Path(__file__)])
    for name in ['material_trig_x87_candidate.inc', 'material_sine_double_x87_candidate.inc', 'material_cosine_double_x87_candidate.inc', 'material_multiply_scalar_x87_candidate.inc']:
        sources[str(ROOT / 'work' / name)] = sha(ROOT / 'work' / name)
    originals = dict(previous['original_inputs'])
    session = next(p for p in originals if p.endswith('mtt_soldier.dmx'))
    game = Path(session).parents[4]
    originals[str(game / 'tf/materials/models/weapons/c_items/c_flamethrower_blue.vtf')] = sha(game / 'tf/materials/models/weapons/c_items/c_flamethrower_blue.vtf')
    studio = game / 'bin/studiorender.dll'
    originals[str(studio)] = sha(studio)
    need(originals[str(studio)] == 'dc7eef7620f2a687c3cdd4182f2f17f0d78890e2748abcbe923b459b5fa340f7', 'native StudioRender image matches preceding static route')
    need(all(sha(p) == h for p, h in originals.items()), 'original benchmark inputs unchanged before')
    selector_receipt = ROOT / 'outputs/studio-proxy-prebind-owned-20261027/verification-wi3kmc6t/results.json'
    need(sha(selector_receipt) == 'c20e68de47f58101d1a00bd25297d380926e46e59bf262d8f4e8d6f07d5ef623', 'accepted whole-native prebind selection receipt')
    image = Image(studio)
    def call(address, target):
        offset = image.offset(address)
        need(image.b[offset] == 0xe8 and address + 5 + struct.unpack_from('<i', image.b, offset + 1)[0] == target, 'current original relative call ' + hex(address))
        return dict(address=hex(address), target=hex(target))
    native = dict(prebind=call(0x100574aa, 0x10053cd0), enqueue=call(0x1005751a, 0x10057080), immediate=call(0x10057550, 0x100502e0))
    at = image.offset(0x10057511)
    need(image.b[at] == 0x68 and struct.unpack_from('<I', image.b, at + 1)[0] == 0x100502e0, 'actual queued draw target payload')
    native['interpretation'] = 'Static: queued branch prebind call precedes enqueue helper with draw target100502E0. This does NOT prove queued material parameter/snapshot capture timing or alias value policy.'
    (run / 'native-static-order.json').write_text(json.dumps(native, indent=2) + '\n')
    editor_dependencies = closure([ROOT / 'work/probe_vulkan_win32_surface_x64.cpp'])
    need(str(ROOT / 'work/model_material_draw_mapping_candidate.h') not in editor_dependencies and all(sha(p) == h for p, h in previous['editor_dependencies'].items()) and sha(previous['editor']) == previous['editor_sha256'], 'editor source closure and binary unchanged before')

    started = time.time()
    built = subprocess.run(['cmd.exe', '/d', '/c', str(ROOT / 'work/build_model_material_draw_mapping_x64.cmd')], cwd=ROOT, capture_output=True, text=True, timeout=300)
    (run / 'build-driver.log').write_text(built.stdout + built.stderr)
    need(built.returncode == 0, 'fresh x64 mapping and prebind regression builds')
    executables, warnings = {}, {}
    for tag in ['test_model_material_draw_mapping', 'test_model_material_prebind_draw']:
        exe = OUT / (tag + '_x64.exe')
        raw = exe.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        need(exe.stat().st_mtime >= started - 2 and struct.unpack_from('<H', raw, pe + 4)[0] == 0x8664 and struct.unpack_from('<H', raw, pe + 24)[0] == 0x20b and b'asInvoker' in raw, 'fresh x64 identity ' + tag)
        log = (OUT / (tag + '-build.log')).read_text()
        (run / (tag + '-build.log')).write_text(log)
        found = re.findall(r'([^\r\n]+): warning (C\d+): ([^\r\n]+)', log)
        warnings[tag] = found
        hierarchy = [row for row in found if row[1] == 'C4458' and 'graph_transform_hierarchy.h(16)' in row[0]]
        getenv = [row for row in found if row[1] == 'C4996' and 'model_render_inputs_candidate.h(' in row[0] and "'getenv'" in row[2]]
        need(len(hierarchy) == 1 and len(getenv) == (3 if tag == 'test_model_material_draw_mapping' else 0) and len(found) == len(hierarchy) + len(getenv), 'only known unchanged dependency warnings ' + tag)
        imports = subprocess.run([str(DUMP), '/imports', str(exe)], capture_output=True, text=True, timeout=30)
        need(imports.returncode == 0, 'import inspection ' + tag)
        (run / (tag + '-imports.txt')).write_text(imports.stdout)
        names = sorted(set(re.findall(r'^\s+([^\s]+\.dll)\s*$', imports.stdout, re.I | re.M)))
        need(names == ['KERNEL32.dll'], 'system-only test imports ' + tag)
        executables[tag] = dict(path=str(exe), sha256=sha(exe), target='AMD64 PE32+', imports=names)

    def execute(tag, args, label):
        result = subprocess.run([executables[tag]['path'], *args], cwd=ROOT, capture_output=True, text=True, timeout=120)
        (run / (label + '.txt')).write_text(result.stdout + result.stderr)
        need(result.returncode == 0, label + ' exit code')
        return result.stdout
    safety = execute('test_model_material_draw_mapping', [], 'mapping-safety')
    need('PASS model_material_mapping_checks=179 actual_allocation_failures=71' in safety and 'queued_snapshot_policy=UNVERIFIED' in safety, 'mapping validation/alias/allocation cases')
    regression = execute('test_model_material_prebind_draw', [], 'prebind-regression')
    need('PASS model_prebind_checks=3696 actual_allocation_failures=1822 post_consumer_failures=2' in regression, 'unchanged complete prebind adapter regression')
    benchmark = execute('test_model_material_draw_mapping', ['--benchmark', session, str(game)], 'actual-model-mapping')
    rows = [line.split('\t') for line in benchmark.splitlines() if line.startswith('RANGE\t')]
    need(len(rows) == 2 and rows[0][1:6] == ['0', '0', '2', '0', 'models/weapons/c_items/c_flamethrower_blue'] and rows[1][1:6] == ['1', '1', '1', '1', 'models/effects/pyro/pilotlight'] and all(int(row[6]) > 0 and int(row[6]) % 3 == 0 for row in rows), 'actual exact triangle ranges map blue texture2/pilot texture1')
    need('PASS actual_model=13817 mapped_ranges=2 resource_groups=2' in benchmark and 'topology=EXACT refusal_rollback=PASS queued_snapshot_policy=UNVERIFIED GPU=NOT_RUN' in benchmark, 'actual callback mapping and rollback')
    need(all(sha(p) == h for p, h in sources.items()) and all(sha(p) == h for p, h in protected.items()), 'new sources and all old sources/artifacts unchanged after')
    need(all(sha(p) == h for p, h in originals.items()), 'all original assets/session/native image unchanged after')
    need(all(sha(p) == h for p, h in editor_dependencies.items()) and sha(previous['editor']) == previous['editor_sha256'], 'editor unchanged after')
    need(all(sha(info['path']) == info['sha256'] for info in executables.values()), 'tested executable identities unchanged')
    result = dict(status='VERIFIED identity-only model material/range mapping and actual benchmark topology; native queued snapshot policy/application rendering INCOMPLETE', utc=datetime.now(timezone.utc).isoformat(), previous_goal_turn='progress', safety=safety, regression=regression, benchmark=benchmark, native_static_order=native, executables=executables, compiler_warnings=warnings, sources=sources, protected=protected, original_inputs=originals, editor=previous['editor'], editor_sha256=previous['editor_sha256'], editor_dependencies=editor_dependencies, editor_rebuilt=False, parent=str(parent), parent_sha256=sha(parent), native_selector_receipt=str(selector_receipt), native_selector_receipt_sha256=sha(selector_receipt), limits=[
        'Maps exact selected MDL/VTX LOD0 triangle indices, body/skin/mesh, logical renderer material keys, unique-texture prebind visit order and shared resource groups. Inputs/loader associations and the selection plan originate from explicit callers; this is not path authorization, shader or GPU admission.',
        'Distinct MDL texture indices can collapse to one renderer material slot and/or share one resource ID. All visits and ranges are retained; no first/last-wins parameter policy is selected and no scoped parameter pointers are retained.',
        'Current original binary confirms queued-branch prebind before enqueue. The native queued draw/material value capture phase and cross-model alias behavior remain unverified. Do not route per-visit snapshots to rendering based solely on this identity map.',
        'Actual benchmark uses original MDL/VVD/VTX and selected loose VMT/VTFs, no missing texture fallback, actual owned material profile/prebind callback with explicit clock, and verifies exact rollback when the renderer topology association is corrupted.',
        'Source graph, serialization, material/proxy evaluation, renderer cache, shaders, GPU code, editor and originals are unchanged. No SFM launch/control/enumeration, original write or publication. Prior GPU upload/readback evidence remains protected and was not rerun for this identity-only change.'
    ])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps({k: result[k] for k in ['status', 'safety', 'regression', 'benchmark', 'native_static_order']}, indent=2), flush=True)

if __name__ == '__main__':
    main()
