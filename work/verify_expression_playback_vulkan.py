from pathlib import Path
import os, subprocess, json, hashlib, re

reference = json.loads(Path('outputs/sfm64-animated-four-model-gpu.json').read_text())
indices = [0, 30, 61]
ticks = [reference['ticks'][i] for i in indices]
tick_path = Path('work/current-playback-vulkan-ticks.txt')
tick_path.write_text('\n'.join(map(str, ticks)) + '\n')
runs = []
for workers, expression_workers in [(1,0),(4,0),(1,4),(4,4)]:
    directory = Path(f'outputs/expression-playback-vulkan-g{workers}-e{expression_workers}')
    directory.mkdir(exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
    env.update(
        SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',
        SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',
        SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',
        SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(ticks[0]),
        SFM64_DIAGNOSTIC_SCENE_TICKS=str(tick_path.resolve()),
        SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',
        SFM64_DIAGNOSTIC_GEOMETRY_WORKERS=str(workers),
        SFM64_DIAGNOSTIC_EXPRESSION_WORKERS=str(expression_workers),
        SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()),
    )
    run = subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'], env=env, capture_output=True, text=True, timeout=60)
    log = run.stdout + run.stderr
    (directory / 'run.txt').write_text(log)
    assert run.returncode == 0, log
    assert f'geometry_worker_count={workers}' in log, log
    assert f"scene_expression_workers={expression_workers}" in log
    if expression_workers:
        assert any(int(n)>0 for n in re.findall(r"scene_private_expression_jobs=(\d+)",log)), "No private expression jobs exercised"
    hashes = []
    for n, original in enumerate(indices):
        pixels = (directory / f'vulkan-retained-frame-{n}.rgba').read_bytes()
        assert len(pixels) == 512 * 512 * 4
        digest = hashlib.sha256(pixels).hexdigest()
        assert digest == reference['frame_sha256'][original], f'Historical frame mismatch workers={workers} index={original}'
        hashes.append(digest)
    assert hashes[0] == hashes[-1]
    assert hashes[0] != hashes[1]
    runs.append(dict(geometry_workers=workers, expression_workers=expression_workers, private_chain_jobs=[int(n) for n in re.findall(r"scene_private_expression_jobs=(\d+)",log)], frame_sha256=hashes))
invalid_options = []
for option, expected in [('61','combined sampling worker budget exceeds 64'), ('bad','expression worker option invalid')]:
    env['SFM64_DIAGNOSTIC_EXPRESSION_WORKERS'] = option
    rejected = subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'], env=env, capture_output=True, text=True, timeout=60)
    assert rejected.returncode != 0 and expected in rejected.stdout + rejected.stderr
    invalid_options.append(option)
files = ['build/session-core/probe_vulkan_win32_surface_x64.exe', 'work/roundtrip-11.dmx', 'work/clip_hierarchy.h', 'work/channel_binding.h', 'work/runtime_channel_play.h', 'work/prepared_session_expression_chain_pass.h']
report = dict(invalid_options_rejected=invalid_options, ticks=ticks, historical_reference='outputs/sfm64-animated-four-model-gpu.json', historical_indices=indices, runs=runs,
    provenance={name: hashlib.sha256(Path(name).read_bytes()).hexdigest() for name in files},
    scope='Rebuilt Vulkan prototype, three saved four-model frames, historical pixel equivalence and one/four geometry workers. Serial/four-worker expressions exercised; native renderer, full shading, editor and fur remain unfinished.')
Path('outputs/sfm64-expression-playback-vulkan-validation.json').write_text(json.dumps(report, indent=2) + '\n')
print('Vulkan expression playback passed: 12 historical frame comparisons, serial/four expression workers, one/four geometry workers and invalid-option rejection')
