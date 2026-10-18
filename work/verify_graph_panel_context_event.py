import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
start, end = 0x1024C210, 0x1024C37F
rows, current, found = [], None, False
with Path("work/ifm-disassembly.txt").open(errors="replace") as source:
    for line in source:
        match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
        if match:
            address = int(match.group(1), 16)
            if address < start:
                continue
            found = True
            if current:
                rows.append(current)
            if address >= end:
                current = None
                break
            current = {"va": address, "bytes": bytes.fromhex(match.group(2))}
        elif found and current and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
            current["bytes"] += bytes.fromhex(line.strip())
if current:
    rows.append(current)

image = Image(binary)
cursor, byte_count, mismatches = start, 0, []
evidence = []
for row in rows:
    offset = image.offset(row["va"])
    live = image.b[offset : offset + len(row["bytes"])]
    match = live == row["bytes"] and row["va"] == cursor
    evidence.append({"va": hex(row["va"]), "length": len(live), "match": match})
    if row["va"] != cursor:
        mismatches.append({"kind": "gap_or_overlap", "expected": hex(cursor), "actual": hex(row["va"])})
    if live != row["bytes"]:
        mismatches.append({"kind": "bytes", "va": hex(row["va"]), "listed": row["bytes"].hex(), "live": live.hex()})
    byte_count += len(live)
    cursor = row["va"] + len(row["bytes"])
if cursor != end:
    mismatches.append({"kind": "end", "expected": hex(end), "actual": hex(cursor)})

receipt = {
    "binary": binary,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "span_start": hex(start),
    "span_end_exclusive": hex(end),
    "instruction_rows": len(rows),
    "instruction_bytes": byte_count,
    "all_match": not mismatches,
    "mismatches": mismatches,
    "row_evidence": evidence,
}
Path("outputs/graph-editor-panel-context-event-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({key: receipt[key] for key in ("sha256", "span_start", "span_end_exclusive", "instruction_rows", "instruction_bytes", "all_match", "mismatches")}, indent=2))
