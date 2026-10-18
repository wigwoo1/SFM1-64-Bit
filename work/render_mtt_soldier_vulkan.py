from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
output = root / "outputs" / "mtt-soldier-first-shot-vulkan"
output.mkdir(parents=True, exist_ok=True)
env = {key: value for key, value in os.environ.items() if not key.startswith("SFM64_DIAGNOSTIC_")}
env.update(
    SFM64_DIAGNOSTIC_PRESENT_DEVICE="0",
    SFM64_DIAGNOSTIC_PRESENT_MODEL=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models\player\hwm\pyro\pyro",
    SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod",
    SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK="49583",
    SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx",
    SFM64_DIAGNOSTIC_CAPTURE_FRAMES="1",
    SFM64_DIAGNOSTIC_FRAME_COUNT="1",
    SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(output),
)
run = subprocess.run([str(root / "build" / "session-core" / "probe_vulkan_win32_surface_x64.exe")], cwd=root, env=env, capture_output=True, text=True, timeout=180)
log = run.stdout + run.stderr
(root / "outputs" / "sfm64-mtt-soldier-vulkan-render.txt").write_text(log, encoding="utf-8")
sys.stdout.write(log)
sys.exit(run.returncode)
