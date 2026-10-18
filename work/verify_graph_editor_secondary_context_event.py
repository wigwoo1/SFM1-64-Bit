import hashlib
import json
import re
from pathlib import Path

from pe_image import Image

binary = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
spans = [
    (0x102FB6B0, 0x102FB774),  # event gates and OpenContextMenu relay
    (0x102F60E8, 0x102F60ED),  # child-view caller
    (0x102FBCD8, 0x102FBCDD),  # second caller
    (0x102FC28C, 0x102FC291),  # third caller
]
listing = Path("work/ifm-disassembly.txt")
rows_by_va = {}
current = None
with listing.open(errors="replace") as source:
    for line in source:
        match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
        if match:
            current = {"va": int(match.group(1), 16), "bytes": bytearray.fromhex(match.group(2))}
            rows_by_va[current["va"]] = current
        elif current and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
            current["bytes"].extend(bytes.fromhex(line.strip()))

image = Image(binary)
checks = []
mismatches = []
for start, end in spans:
    cursor = start
    expected = bytearray()
    for va in sorted(k for k in rows_by_va if start <= k < end):
        row = rows_by_va[va]
        if va != cursor:
            mismatches.append({"kind": "gap_or_overlap", "start": hex(start), "expected": hex(cursor), "actual": hex(va)})
        expected.extend(row["bytes"])
        cursor = va + len(row["bytes"])
    if cursor != end:
        mismatches.append({"kind": "end", "start": hex(start), "expected": hex(end), "actual": hex(cursor)})
    offset = image.offset(start)
    live = image.b[offset:offset + len(expected)]
    if live != expected:
        mismatches.append({"kind": "bytes", "start": hex(start), "listed": expected.hex(), "live": live.hex()})
    checks.append({"start": hex(start), "end_exclusive": hex(end), "bytes": len(expected), "matches": live == expected})

receipt = {
    "binary": binary,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "handler": {"start": "0x102fb6b0", "end_exclusive": "0x102fb774"},
    "callers": ["0x102f60e8", "0x102fbcd8", "0x102fc28c"],
    "checks": checks,
    "all_match": not mismatches,
    "mismatches": mismatches,
    "behavior": "For event code 0x6C with both state gates clear (mode/predicate IDs 0x51 and 0x52), the handler resolves object ID 0x24 and key OpenContextMenu, then forwards it to service 0x10F4C7E4 virtual slot +0x5C. Three separate callers route into this handler; their UI class names and downstream visible menu contents are not established.",
}
Path("outputs/graph-editor-secondary-context-event-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({key: receipt[key] for key in ("sha256", "checks", "all_match", "mismatches")}, indent=2))
