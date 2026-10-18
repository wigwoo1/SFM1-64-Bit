from pathlib import Path
import hashlib,json,re
from pe_image import Image
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
matches=[]; scanned=0; unreadable=[]
for path in sorted(root.rglob('*')):
    if path.suffix.lower() not in ('.dll','.exe'): continue
    try: data=path.read_bytes()
    except OSError as error:
        unreadable.append(dict(path=str(path),error=str(error))); continue
    scanned+=1
    occurrences=list(re.finditer(rb'VStudioRender026\x00',data))
    if not occurrences: continue
    image=Image(path); strings=[]
    for match in occurrences:
        offset=match.start()
        addresses=[image.base+rva+offset-raw for rva,size,raw in image.sections if raw<=offset<raw+size]
        strings.append(dict(file_offset=offset,virtual_address=hex(addresses[0]) if addresses else None))
    matches.append(dict(path=str(path),sha256=hashlib.sha256(data).hexdigest(),strings=strings))
report=dict(scanned_binaries=scanned,unreadable=unreadable,matches=matches,limitations='Read-only exact ASCII string scan. Presence alone does not establish an interface request or successful runtime use; modules may request via shared helpers or constructed names.')
Path('outputs/sfm64-studio-interface-consumer-scan.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
