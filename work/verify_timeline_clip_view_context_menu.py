import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image


BINARY = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
image = Image(BINARY)
disassembly = Path("work/ifm-disassembly.txt")


def saved_rows(start, end):
    rows, current, found = [], None, False
    for line in disassembly.open(errors="replace"):
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


def check(start, end):
    cursor = start
    mismatches = []
    rows = saved_rows(start, end)
    total = 0
    for row in rows:
        offset = image.offset(row["va"])
        actual = image.b[offset : offset + len(row["bytes"])]
        if row["va"] != cursor or actual != row["bytes"]:
            mismatches.append({"va": hex(row["va"]), "expected_va": hex(cursor), "match": actual == row["bytes"]})
        cursor = row["va"] + len(actual)
        total += len(actual)
    if cursor != end:
        mismatches.append({"expected_end": hex(end), "actual_end": hex(cursor)})
    return {
        "start": hex(start),
        "end_exclusive": hex(end),
        "instruction_rows": len(rows),
        "bytes": total,
        "all_match": not mismatches,
        "mismatches": mismatches,
    }


spans = {
    "qt_meta_call": check(0x1007B480, 0x1007B4CC),
    "clip_view_constructor": check(0x101686E0, 0x101687D3),
    "context_menu_builder": check(0x10185FD0, 0x101860BC),
    "menu_action_population_helper": check(0x10168B10, 0x10168BAC),
}
vtable_va = 0x10AC2C34
slot_offset = 0xC8
target_va = image.word(vtable_va + slot_offset)
constructor_bytes = image.b[image.offset(0x10168704) : image.offset(0x1016870A)]
signal_and_slot = {
    "signal_va": "0x10AAC39C",
    "signal": image.string(0x10AAC39C),
    "slot_va": "0x10AAC3CC",
    "slot": image.string(0x10AAC3CC),
}
receipt = {
    "binary": BINARY,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "class": "CQTimelineClipView",
    "method": "OnCustomContextMenuRequested(QPoint)",
    "method_local_id": 0,
    "method_inventory": "outputs/editor-qt-method-inventory.json",
    "dispatch_inventory": "outputs/editor-qt-dispatch-location-inventory.json",
    "route": {
        "qt_metacall": "0x1007B480",
        "dispatch_virtual_offset": hex(slot_offset),
        "constructor": "0x101686E0",
        "constructor_vtable_store_va": "0x10168704",
        "constructor_vtable_store_bytes": constructor_bytes.hex(),
        "vtable_va": hex(vtable_va),
        "vtable_slot_va": hex(vtable_va + slot_offset),
        "target_va": hex(target_va),
        "context_menu_policy": 3,
        "signal_connection": signal_and_slot,
    },
    "menu_builder": {
        "title_va": "0x10AC2BFC",
        "title": image.string(0x10AC2BFC),
        "action_population_helper_va": "0x10168B10",
        "action_helper_static_caption_va": "0x10AC2D3C",
        "action_helper_static_caption": image.string(0x10AC2D3C),
        "exec_wrapper_va": "0x106A09A0",
        "behavior": "The target creates a QMenu titled Track Group Container, resolves the clicked timeline item through editor/model helper calls, invokes the shared action-population helper with the menu/view/item context, maps the click to global coordinates, executes the menu, then releases it.",
        "action_helper_behavior": "The helper adds a Timeline caption/action and attaches two QAction pointers read from offsets +0xF8 and +0xFC of its context object. Their visible labels and downstream effects are not established by this receipt.",
    },
    "code_checks": spans,
    "limits": [
        "This verifies static dispatch and menu construction against the installed image bytes; it does not prove live menu visibility or exact state-dependent action order/enabled state.",
        "The two QAction labels and their callback effects remain unresolved.",
        "No SFM runtime menu capture or recreated UI test was performed.",
    ],
}
receipt["all_match"] = (
    all(item["all_match"] for item in spans.values())
    and target_va == 0x10185FD0
    and constructor_bytes == bytes.fromhex("c706342c ac10".replace(" ", ""))
    and signal_and_slot["signal"].startswith("2customContextMenuRequested")
    and signal_and_slot["slot"].startswith("1OnCustomContextMenuRequested")
    and receipt["sha256"] == "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
)
Path("outputs/timeline-clip-view-context-menu-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"sha256": receipt["sha256"], "target": hex(target_va), "checks": {k: {x: y for x, y in v.items() if x != "mismatches"} for k, v in spans.items()}, "all_match": receipt["all_match"]}, indent=2))
