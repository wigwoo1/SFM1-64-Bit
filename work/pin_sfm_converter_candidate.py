import os,json,hashlib,uuid
from pathlib import Path
source=Path(r'C:/Users/wigwo/Documents/Codex/2026-10-04/i-w/outputs/SFMToBlender');target=Path('work/cv')/str(uuid.uuid4())[:8];target.mkdir(parents=True);files={}
for directory,dirs,names in os.walk(source):
 dirs[:]=[d for d in dirs if d not in ('__pycache__','diagnostics','tests','.git')]
 for name in names:
  if name.endswith(('.pyc','.pyo')):continue
  path=Path(directory)/name;relative=path.relative_to(source);data=path.read_bytes();digest=hashlib.sha256(data).hexdigest();destination=target/relative;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data);files[relative.as_posix()]=dict(sha256=digest,bytes=len(data))
for relative,record in files.items():
 if hashlib.sha256((source/relative).read_bytes()).hexdigest()!=record['sha256']:raise RuntimeError('Converter changed during pinning: '+relative)
 if hashlib.sha256((target/relative).read_bytes()).hexdigest()!=record['sha256']:raise RuntimeError('Pinned file mismatch: '+relative)
manifest=dict(source=str(source),snapshot=str(target.resolve()),files=files,coherent=True)
(target/'snapshot-manifest.json').write_text(json.dumps(manifest,indent=2));out=Path('outputs/cycles-worker');out.mkdir(parents=True,exist_ok=True);(out/'converter-snapshot.json').write_text(json.dumps(dict(snapshot=str(target.resolve()),files=len(files),bytes=sum(r['bytes'] for r in files.values()),manifest=str((target/'snapshot-manifest.json').resolve())),indent=2));print('converter_snapshot_verified files='+str(len(files)))

