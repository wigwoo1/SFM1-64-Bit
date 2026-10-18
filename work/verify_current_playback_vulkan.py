from pathlib import Path
import os, subprocess, json, hashlib

reference = json.loads(Path('outputs/sfm64-animated-four-model-gpu.json').read_text())
indices = [0, 30, 61]
ticks = [reference['ticks'][i] for i in indices]
tick_path = Path('work/current-playback-vulkan-ticks.txt')
tick_path.write_text('\n'.join(map(str, ticks)) + '\n')
runs = []
for workers in [1, 4]:
    directory = Path(f'outputs/current-playback-vulkan-workers-{workers}')
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
        SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory.resolve()),
    )
    run = subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'], env=env, capture_output=True, text=True, timeout=60)
    log = run.stdout + run.stderr
    (directory / 'run.txt').write_text(log)
    assert run.returncode == 0, log
    assert f'geometry_worker_count={workers}' in log, log
    hashes = []
    for n, original in enumerate(indices):
        pixels = (directory / f'vulkan-retained-frame-{n}.rgba').read_bytes()
        assert len(pixels) == 512 * 512 * 4
        digest = hashlib.sha256(pixels).hexdigest()
        assert digest == reference['frame_sha256'][original], f'Historical frame mismatch workers={workers} index={original}'
        hashes.append(digest)
    assert hashes[0] == hashes[-1]
    assert hashes[0] != hashes[1]
    runs.append(dict(geometry_workers=workers, frame_sha256=hashes))
files = ['build/session-core/probe_vulkan_win32_surface_x64.exe', 'work/roundtrip-11.dmx', 'work/clip_hierarchy.h', 'work/channel_binding.h', 'work/runtime_channel_play.h', 'work/prepared_session_expression_chain_pass.h']
report = dict(ticks=ticks, historical_reference='outputs/sfm64-animated-four-model-gpu.json', historical_indices=indices, runs=runs,
    provenance={name: hashlib.sha256(Path(name).read_bytes()).hexdigest() for name in files},
    scope='Rebuilt Vulkan prototype, three saved four-model frames, historical pixel equivalence and one/four geometry workers. Default serial expressions; native renderer, full shading, editor and fur remain unfinished.')
Path('outputs/sfm64-current-playback-vulkan-validation.json').write_text(json.dumps(report, indent=2) + '\n')
print('Rebuilt Vulkan playback passed: three historical frames, backward seek, changing animation and one/four geometry workers pixel exact')
