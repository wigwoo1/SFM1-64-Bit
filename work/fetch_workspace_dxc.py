"""Fetch a pinned Microsoft DXC release only into this workspace."""
import hashlib
import json
import urllib.request
import zipfile
from pathlib import Path

url = 'https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2609/dxc_2026_09_29.zip'
expected = 'ad31b1fc8443175d204f77a611fdb3ef2ec42759bdc2f1167368de24a4a7e7f1'
root = Path('work/dependencies/dxc-v1.9.2609').resolve()
root.mkdir(parents=True, exist_ok=True)
archive = root / 'release.zip'
if not archive.exists():
    request = urllib.request.Request(url, headers={'User-Agent': 'SFM64-workspace-research'})
    with urllib.request.urlopen(request, timeout=30) as response, archive.open('wb') as out:
        while chunk := response.read(1024*1024):
            out.write(chunk)
actual = hashlib.sha256(archive.read_bytes()).hexdigest()
if actual != expected:
    raise ValueError('DXC archive does not match official release SHA256')
with zipfile.ZipFile(archive) as package:
    for member in package.infolist():
        target = (root / member.filename).resolve()
        if not target.is_relative_to(root):
            raise ValueError('archive member escapes dependency directory')
    package.extractall(root)
(root/'provenance.json').write_text(json.dumps(dict(url=url, sha256=actual),indent=2))
print('Verified DXC workspace dependency:', root)
print('x64 compiler:', [str(p) for p in root.rglob('dxc.exe') if 'x64' in p.parts])
