import hashlib
import json
import tarfile
import urllib.request
from pathlib import Path

commit = '6802bb4733b63ed5efd3adb308a6c885ef180ea1'
url = f'https://codeload.github.com/KhronosGroup/Vulkan-Headers/tar.gz/{commit}'
root = Path('work/dependencies/vulkan-headers-' + commit[:12]).resolve()
root.mkdir(parents=True, exist_ok=True)
archive = root / 'source.tar.gz'
if not archive.exists():
    with urllib.request.urlopen(url, timeout=30) as response, archive.open('wb') as out:
        while chunk := response.read(1024*1024):
            out.write(chunk)
with tarfile.open(archive) as package:
    for member in package.getmembers():
        if not (root / member.name).resolve().is_relative_to(root):
            raise ValueError('archive path escapes workspace dependency')
    package.extractall(root, filter='data')
source = root / ('Vulkan-Headers-' + commit)
(root/'provenance.json').write_text(json.dumps(dict(commit=commit,url=url,
    archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest()),indent=2))
print('Official pinned header:', source/'include/vulkan/vulkan_core.h')
