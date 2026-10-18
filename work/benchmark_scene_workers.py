"""Capture-free diagnostic throughput; no native-renderer performance claim."""
from pathlib import Path
import argparse, hashlib, json, os, platform, re, statistics, subprocess, time
from datetime import datetime, timezone

parser=argparse.ArgumentParser()
parser.add_argument('--output-directory',default='outputs/scene-worker-benchmark')
parser.add_argument('--static-flex',action='store_true')
args=parser.parse_args()
out = Path(args.output_directory)
out.mkdir(parents=True,exist_ok=True)
executable=Path('build/session-core/probe_vulkan_win32_surface_x64.exe')
session=Path('work/roundtrip-11.dmx')
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
provenance=dict(started_utc=datetime.now(timezone.utc).isoformat(),
    executable_sha256=sha(executable),session_sha256=sha(session),
    source_sha256={str(p):sha(p) for p in (Path('work/scene_gpu_upload_candidate.h'),Path('work/prepared_session_scene_candidate.h'),Path('work/geometry_deformation_worker_pool.h'),Path('work/scene_geometry_preparation_candidate.h'),Path('work/retained_static_flex_provider_candidate.h'),Path('work/model_flex_deformation_candidate.h'))})
ticks = [1259500 + (n % 61) * 10000 // 60 for n in range(180)]
tick_path = out / 'ticks.txt'
tick_path.write_text('\n'.join(map(str, ticks)) + '\n', encoding='utf-8')
runs = []
configs = [(1, 0), (4, 0), (1, 1), (4, 1)]
for repeat in range(2):
    for workers, prefetch in (configs if repeat == 0 else list(reversed(configs))):
        env = {k: v for k, v in os.environ.items() if not k.startswith('SFM64_DIAGNOSTIC_')}
        env.update(
            SFM64_DIAGNOSTIC_PRESENT_DEVICE='0',
            SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod',
            SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK=str(ticks[0]),
            SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION='work/roundtrip-11.dmx',
            SFM64_DIAGNOSTIC_SCENE_TICKS=str(tick_path.resolve()),
            SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0',
            SFM64_DIAGNOSTIC_FRAME_PREFETCH=str(prefetch),
            SFM64_DIAGNOSTIC_GEOMETRY_WORKERS=str(workers),
            SFM64_DIAGNOSTIC_STATIC_FLEX='1' if args.static_flex else '0',
            SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out.resolve()),
        )
        start = time.perf_counter()
        result = subprocess.run(['build/session-core/probe_vulkan_win32_surface_x64.exe'],
                                env=env, capture_output=True, text=True, timeout=120)
        elapsed = time.perf_counter() - start
        log = result.stdout + result.stderr
        (out / f'w{workers}-p{prefetch}-r{repeat}.txt').write_text(log, encoding='utf-8')
        assert result.returncode == 0, log[-4000:]
        assert f'geometry_worker_count={workers}' in log
        assert 'gpu_readback_copies=0 capture_frames=0' in log, 'capture-free invariant missing'
        frames = []
        for line in log.splitlines():
            if line.startswith('frame_profile '):
                frames.append({k: float(v) for k, v in re.findall(r'(\w+)=([\d.eE+-]+)', line)})
        assert len(frames) == len(ticks), len(frames)
        steady = frames[30:]
        metrics = {key: dict(median=statistics.median(f[key] for f in steady),
                             mean=statistics.mean(f[key] for f in steady),
                             p95=sorted(f[key] for f in steady)[int(.95 * (len(steady)-1))])
                   for key in frames[0] if key != 'frame'}
        runs.append(dict(workers=workers, prefetch=prefetch, repeat=repeat,
                         process_seconds=elapsed, metrics_ms=metrics, frames=frames))
        print(f'workers={workers} prefetch={prefetch} repeat={repeat} '
              f'wall_median_ms={metrics["frame_wall_ms"]["median"]:.3f}', flush=True)

summary = []
for workers, prefetch in configs:
    matching = [r for r in runs if r['workers'] == workers and r['prefetch'] == prefetch]
    summary.append(dict(workers=workers, prefetch=prefetch,
        median_frame_wall_ms=statistics.median(r['metrics_ms']['frame_wall_ms']['median'] for r in matching),
        median_cpu_prepare_ms=statistics.median(r['metrics_ms']['cpu_prepare_ms']['median'] for r in matching)))
assert sha(executable)==provenance['executable_sha256'], 'executable changed during benchmark'
assert sha(session)==provenance['session_sha256'], 'session changed during benchmark'
assert all(sha(Path(path))==digest for path,digest in provenance['source_sha256'].items()), 'tracked source changed during benchmark'
report = dict(platform=platform.platform(), processor=platform.processor(),provenance=provenance,static_flex=args.static_flex,
    scope='GPU0 hidden Win32 512x512 four-model scene, 180 frames/run, first 30 omitted, two repeats in reversed order; capture disabled; frame wall includes presentation callback; CPU preparation reports work duration even when overlapped; no pixel validation in timed runs, no native renderer comparison',
    summary=summary, runs=runs)
(out / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(summary, indent=2))
