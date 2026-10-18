"""Run inside private Blender; query the same mounted resolver as conversion."""
import json
import sys
from pathlib import Path

workspace = Path(__file__).resolve().parent.parent
pin = json.loads((workspace / 'outputs/cycles-worker/converter-snapshot.json').read_text())
sys.path.insert(0, pin['snapshot'])
from blender_worker import Assets

folder = workspace / 'outputs/cycles-conversion/override-audit'
folder.mkdir(parents=True, exist_ok=True)
assets = Assets(dict(sfm=r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game',
                     output=str(folder / 'unused.blend'), extra_assets=[]))
paths = ['materials/models/sneakyerz/wigwoo1/hobkin/' + name + '.vtf'
         for name in ('bodyhobbie2', 'clothingemi_yellow', 'eyeshobbiette2')]
paths.append('materials/models/anno/mamagen/visor.vtf')
found = {}
for path in paths:
    buffer = assets.buffer(path)
    found[path] = buffer is not None
if not found[paths[-1]]:
    raise RuntimeError('Known mounted texture did not resolve; negative results are inconclusive')
evidence = dict(paths=found, mounts=[str(mount) for mount in assets.cm.children],
    scope='Current gameinfo mounts, using the pinned converter resolver; unmounted archives and external assets are not covered.')
(folder / 'report.json').write_text(json.dumps(evidence, indent=2), encoding='utf-8')
print('override_mount_audit_complete', json.dumps(found), flush=True)
