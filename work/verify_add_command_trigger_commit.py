import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
image = Image(binary)
binary_hash = hashlib.sha256(image.b).hexdigest()


def verify_code(start, end):
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


track_menu = json.loads(Path("outputs/timeline-track-context-menu-population-verification.json").read_text())
assert track_menu["all_match"] and track_menu["sha256"] == binary_hash
channel_action = next(x for x in track_menu["track_kind_specific_utilities"]["branches"] if x["root_title"] == "Channel Track")
assert channel_action["callback_va"] == "0x1017D3F0"
assert channel_action["command_key"] == "AddCommandTrigger"

spans = {
    "dialog_action_callback": verify_code(0x1017D3F0, 0x1017D505),
    "command_commit_and_document_update": verify_code(0x10296380, 0x10296637),
    "symbol_attribute_write_helper": verify_code(0x1000B320, 0x1000B34F),
    "active_attribute_write_helper": verify_code(0x104CD210, 0x104CD27F),
}
string_map = {
    "command_channel_element_type": (0x10AE4E30, "command channelsclip"),
    "activate_command_channel_record": (0x10AE4E14, "activate command channel"),
    "active_log": (0x10AE4E08, "active log"),
    "command_attribute": (0x10AD1D40, "command"),
    "misc_bin_attribute": (0x10AE4E00, "miscBin"),
    "active_attribute": (0x10AB7F5C, "active"),
}
strings = {key: {"va": hex(va), "expected": text, "actual": image.string(va), "matches": image.string(va) == text} for key, (va, text) in string_map.items()}
receipt = {
    "binary": binary,
    "sha256": binary_hash,
    "track_menu_evidence": "outputs/timeline-track-context-menu-population-verification.json",
    "entry_action": {"label": channel_action["label"], "command_key": channel_action["command_key"], "dialog_title": channel_action["dialog_title"], "dialog_prompt": channel_action["dialog_prompt"], "callback_va": channel_action["callback_va"]},
    "code_spans": spans,
    "downstream_data_path": {
        "strings": strings,
        "observed_call_targets": ["0x101D5CF0", "0x101D5D50", "0x1027B290", "0x1027E610", "0x1000B320", "0x104CD210"],
        "observed_flow": [
            "The downstream routine gates the incoming element/command before processing.",
            "It resolves the command channelsclip typed object and an activate command channel object using the associated model/context.",
            "It accesses the active log record, looks up the command attribute, and passes the dialog-provided value to helper 0x1000B320; that wrapper resolves the symbol/string and calls the string-attribute update path 0x10366100.",
            "It inspects or creates miscBin data, then calls 0x104CD210 on the activate command channel record with the command attribute handle, field name active, and numeric value 0. That wrapper updates its typed field state and notifies the owner.",
            "It invokes document/service callbacks on both the success and cleanup paths.",
        ],
    },
    "verified_write_helpers": {
        "command_symbol_write": {
            "span": spans["symbol_attribute_write_helper"],
            "behavior": "The wrapper takes a symbol/string value, asks the data-model service at virtual offset +0xC8 to convert/resolve it, then calls 0x10366100 on the wrapped destination field. The existing element-string recovery identifies 0x10366100 as updating a string/symbol attribute.",
            "prior_recovery_context": "outputs/element-constructor-string-service-recovery.json",
        },
        "active_field_update": {
            "span": spans["active_attribute_write_helper"],
            "caller_arguments": {"target_record": "activate command channel", "associated_attribute": "command attribute handle", "field_name": "active", "value": 0},
            "behavior": "The wrapper resolves/normalizes the target attribute, updates its cached data-model wrappers, sends the third argument through a typed scalar setter path, and calls the owner notification helper 0x104CC8F0.",
        },
    },
    "limits": [
        "The exact schema meaning of the active field and the complete internal semantics of 0x10366100/0x1036CEF0 are broader data-model work; this receipt establishes the command-symbol and active=0 update calls.",
        "This is static data/control-flow evidence; it does not prove runtime dialog acceptance, resulting session bytes, transaction/undo behavior, or successful playback of the inserted command.",
    ],
}
receipt["all_match"] = (
    all(span["all_match"] for span in spans.values())
    and all(item["matches"] for item in strings.values())
    and track_menu["sha256"] == binary_hash
    and channel_action["callback_va"] == "0x1017D3F0"
)
Path("outputs/timeline-add-command-trigger-commit-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"sha256": binary_hash, "spans": {name: {k: v for k, v in span.items() if k != "mismatches"} for name, span in spans.items()}, "strings_match": all(item["matches"] for item in strings.values()), "all_match": receipt["all_match"]}, indent=2))
if not receipt["all_match"]:
    raise SystemExit(1)
