"""Resolve installed ifm.dll RTTI vtables with handlers in selected high slots."""
import json
import pathlib
import struct
from pe_image import Image

DLL = pathlib.Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
SLOTS = (0x578, 0x57C)
image = Image(DLL)

# Build a reverse map for aligned file-backed words, which is how MSVC stores
# x86 RTTI pointers and vtable references in this PE.
refs = {}
for rva, size, raw in image.sections:
    stop = min(raw + size, len(image.b))
    for offset in range((raw + 3) & ~3, stop - 3, 4):
        value = struct.unpack_from("<I", image.b, offset)[0]
        refs.setdefault(value, []).append(image.base + rva + offset - raw)

def section_for(va):
    for index, (rva, size, raw) in enumerate(image.sections):
        if image.base + rva <= va < image.base + rva + size:
            return index
    return None

rows = []
seen = set()
start = 0
while True:
    pos = image.b.find(b".?AV", start)
    if pos < 0:
        break
    end = image.b.find(b"\0", pos)
    start = pos + 4
    if end < 0 or end - pos > 512:
        continue
    try:
        rva = next(rva for rva, size, raw in image.sections if raw <= pos < raw + size)
        descriptor = image.base + rva + pos - 8 - next(raw for rva2, size, raw in image.sections if rva2 == rva)
        name = image.b[pos:end].decode("ascii")
        for descriptor_ref in refs.get(descriptor, ()):
            locator = descriptor_ref - 12
            if section_for(locator) is None or image.word(locator) != 0:
                continue
            if image.word(locator + 4) > 0x10000:
                continue
            hierarchy = image.word(locator + 16)
            if section_for(hierarchy) is None:
                continue
            for locator_ref in refs.get(locator, ()):
                table = locator_ref + 4
                if section_for(table) is None:
                    continue
                for slot in SLOTS:
                    target = image.word(table + slot)
                    if section_for(target) != 0:
                        continue
                    key = (name, locator, table, slot, target)
                    if key in seen:
                        continue
                    seen.add(key)
                    rows.append({"type": name, "locator": hex(locator), "vtable": hex(table),
                                 "slot_offset": hex(slot), "target": hex(target)})
    except (ValueError, StopIteration, struct.error):
        continue

rows.sort(key=lambda row: (row["slot_offset"], row["type"], row["vtable"]))
out = pathlib.Path(__file__).resolve().parent.parent / "outputs" / "ifm-snap-vtable-slot-candidates.json"
out.write_text(json.dumps(rows, indent=2), encoding="utf-8")
print(json.dumps({"dll": str(DLL), "rows": len(rows), "candidates": rows}, indent=2))
