import hashlib
import json
import re
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


def check_span(start, end):
    cursor, mismatches, byte_count = start, [], 0
    rows = saved_rows(start, end)
    for row in rows:
        live = image.b[image.offset(row["va"]):image.offset(row["va"]) + len(row["bytes"])]
        if row["va"] != cursor or live != row["bytes"]:
            mismatches.append({"va": hex(row["va"]), "expected_address": hex(cursor), "saved": row["bytes"].hex(), "installed": live.hex()})
        byte_count += len(live)
        cursor = row["va"] + len(row["bytes"])
    if cursor != end:
        mismatches.append({"end_expected": hex(end), "end_actual": hex(cursor)})
    return {"start": hex(start), "end_exclusive": hex(end), "instruction_rows": len(rows), "bytes": byte_count, "all_match": not mismatches, "mismatches": mismatches}


registration = json.loads(Path("outputs/graph-editor-bookmark-command-registration-recovery.json").read_text())
actions = {action["command_key"]: action for action in registration["actions"]}
cut_action = actions["CutBookmark"]
handler = check_span(0x100B2D30, 0x100B2DB7)
subactions = [
    {"command_key": key, "display_label": actions[key]["display_label"], "callback_va": actions[key]["callback_va"], "handler": check_span(start, end)}
    for key, start, end in [
        ("CopyBookmark", 0x100B16F0, 0x100B184D),
        ("DeleteBookmark", 0x100B24F0, 0x100B2557),
        ("ClearBookmark", 0x100B19C0, 0x100B1A58),
    ]
]
receipt = {
    "binary": binary,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "registration_receipt": "outputs/graph-editor-bookmark-command-registration-recovery.json",
    "action": cut_action,
    "handler": handler,
    "composed_registered_callbacks": subactions,
    "observed_dispatch": [
        "Branches on a virtual boolean queried from the callback object's referenced selection/provider object.",
        "Both branches call 0x100B16F0, 0x100B24F0, and 0x100B19C0 in that order.",
        "Those three targets resolve in the same registered command collection to CopyBookmark, DeleteBookmark, and ClearBookmark, respectively.",
        "Each branch then tail-dispatches through virtual slot +0x10C on the global service at 0x10E9A51C.",
    ],
    "static_behavior_interpretation": "The registered callback composition implements Cut Bookmark by dispatching Copy Bookmark, then Delete Bookmark, then Clear Bookmark. This is a static reconstruction from function calls and registration labels, not a runtime clipboard/undo test.",
    "handler_observations": {
        "CopyBookmark": "Checks the callback object's virtual availability predicate and selected-item count, iterates the selected-item pointer array at +0x0C, invokes per-item conversion/helper paths, and appends resulting entries to a local vector.",
        "DeleteBookmark": "Runs only when the callback object's selected-item count at +0x18 is positive; gathers item data through 0x100B12C0 and dispatches through 0x100A6250 and 0x1009A8F0 before local-vector cleanup.",
        "ClearBookmark": "Resets the callback object's +0x18 count when positive, passes -1 to helper 0x100AFAB0 for the field at +0x20, iterates shared-list entries at global 0x10F2BC1C through their virtual +0x10 callbacks, and finishes through owner virtual +0x14.",
    },
    "limits": [
        "The callback body and three registered target handler bodies are directly byte-verified; deeper helper effects and the global service target are not identified by this receipt.",
        "The boolean's exact meaning, clipboard format/persistence, undo behavior, and save/reopen results remain unverified.",
        "This registration is not linked to a specific visible popup or gesture.",
    ],
}
receipt["all_match"] = handler["all_match"] and all(item["handler"]["all_match"] for item in subactions) and all(item["callback_va"].lower() == item["handler"]["start"] for item in subactions) and receipt["sha256"] == registration["sha256"]
Path("outputs/graph-editor-cut-bookmark-callback-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"sha256": receipt["sha256"], "registered_callback": cut_action["callback_va"], "handler": {k: v for k, v in handler.items() if k != "mismatches"}, "composed_callbacks": [{"key": item["command_key"], "callback": item["callback_va"], "all_match": item["handler"]["all_match"], "bytes": item["handler"]["bytes"]} for item in subactions], "all_match": receipt["all_match"]}, indent=2))
