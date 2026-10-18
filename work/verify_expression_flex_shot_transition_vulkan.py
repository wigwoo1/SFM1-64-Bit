from pathlib import Path
import os, subprocess, json, hashlib, re

reference_path = Path('outputs/sfm64-flex-scene-topology-transition-gpu.json')
reference = json.loads(reference_path.read_text())
expected = next(run['frame_sha256'] for run in reference['runs'] if run['name'] == 'sync')
ticks = [870833, 1259500, 870833]
tick_path = Path('work/expression-flex-shot-transition-ticks.txt')
tick_path.write_text('\n'.join(map(str, ticks)) + '\n')
runs = []
for expressions, prefetch in [(0,0), (4,0), (0,1), (4,1)]:
    directory = Path(f'outputs/expression-flex-shot-transition-e{expressions}-p{prefetch}')
    directory.mkdir(exist_ok=True)
    env = {k:v for k,v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
    env.update(
        SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',
        SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',
        SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',
        SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(ticks[0]),
        SFM64_DIAGNOSTIC_SCENE_TICKS=str(tick_path.resolve()),
        SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',
        SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(prefetch),
        SFM64_DIAGNOSTIC_GEOMETRY_WORKERS='4',
        SFM64_DIAGNOSTIC_STATIC_FLEX='1',
        SFM64_DIAGNOSTIC_EXPRESSION_WORKERS=str(expressions),
        SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()),
    )
    run = subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'], env=env, capture_output=True, text=True, timeout=60)
    log = run.stdout + run.stderr
    (directory / 'run.txt').write_text(log)
    assert run.returncode == 0, log
    assert [int(n) for n in re.findall(r'multi_model_scene_frame=\d+ tick=\d+ models=(\d+)',log)] == [1,4,1]
    assert 'geometry_allocations=2 material_uploads=2' in log
    assert 'retained_gpu_material_cache_entries=5' in log
    assert f'preparation_tasks={2 if prefetch else 0}' in log
    assert f'preparation_worker_threads={prefetch} lifetime=retained queue_capacity=1' in log
    jobs = [int(n) for n in re.findall(r'scene_private_expression_jobs=(\d+)', log)]
    if expressions:
        assert jobs and all(n>0 for n in jobs), 'No admitted expressions on one of the sampled shots'
    hashes = []
    for n in range(3):
        pixels = (directory / f'vulkan-retained-frame-{n}.rgba').read_bytes()
        assert len(pixels) == 512*512*4
        hashes.append(hashlib.sha256(pixels).hexdigest())
    assert hashes == expected, f'Historical transition pixels differ e={expressions} p={prefetch}'
    runs.append(dict(expression_workers=expressions, prefetch=prefetch, private_chain_jobs=jobs, frame_sha256=hashes))
files = ['build/session-core/probe_vulkan_win32_surface_x64.exe','work/roundtrip-11.dmx','work/retained_session_scene_candidate.h','work/retained_shot_timeline_candidate.h','work/prepared_session_expression_chain_pass.h']
report = dict(ticks=ticks, reference=str(reference_path), models=[1,4,1], runs=runs,
    provenance={name:hashlib.sha256(Path(name).read_bytes()).hexdigest() for name in files},
    scope='GPU0 retained prototype shot transitions; static model flex, serial/four expression workers and synchronous/prefetched preparation. Exact historical pixels; native renderer, full shading/editor, fur and complete scheduler remain unfinished.')
Path('outputs/sfm64-expression-flex-shot-transition-vulkan-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('Vulkan expression flex shot transitions passed: 12 historical frames, 1/4/1 models, serial/four expression workers and synchronous/prefetched preparation')
