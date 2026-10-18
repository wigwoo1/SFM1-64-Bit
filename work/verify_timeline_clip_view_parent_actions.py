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
    cursor, count, mismatches = start, 0, []
    rows = saved_rows(start, end)
    for row in rows:
        offset = image.offset(row["va"])
        actual = image.b[offset : offset + len(row["bytes"])]
        if row["va"] != cursor or actual != row["bytes"]:
            mismatches.append({"va": hex(row["va"]), "expected_va": hex(cursor), "match": actual == row["bytes"]})
        cursor = row["va"] + len(actual)
        count += len(actual)
    if cursor != end:
        mismatches.append({"expected_end": hex(end), "actual_end": hex(cursor)})
    return {"start": hex(start), "end_exclusive": hex(end), "instruction_rows": len(rows), "bytes": count, "all_match": not mismatches, "mismatches": mismatches}


spans = {
    "editor_creates_clip_view_with_parent": check(0x1017A4D9, 0x1017A4E2),
    "add_track_group_action_registration": check(0x10176FB6, 0x1017700E),
    "show_hide_track_groups_action_registration": check(0x101770CA, 0x1017714B),
    "add_track_group_dialog_handler": check(0x1017C560, 0x1017C7B0),
    "show_hide_action_callback": check(0x1010C7D0, 0x1010C802),
    "track_group_visibility_dialog_factory": check(0x1010C550, 0x1010C7D0),
}
previous = json.loads(Path("outputs/timeline-add-track-group-handler-recovery.json").read_text())
assert previous["binary_sha256"] == hashlib.sha256(image.b).hexdigest()
assert previous["action"] == "Add Track Group" and previous["action_callback"].lower() == "0x1017c560"
receipt = {
    "binary": BINARY,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "class": "CQTimelineClipView / parent CQTimelineEditor",
    "prior_clip_menu_receipt": "outputs/timeline-clip-view-context-menu-verification.json",
    "prior_add_track_group_receipt": "outputs/timeline-add-track-group-handler-recovery.json",
    "visibility_dialog_slot_receipt": "outputs/timeline-track-group-visibility-dialog-verification.json",
    "visibility_dialog_slot_receipt": "outputs/timeline-track-group-visibility-dialog-verification.json",
    "parent_route": {
        "clip_view_constructor_parent_field": "+0x14",
        "editor_constructor_callsite": "0x1017A4D9",
        "child_constructor": "0x101686E0",
        "menu_helper_reads_action_fields": ["parent editor +0xF8", "parent editor +0xFC"],
        "menu_helper_action_order": ["parent editor +0xF8", "parent editor +0xFC"],
    },
    "actions": [
        {
            "field": "+0xF8",
            "visible_label": image.string(0x10AC4C70),
            "label_va": "0x10AC4C70",
            "callback_va": "0x1017C560",
            "behavior": "The callback builds a Create Track Group dialog with a Track Group Name field and a default Track Group name; on acceptance it proceeds through the track-group creation/model update path.",
            "callback_receipt": "outputs/timeline-add-track-group-handler-recovery.json",
        },
        {
            "field": "+0xFC",
            "visible_label": image.string(0x10AC4C20),
            "label_va": "0x10AC4C20",
            "callback_va": "0x1010C7D0",
            "dialog_class_va": "0x10AB71B8",
            "dialog_class": image.string(0x10AB71B8),
            "dialog_title_va": "0x10AB71D4",
            "dialog_title": image.string(0x10AB71D4),
            "ok_slot_va": "0x10AB71B0",
            "ok_slot": image.string(0x10AB71B0),
            "callback_behavior": "The callback constructs a TrackGroupVisibilityDialog request through 0x1010C550, titled Track Group Visibility and wired to OnOK(), then passes it to service 0x106A0960. The service's runtime display/result behavior is not tested.",
            "handler_evidence": "The child dialog factory/callback plus Qt meta-call slot map and three slot bodies are byte-verified in the linked receipt. OnOK only caches QAbstractButton::isChecked() from a dialog-owned widget; applying visibility changes to track-group model data is not proven.",
        },
    ],
    "code_checks": spans,
    "limits": [
        "Action registration and callback bytes are verified statically; actual menu invocation and visible runtime ordering/state remain untested.",
        "The Track Group Visibility dialog request is recovered statically, but the downstream service's runtime display and resulting edits are not tested.",
        "The static OnCellChanged/OnOK slot bodies and Qt imports are identified, but signal/slot runtime behavior and model persistence remain unverified.",
        "No recreated UI or live SFM screenshot was produced.",
    ],
}
receipt["all_match"] = (
    all(item["all_match"] for item in spans.values())
    and receipt["actions"][0]["visible_label"] == "Add Track Group"
    and receipt["actions"][1]["visible_label"] == "Show/Hide Track Groups..."
    and receipt["sha256"] == "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
)
Path("outputs/timeline-clip-view-parent-actions-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"sha256": receipt["sha256"], "actions": [{"field": x["field"], "label": x["visible_label"], "callback": x["callback_va"]} for x in receipt["actions"]], "checks": {k: {x: y for x, y in v.items() if x != "mismatches"} for k, v in spans.items()}, "all_match": receipt["all_match"]}, indent=2))
