"""Resolve imported targets behind the timeline editor's snap-command vtable slots."""
import json
from pathlib import Path
from pe_image import Image

image = Image(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
targets = {0x10A7D944, 0x10A7D948}
pe = image.u32(60)
optional = pe + 24
descriptor = image.base + image.u32(optional + 104)
found = {}
while image.word(descriptor):
    original = image.word(descriptor) or image.word(descriptor + 16)
    library = image.string(image.base + image.word(descriptor + 12))
    iat = image.word(descriptor + 16)
    index = 0
    while image.word(image.base + original + index * 4):
        thunk = image.word(image.base + original + index * 4)
        address = image.base + iat + index * 4
        if address in targets:
            name = "ordinal " + str(thunk & 0xFFFF) if thunk & 0x80000000 else image.string(image.base + thunk + 2)
            found[hex(address)] = {"library": library, "name": name}
        index += 1
    descriptor += 20
if set(int(address, 16) for address in found) != targets:
    raise RuntimeError(f"snap import targets unresolved: {found}")
result = {
    "dll": str(Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")),
    "slot_0x578_thunk": "0x1071b84c",
    "slot_0x57c_thunk": "0x1071b852",
    "imports": found,
}
out = Path("outputs/sfm-timeline-snap-import-recovery.json")
out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps(result, indent=2))
