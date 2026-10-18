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


def verify_code(start, end):
    cursor, byte_count, mismatches = start, 0, []
    rows = saved_rows(start, end)
    for row in rows:
        live = image.b[image.offset(row["va"]):image.offset(row["va"]) + len(row["bytes"])]
        if row["va"] != cursor or row["bytes"] != live:
            mismatches.append({"va": hex(row["va"]), "expected_va": hex(cursor), "saved": row["bytes"].hex(), "installed": live.hex()})
        cursor = row["va"] + len(row["bytes"])
        byte_count += len(live)
    if cursor != end:
        mismatches.append({"end_expected": hex(end), "end_actual": hex(cursor)})
    return {"start": hex(start), "end_exclusive": hex(end), "rows": len(rows), "bytes": byte_count, "all_match": not mismatches, "mismatches": mismatches}


meta = json.loads(Path("outputs/editor-qt-method-inventory.json").read_text())
dispatch = json.loads(Path("outputs/editor-qt-dispatch-location-inventory.json").read_text())
class_meta = next(item for item in meta["objects"] if item["class_name"] == "CQTimelineTrackView")
slot = next((item for item in class_meta["methods"] if item["signature"] == "OnCustomContextMenuRequested(QPoint)"), None)
assert slot is not None and slot["local_id"] == 0
class_dispatch = next(item for item in dispatch["classes"] if item["class_name"] == "CQTimelineTrackView")
meta_call = 0x1007C210
static_call = 0x1007C060
code = {
    "qt_metacall": verify_code(meta_call, 0x1007C256),
    "qt_static_metacall": verify_code(static_call, 0x1007C0C6),
    "metaobject_accessor": verify_code(0x1007C260, 0x1007C26F),
}
table_va = 0x1007C0C8
table_offset = image.offset(table_va)
table_bytes = image.b[table_offset:table_offset + 20]
table_targets = list(struct.unpack("<5I", table_bytes))
expected_targets = [0x1007C078, 0x1007C08E, 0x1007C09C, 0x1007C0AA, 0x1007C0B8]
receipt = {
    "binary": binary,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "class": class_meta["class_name"],
    "metaobject_va": class_meta["metaobject_va"],
    "method": slot,
    "dispatch_inventory": class_dispatch,
    "code_checks": code,
    "static_dispatch_table": {"va": hex(table_va), "bytes": table_bytes.hex(), "targets": [hex(value) for value in table_targets], "expected_targets": [hex(value) for value in expected_targets], "matches": table_targets == expected_targets},
    "verified_route": [
        "CQTimelineTrackView method inventory assigns OnCustomContextMenuRequested(QPoint) local method ID 0.",
        "The byte-verified qt_metacall range routes local IDs 0..4 to the byte-verified qt_static_metacall; its verified five-entry table selects 0x1007C078 for local ID 0.",
        "The ID-0 dispatcher loads a QPoint argument from the meta-call argument array and invokes this object's virtual slot +0xE4.",
    ],
    "limits": [
        "This proves Qt generated-slot dispatch only. The concrete runtime target behind virtual slot +0xE4 is not resolved here.",
        "No byte-proven connection from this slot to the OpenTrackContextMenu event route, menu builder, visible menu contents, or action callbacks is established.",
        "No live SFM context menu was available for comparison.",
    ],
}
receipt["all_match"] = all(item["all_match"] for item in code.values()) and receipt["static_dispatch_table"]["matches"] and receipt["sha256"] == meta["binary_sha256"]
Path("outputs/timeline-track-view-context-slot-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"sha256": receipt["sha256"], "code_checks": {k: {key: value for key, value in v.items() if key != "mismatches"} for k, v in code.items()}, "slot0_target": hex(table_targets[0]), "all_match": receipt["all_match"]}, indent=2))
