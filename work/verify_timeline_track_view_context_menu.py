import hashlib
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image
from resolve_pe_iat_slot import resolve as resolve_iat

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
    return {"start": hex(start), "end_exclusive": hex(end), "instruction_rows": len(rows), "bytes": byte_count, "all_match": not mismatches, "mismatches": mismatches}


slot_receipt = json.loads(Path("outputs/timeline-track-view-context-slot-verification.json").read_text())
assert slot_receipt["all_match"] and slot_receipt["sha256"] == hashlib.sha256(image.b).hexdigest()
vtable_va = 0x10AC66B4
slot_offset = 0xE4
vtable_file_offset = image.offset(vtable_va + slot_offset)
vtable_slot_bytes = image.b[vtable_file_offset:vtable_file_offset + 4]
vtable_target = struct.unpack("<I", vtable_slot_bytes)[0]
code = {
    "constructor_vptr_and_this_setup": verify_code(0x1018C700, 0x1018C742),
    "constructor_context_policy_and_qt_connection": verify_code(0x1018C837, 0x1018C855),
    "concrete_context_menu_slot": verify_code(0x1018BFC0, 0x1018C10C),
}
symbols = {
    "QWidget::setContextMenuPolicy": resolve_iat(Path(binary), 0x10A7CDB0),
    "QObject::connect": resolve_iat(Path(binary), 0x10A7B954),
    "QMenu::QMenu(QString const&, QWidget*)": resolve_iat(Path(binary), 0x10A7C434),
    "QWidget::mapToGlobal(QPoint const&)": resolve_iat(Path(binary), 0x10A7CDB8),
}
title_va = 0x10AC3AF8
title = image.string(title_va)
exec_wrapper_receipt = json.loads(Path("outputs/timestrip-menu-provider-callchain-verification.json").read_text())
receipt = {
    "binary": binary,
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "prior_qt_slot_receipt": "outputs/timeline-track-view-context-slot-verification.json",
    "class": "CQTimelineTrackView",
    "constructor": {
        "span": code["constructor_vptr_and_this_setup"],
        "vptr_store_va": "0x1018C73C",
        "vptr_value": hex(image.word(0x1018C73E)),
        "verified_behavior": "The constructor saves ecx/this in esi and installs the CQTimelineTrackView Qt vtable/dispatch table 0x10AC66B4 in [esi].",
    },
    "signal_connection": {
        "span": code["constructor_context_policy_and_qt_connection"],
        "context_menu_policy": 3,
        "signal_signature_va": "0x10AAC39C",
        "signal_signature": image.string(0x10AAC39C),
        "slot_signature_va": "0x10AAC3CC",
        "slot_signature": image.string(0x10AAC3CC),
        "same_receiver_register": "esi",
        "qt_imports": {key: symbols[key] for key in ("QWidget::setContextMenuPolicy", "QObject::connect")},
    },
    "virtual_slot": {
        "vtable_va": hex(vtable_va),
        "slot_offset": hex(slot_offset),
        "file_offset": hex(vtable_file_offset),
        "bytes": vtable_slot_bytes.hex(),
        "target_va": hex(vtable_target),
        "matches_constructor_class": image.word(0x1018C73E) == vtable_va,
    },
    "menu_builder": {
        "span": code["concrete_context_menu_slot"],
        "qmenu_title_va": hex(title_va),
        "qmenu_title": title,
        "qt_imports": {key: symbols[key] for key in ("QMenu::QMenu(QString const&, QWidget*)", "QWidget::mapToGlobal(QPoint const&)")},
        "exec_wrapper_va": "0x106A09A0",
        "exec_wrapper_evidence": "outputs/timestrip-menu-provider-callchain-verification.json",
        "behavior": "The slot constructs a QMenu titled Track, hit-tests/branches using the QPoint and track state, maps the point to global coordinates, executes the menu through wrapper 0x106A09A0, then destroys the menu.",
    },
    "verified_route": [
        "Constructor sets context-menu policy 3 and connects customContextMenuRequested(const QPoint&) to OnCustomContextMenuRequested(const QPoint&) on the same receiver.",
        "The prior byte-verified Qt meta-call path dispatches that slot through local method ID 0 to this->vtable[+0xE4].",
        "The constructor installs vtable 0x10AC66B4, whose +0xE4 entry resolves from installed bytes to 0x1018BFC0.",
        "The complete target body constructs and executes a QMenu titled Track.",
    ],
    "limits": [
        "This establishes the Track view's context gesture and dynamic menu execution path, but not the complete menu action labels/order or the effects of the menu helper callbacks.",
        "No live SFM menu was available for visual comparison.",
    ],
}
receipt["all_match"] = (
    all(item["all_match"] for item in code.values())
    and vtable_target == 0x1018BFC0
    and receipt["virtual_slot"]["matches_constructor_class"]
    and title == "Track"
    and all("error" not in value for value in symbols.values())
    and exec_wrapper_receipt["binary_sha256"] == receipt["sha256"]
)
Path("outputs/timeline-track-view-context-menu-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"sha256": receipt["sha256"], "menu_title": title, "slot_target": hex(vtable_target), "code_checks": {key: {k: v for k, v in value.items() if k != "mismatches"} for key, value in code.items()}, "all_match": receipt["all_match"]}, indent=2))
