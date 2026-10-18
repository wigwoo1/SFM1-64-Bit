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
binary_hash = hashlib.sha256(image.b).hexdigest()


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
    cursor, count, mismatches = start, 0, []
    for row in rows:
        live = image.b[image.offset(row["va"]):image.offset(row["va"]) + len(row["bytes"])]
        if row["va"] != cursor or live != row["bytes"]:
            mismatches.append({"va": hex(row["va"]), "expected_va": hex(cursor), "saved": row["bytes"].hex(), "installed": live.hex()})
        cursor = row["va"] + len(row["bytes"])
        count += len(live)
    if cursor != end:
        mismatches.append({"end_expected": hex(end), "end_actual": hex(cursor)})
    return {"start": hex(start), "end_exclusive": hex(end), "instruction_rows": len(rows), "bytes": count, "all_match": not mismatches, "mismatches": mismatches}


def installed_span_has(start, end, operand):
    span = image.b[image.offset(start):image.offset(end)]
    return struct.pack("<I", operand) in span


slot_receipt = json.loads(Path("outputs/timeline-track-view-context-menu-verification.json").read_text())
assert slot_receipt["all_match"] and slot_receipt["sha256"] == binary_hash
compact_receipt = json.loads(Path("outputs/timeline-compact-track-action-recovery.json").read_text())
assert compact_receipt["binary_sha256"] == binary_hash
body = saved_rows(0x101733A0, 0x10173A44)
utilities_compact_track = saved_rows(0x101735FD, 0x10173624)
add_track_items = saved_rows(0x101736CF, 0x10173765)
shared_add_track_handler = saved_rows(0x1017DBB0, 0x1017DDB9)
track_action_registrations = {
    "sound": saved_rows(0x1017744C, 0x10177491),
    "effects": saved_rows(0x10177496, 0x101774DB),
    "channel": saved_rows(0x101774E0, 0x10177525),
}
track_kind_specific_actions = saved_rows(0x10173765, 0x101738BA)
cleanup_action_registration = saved_rows(0x10179C86, 0x10179CB8)
cleanup_action_attachment = saved_rows(0x101739FC, 0x10173A1A)
remove_muted_sounds_handler = saved_rows(0x1017DDC0, 0x1017DE86)
cleanup_group_action_registration = saved_rows(0x10179C3F, 0x10179C86)
cleanup_group_action_attachment = saved_rows(0x10173279, 0x101732A6)
cleanup_group_handler = saved_rows(0x1017ED30, 0x1017EE10)
cleanup_index_remove_helper = saved_rows(0x104D2CB0, 0x104D2CD5)
cleanup_group_remove_thunk = saved_rows(0x104BB640, 0x104BB64C)
cleanup_collection_erase_adapter = saved_rows(0x103885E0, 0x1038860B)
cleanup_callback_binding_single = saved_rows(0x1016CCC0, 0x1016CD3E)
cleanup_callback_binding_multi = saved_rows(0x1016CE80, 0x1016CF10)
track_kind_action_registrations = {
    "add_clip_to_track": saved_rows(0x10177A88, 0x10177ACD),
    "set_volume": saved_rows(0x10177AD2, 0x10177B17),
    "record_narration": saved_rows(0x10177B1C, 0x10177B63),
    "add_command_trigger": saved_rows(0x10177BB0, 0x10177BF5),
}
add_command_trigger_handler = saved_rows(0x1017D3F0, 0x1017D505)
table_va = 0x10173A44
table = struct.unpack("<4I", image.b[image.offset(table_va):image.offset(table_va) + 16])
labels = {
    "root_default": (0x10AC3AF8, "Track"),
    "root_film": (0x10AC3B00, "Film Track"),
    "root_effects": (0x10AC3B0C, "Effects Track"),
    "root_sound": (0x10AC3B1C, "Sound Track"),
    "root_channel": (0x10AC3B28, "Channel Track"),
    "utilities_submenu": (0x10AA69E4, "Utilities"),
    "add_track_submenu": (0x10AC3ACC, "Add Track"),
    "cleanup_submenu": (0x10AC3AC4, "Cleanup"),
    "sound_track_action": (0x10AB0794, "Sound"),
    "sound_track_command": (0x10AC4ABC, "InsertSoundTrackIntoTrackGroup"),
    "effects_track_action": (0x10AB06F4, "Effects"),
    "effects_track_command": (0x10AC4A98, "InsertEffectsTrackIntoTrackGroup"),
    "channel_track_action": (0x10AC4A6C, "Channel"),
    "channel_track_command": (0x10AC4A74, "InsertChannelTrackIntoTrackGroup"),
    "add_clip_to_track_action": (0x10AC484C, "Add Clip to Track"),
    "add_clip_to_track_command": (0x10AC4860, "AddClipToTrack"),
    "set_volume_action": (0x10AC4834, "Set Volume"),
    "set_volume_command": (0x10AC4840, "SetVolume"),
    "record_narration_action": (0x10AC4810, "Record Narration"),
    "record_narration_command": (0x10AC4824, "RecordNarration"),
    "add_command_trigger_action": (0x10AC47BC, "Add Command Trigger"),
    "add_command_trigger_command": (0x10AC47D0, "AddCommandTrigger"),
    "edit_command_dialog": (0x10AC55F8, "Edit Command"),
    "edit_command_prompt": (0x10AC5608, "Enter command"),
    "remove_muted_sounds_action_label": (0x10AC3CA0, "Remove Muted Sounds"),
    "remove_muted_sounds_command_key": (0x10AC3CB4, "RemoveMutedSounds"),
}
label_checks = {name: {"va": hex(va), "expected": expected, "actual": image.string(va), "matches": image.string(va) == expected} for name, (va, expected) in labels.items()}
imports = {
    "QString::fromAscii_helper": resolve_iat(Path(binary), 0x10A7B8FC),
    "QString::operator=(char const*)": resolve_iat(Path(binary), 0x10A7B4B4),
    "QMenu::addMenu(QString const&)": resolve_iat(Path(binary), 0x10A7CA84),
    "QMenu::addSeparator()": resolve_iat(Path(binary), 0x10A7C430),
    "QWidget::addAction(QAction*)": resolve_iat(Path(binary), 0x10A7BC24),
    "QString::free": resolve_iat(Path(binary), 0x10A7B8F4),
}
expected_roots = [0x10173425, 0x1017342C, 0x10173433, 0x1017343A]
receipt = {
    "binary": binary,
    "sha256": binary_hash,
    "parent_route_evidence": "outputs/timeline-track-view-context-menu-verification.json",
    "function": "CQTimelineTrackView track-context menu population helper",
    "function_va": "0x101733A0",
    "direct_byte_verification": body,
    "track_kind_title_dispatch": {
        "table_va": hex(table_va),
        "targets": [hex(item) for item in table],
        "expected_case_targets": [hex(item) for item in expected_roots],
        "labels": {name: label_checks[name] for name in ("root_film", "root_effects", "root_sound", "root_channel")},
        "default_title": label_checks["root_default"],
    },
    "menu_structure_evidence": {
        "submenus": {name: label_checks[name] for name in ("utilities_submenu", "add_track_submenu", "cleanup_submenu")},
        "qt_imports": imports,
        "population_callbacks": ["0x1016EB00", "0x1016EBA0", "0x1016CD40", "0x1016D0C0"],
        "separators_added": True,
    },
    "utilities_compact_track_action": {
        "byte_verification": utilities_compact_track,
        "action_member": compact_receipt["action_member"],
        "action_label": compact_receipt["action_label"],
        "callback_va": compact_receipt["action_callback"],
        "existing_action_evidence": "outputs/timeline-compact-track-action-recovery.json",
        "menu_attach_import": imports["QWidget::addAction(QAction*)"],
        "behavior": "Inside the Utilities submenu population block, the code binds CQTimelineEditor +0x124 to callback 0x1017C8C0, then adds that QAction member to the submenu. The existing action recovery identifies it as Compact Track.",
    },
    "add_track_submenu_actions": {
        "attachment_span": add_track_items,
        "shared_callback_va": "0x1017DBB0",
        "shared_callback_span": shared_add_track_handler,
        "member_order_and_type_values": [
            {"member": "CQTimelineEditor +0x140", "label": label_checks["channel_track_action"]["actual"], "command_key": label_checks["channel_track_command"]["actual"], "mode": 0},
            {"member": "CQTimelineEditor +0x13C", "label": label_checks["effects_track_action"]["actual"], "command_key": label_checks["effects_track_command"]["actual"], "mode": 2},
            {"member": "CQTimelineEditor +0x138", "label": label_checks["sound_track_action"]["actual"], "command_key": label_checks["sound_track_command"]["actual"], "mode": 1},
        ],
        "constructor_registration_spans": track_action_registrations,
        "qt_import": imports["QWidget::addAction(QAction*)"],
        "interpretation": "The Add Track submenu receives three registered actions in order: Channel, Effects, Sound. Their constructor-associated command keys name insertion of the corresponding track type into a TrackGroup; the shared callback receives mode values 0, 2, and 1 respectively.",
    },
    "cleanup_group_action": {
        "constructor_registration_span": cleanup_group_action_registration,
        "constructor_member_store_matches": installed_span_has(0x10179C3F, 0x10179C86, 0x14C),
        "visible_label_operand_matches": installed_span_has(0x10179C3F, 0x10179C86, 0x10AC3CA0),
        "command_key_operand_matches": installed_span_has(0x10179C3F, 0x10179C86, 0x10AC3CB4),
        "attachment_span": cleanup_group_action_attachment,
        "handler_va": "0x1017ED30",
        "handler_span": cleanup_group_handler,
        "group_remove_thunk_span": cleanup_group_remove_thunk,
        "callback_binding_wrapper_span": cleanup_callback_binding_multi,
        "action_member": "CQTimelineEditor +0x14C",
        "visible_label": label_checks["remove_muted_sounds_action_label"],
        "command_key": label_checks["remove_muted_sounds_command_key"],
        "interpretation": "The QAction stored at CQTimelineEditor +0x14C is constructed with visible label Remove Muted Sounds and command key RemoveMutedSounds, then attached to the Cleanup submenu with callback 0x1017ED30. Its byte-verified handler walks selected group entries in reverse, resolves each entry, checks an internal type value of 1 and a further helper predicate, then either removes the group entry or delegates to 0x1017DDC0. The helper predicate’s semantic meaning and the branch’s runtime/undo effects remain unverified.",
    },
    "cleanup_submenu_action": {
        "constructor_registration_span": cleanup_action_registration,
        "constructor_member_store_matches": installed_span_has(0x10179C86, 0x10179CB8, 0x150),
        "visible_label_operand_matches": installed_span_has(0x10179C86, 0x10179CB8, 0x10AC3CA0),
        "attachment_span": cleanup_action_attachment,
        "handler_va": "0x1017DDC0",
        "handler_span": remove_muted_sounds_handler,
        "index_remove_helper_span": cleanup_index_remove_helper,
        "collection_erase_adapter_span": cleanup_collection_erase_adapter,
        "callback_binding_wrapper_span": cleanup_callback_binding_single,
        "action_member": "CQTimelineEditor +0x150",
        "visible_label": label_checks["remove_muted_sounds_action_label"],
        "interpretation": "The QAction stored at CQTimelineEditor +0x150 is also constructed with the visible label Remove Muted Sounds and is attached with callback 0x1017DDC0. The byte-verified handler gates on a non-null object of internal type 1, starts a labeled operation, walks indexed items in reverse, tests each non-null item with a virtual predicate plus byte field +0x8C, removes qualifying entries through helper 0x104D2CB0, then calls completion/cleanup service slots. The predicate meaning and actual runtime/undo behavior remain unresolved.",
    },
    "track_kind_specific_utilities": {
        "byte_verification": track_kind_specific_actions,
        "root_title_enum_mapping": {"0": "Channel Track", "1": "Sound Track", "2": "Effects Track", "3": "Film Track"},
        "action_registrations": track_kind_action_registrations,
        "add_command_trigger_handler_span": add_command_trigger_handler,
        "branches": [
            {"track_kind": 2, "root_title": "Effects Track", "attached_action_member": "CQTimelineEditor +0x190", "label": label_checks["add_clip_to_track_action"]["actual"], "command_key": label_checks["add_clip_to_track_command"]["actual"], "callback_va": "0x1017CF60"},
            {"track_kind": 1, "root_title": "Sound Track", "attached_actions": [
                {"member": "CQTimelineEditor +0x190", "label": label_checks["add_clip_to_track_action"]["actual"], "command_key": label_checks["add_clip_to_track_command"]["actual"], "callback_va": "0x101692A0"},
                {"member": "CQTimelineEditor +0x194", "label": label_checks["set_volume_action"]["actual"], "command_key": label_checks["set_volume_command"]["actual"], "callback_va": "0x1017CDD0"},
                {"member": "CQTimelineEditor +0x198", "label": label_checks["record_narration_action"]["actual"], "command_key": label_checks["record_narration_command"]["actual"], "binding_helper_va": "0x1016ECE0", "callback_target_va": "0x1029AF10"},
            ]},
            {"track_kind": 0, "root_title": "Channel Track", "attached_action_member": "CQTimelineEditor +0x1A0", "label": label_checks["add_command_trigger_action"]["actual"], "command_key": label_checks["add_command_trigger_command"]["actual"], "callback_va": "0x1017D3F0", "handler_span": add_command_trigger_handler, "dialog_title": label_checks["edit_command_dialog"]["actual"], "dialog_prompt": label_checks["edit_command_prompt"]["actual"]},
            {"track_kind": 3, "root_title": "Film Track", "branch_specific_action": "none in this verified conditional block"},
        ],
        "interpretation": "The verified builder applies a track-kind-specific utility-action branch after Add Track. Effects tracks receive Add Clip to Track; Sound tracks receive Add Clip to Track, Set Volume, and Record Narration; Channel tracks receive Add Command Trigger; Film tracks take no branch-specific action in this block. The Channel callback body uses Edit Command and Enter command labels before handing the input to helper 0x10296380. This establishes dialog labels and dispatch route, not final document/runtime effects.",
    },
    "interpretation": "The concrete CQTimelineTrackView menu slot calls this helper. It chooses a root menu title from the track kind (Film, Effects, Sound, Channel, or generic Track), and its verified body creates Utilities, Add Track, and Cleanup submenus. Within Utilities, it registers and attaches the previously recovered Compact Track action. Other child actions remain for recovery.",
    "limits": [
        "Other internal action-population callbacks and their complete labels/behavior are not fully recovered here.",
        "The shared Add Track handler's downstream document/model mutations and undo behavior remain unverified.",
        "Static bytes do not establish which menu entries are enabled for a live scene or the displayed order after helper callbacks run.",
        "No live SFM menu was available for visual comparison.",
        "This adds reverse-engineering evidence only; no replacement UI has been implemented by this verifier.",
    ],
}
receipt["all_match"] = (
    body["all_match"]
    and utilities_compact_track["all_match"]
    and add_track_items["all_match"]
    and shared_add_track_handler["all_match"]
    and all(span["all_match"] for span in track_action_registrations.values())
    and track_kind_specific_actions["all_match"]
    and all(span["all_match"] for span in track_kind_action_registrations.values())
    and add_command_trigger_handler["all_match"]
    and cleanup_action_registration["all_match"]
    and cleanup_action_attachment["all_match"]
    and remove_muted_sounds_handler["all_match"]
    and cleanup_group_action_registration["all_match"]
    and cleanup_group_action_attachment["all_match"]
    and cleanup_group_handler["all_match"]
    and cleanup_index_remove_helper["all_match"]
    and cleanup_group_remove_thunk["all_match"]
    and cleanup_collection_erase_adapter["all_match"]
    and cleanup_callback_binding_single["all_match"]
    and cleanup_callback_binding_multi["all_match"]
    and receipt["cleanup_group_action"]["constructor_member_store_matches"]
    and receipt["cleanup_group_action"]["visible_label_operand_matches"]
    and receipt["cleanup_group_action"]["command_key_operand_matches"]
    and receipt["cleanup_submenu_action"]["constructor_member_store_matches"]
    and receipt["cleanup_submenu_action"]["visible_label_operand_matches"]
    and table == tuple(expected_roots)
    and all(item["matches"] for item in label_checks.values())
    and all("error" not in symbol for symbol in imports.values())
    and compact_receipt["action_label"] == "Compact Track"
    and compact_receipt["action_callback"] == "0x1017C8C0"
    and slot_receipt["sha256"] == binary_hash
)
Path("outputs/timeline-track-context-menu-population-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({
    "sha256": binary_hash,
    "verified_body": {key: value for key, value in body.items() if key != "mismatches"},
    "track_kind_titles": [label_checks[name]["actual"] for name in ("root_film", "root_effects", "root_sound", "root_channel")],
    "submenus": [label_checks[name]["actual"] for name in ("utilities_submenu", "add_track_submenu", "cleanup_submenu")],
    "utilities_action": compact_receipt["action_label"],
    "utilities_action_callback": compact_receipt["action_callback"],
    "utilities_attach_span": {key: value for key, value in utilities_compact_track.items() if key != "mismatches"},
    "add_track_actions": [
        {"label": item["label"], "command": item["command_key"], "mode": item["mode"]}
        for item in receipt["add_track_submenu_actions"]["member_order_and_type_values"]
    ],
    "add_track_attach_bytes": add_track_items["bytes"],
    "shared_handler_bytes": shared_add_track_handler["bytes"],
    "track_kind_specific_action_bytes": track_kind_specific_actions["bytes"],
    "add_command_trigger_handler_bytes": add_command_trigger_handler["bytes"],
    "cleanup_action_label": label_checks["remove_muted_sounds_action_label"]["actual"],
    "cleanup_handler_bytes": remove_muted_sounds_handler["bytes"],
    "cleanup_group_handler_bytes": cleanup_group_handler["bytes"],
    "cleanup_index_remove_helper_bytes": cleanup_index_remove_helper["bytes"],
    "cleanup_group_remove_thunk_bytes": cleanup_group_remove_thunk["bytes"],
    "cleanup_collection_erase_adapter_bytes": cleanup_collection_erase_adapter["bytes"],
    "cleanup_callback_binding_bytes": [cleanup_callback_binding_single["bytes"], cleanup_callback_binding_multi["bytes"]],
    "all_match": receipt["all_match"],
}, indent=2))
if not receipt["all_match"]:
    raise SystemExit(1)
