import hashlib
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
image = Image(binary)

def saved_rows(start, end):
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
    return rows

def check_code_span(start, end):
    cursor, count, mismatches = start, 0, []
    rows = saved_rows(start, end)
    for row in rows:
        offset = image.offset(row["va"])
        live = image.b[offset : offset + len(row["bytes"])]
        if row["va"] != cursor or live != row["bytes"]:
            mismatches.append({"va": hex(row["va"]), "expected_address": hex(cursor), "listed": row["bytes"].hex(), "live": live.hex()})
        count += len(live)
        cursor = row["va"] + len(row["bytes"])
    if cursor != end:
        mismatches.append({"end_expected": hex(end), "end_actual": hex(cursor)})
    return {"start": hex(start), "end_exclusive": hex(end), "instruction_rows": len(rows), "bytes": count, "all_match": not mismatches, "mismatches": mismatches}

ctor_span = check_code_span(0x1024DAB7, 0x1024DABD)
dispatch_span = check_code_span(0x10996A70, 0x10996AA4)
vtable_offset = image.offset(0x10ADD44C + 0xA8)
vtable_bytes = image.b[vtable_offset : vtable_offset + 4]
vtable_slot = struct.unpack("<I", vtable_bytes)[0]
receipt = {
    "binary": binary,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "checks": {
        "CGraphEditorPanel_primary_vptr_store": {**ctor_span, "observed_vtable_va": "0x10ADD44C"},
        "CGraphEditorPanel_vtable_slot_0xA8": {"vtable_va": "0x10ADD44C", "slot_offset": "0xA8", "file_offset": hex(vtable_offset), "raw_bytes": vtable_bytes.hex(), "target_va": hex(vtable_slot), "matches_expected": vtable_slot == 0x10996A70},
        "virtual_dispatch_method": {**dispatch_span, "behavior": "loads shared service pointer 0x10F4C7E4, calls a panel virtual method, then forwards arguments/result to service vtable slot +0x5C"},
    },
}
receipt["all_match"] = all([ctor_span["all_match"], dispatch_span["all_match"], vtable_slot == 0x10996A70])
Path("outputs/graph-editor-panel-event-dispatch-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"sha256": receipt["sha256"], "all_match": receipt["all_match"], "checks": {k: {x:y for x,y in v.items() if x not in ("mismatches",)} for k,v in receipt["checks"].items()}}, indent=2))
