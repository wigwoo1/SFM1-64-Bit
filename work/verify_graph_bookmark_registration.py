import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
start, end = 0x100B3350, 0x100B3AAF
source = Path("work/ifm-disassembly.txt").open(errors="replace")
rows = []
current = None
for line in source:
    match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
    if match:
        address = int(match.group(1), 16)
        if address < start:
            continue
        if current:
            rows.append(current)
        if address >= end:
            current = None
            break
        current = {
            "va": address,
            "bytes": bytes.fromhex(match.group(2)),
            "line": line.strip(),
        }
    elif current and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
        current["bytes"] += bytes.fromhex(line.strip())
if current:
    rows.append(current)
rows = [row for row in rows if start <= row["va"] < end]

image = Image(binary)
cursor = start
mismatches = []
byte_count = 0
for row in rows:
    if row["va"] != cursor:
        mismatches.append({"kind": "gap_or_overlap", "expected": hex(cursor), "actual": hex(row["va"])})
    offset = image.offset(row["va"])
    live = image.b[offset : offset + len(row["bytes"])]
    if live != row["bytes"]:
        mismatches.append({"kind": "bytes", "va": hex(row["va"]), "listed": row["bytes"].hex(), "live": live.hex()})
    byte_count += len(live)
    cursor = row["va"] + len(row["bytes"])
if cursor != end:
    mismatches.append({"kind": "end", "expected": hex(end), "actual": hex(cursor)})

ids = {
    "ShowInElementViewer": (0x10AABFD8, "0x100B2470", 0x10AA64C4),
    "AddBookmarkAtHead": (0x10AABFC4, "0x100B1570", 0x10AABE28),
    "EditBookmarks": (0x10AABFB4, "0x100AFC60", 0x10AABFA0),
    "CutBookmark": (0x10AABF94, "0x100B2D30", 0x10AABE18),
    "CopyBookmark": (0x10AABF84, "0x100B16F0", 0x10AABE08),
    "PasteBookmark": (0x10AABF74, "0x100B2DC0", 0x10AABDC4),
    "DeleteBookmark": (0x10AABF64, "0x100B24F0", 0x10AABE5C),
    "SelectAllBookmark": (0x10AABF50, "0x100B1850", None),
    "ClearBookmark": (0x10AABF40, "0x100B19C0", 0x10AABF30),
    "JumpToPrevious": (0x10AABF20, "0x100AFB20", 0x10AABF0C),
    "JumpToNext": (0x10AABF00, "0x100AFBA0", 0x10AABEF0),
    "EditBookmarkSets": (0x10AABEDC, "0x100AFC20", 0x10AABEC4),
    "CreateNewBookmarkSetWithDialog": (0x10AABEA4, "0x100B1F30", 0x10AABE8C),
}
actions = []
for key, (key_va, callback, label_va) in ids.items():
    actions.append(
        {
            "command_key": image.string(key_va),
            "command_key_va": hex(key_va),
            "display_label": image.string(label_va) if label_va else None,
            "display_label_va": hex(label_va) if label_va else None,
            "callback_va": callback,
        }
    )

receipt = {
    "binary": binary,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "constructor_entry": hex(start),
    "constructor_end_exclusive": hex(end),
    "verified_instruction_rows": len(rows),
    "verified_bytes": byte_count,
    "all_match": not mismatches,
    "mismatches": mismatches,
    "scope": "The constructor stores the listed handler addresses in generated 24-byte callback objects, then passes each into action-registration helper 0x1069D780. Command keys, labels, and all target bytes were checked against the installed binary. This establishes one bookmark command/action collection; its exact owning C++ class and user gesture/menu surface are not established.",
    "actions": actions,
    "limits": [
        "Do not equate this registration set with the Graph Editor right-click menu without an event-to-owner connection",
        "Visibility, enablement state, runtime ordering, and action outcomes are not exercised",
        "The helper's broader menu semantics are not inferred from registration alone",
    ],
}
Path("outputs/graph-editor-bookmark-command-registration-recovery.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({key: receipt[key] for key in ("sha256", "constructor_entry", "constructor_end_exclusive", "verified_instruction_rows", "verified_bytes", "all_match", "mismatches")}, indent=2))
