import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
disassembly = Path("work/ifm-disassembly.txt")
image = Image(binary)


def checked_span(start, end):
    rows = []
    current = None
    found = False
    for line in disassembly.open(errors="replace"):
        match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
        if match:
            address = int(match.group(1), 16)
            if address < start:
                continue
            found = True
            if address >= end:
                break
            if current:
                rows.append(current)
            current = [address, bytes.fromhex(match.group(2))]
        elif found and current and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
            current[1] += bytes.fromhex(line.strip())
    if current:
        rows.append(current)
    cursor = start
    result = bytearray()
    for address, chunk in rows:
        if address != cursor:
            return None
        try:
            raw = image.b[image.offset(address):image.offset(address) + len(chunk)]
        except (ValueError, IndexError):
            return None
        if raw != chunk:
            return None
        result.extend(raw)
        cursor += len(chunk)
    return bytes(result) if cursor == end else None


def contains(code, pattern):
    return code is not None and bytes.fromhex(pattern) in code


# The 7-byte helper is a direct read of the DAG's child-count field.
child_count = checked_span(0x104C2B00, 0x104C2B07)
# Helper invoked with the selected DAG and its undo-operation context.
record_target = checked_span(0x104C2B10, 0x104C2B4E)
# Complete registered callback body, including its early exits and transaction end.
delete_callback = checked_span(0x1008CBE0, 0x1008CC3E)
# Caller-side menu enable calculation around the count-helper call.
menu_predicate = checked_span(0x10087EEE, 0x10087F1B)

receipt = {
    "binary": str(binary),
    "sha256": hashlib.sha256(image.b).hexdigest(),
    "method": "Delete Empty Dag callback and menu predicate byte verification",
    "ranges": {
        "child_count_helper": {
            "start": "0x104c2b00", "end_exclusive": "0x104c2b07",
            "bytes": len(child_count) if child_count else 0,
            "disassembly_matches_installed_bytes": child_count is not None,
        },
        "transaction_target_recorder": {
            "start": "0x104c2b10", "end_exclusive": "0x104c2b4e",
            "bytes": len(record_target) if record_target else 0,
            "disassembly_matches_installed_bytes": record_target is not None,
        },
        "delete_callback": {
            "start": "0x1008cbe0", "end_exclusive": "0x1008cc3e",
            "bytes": len(delete_callback) if delete_callback else 0,
            "disassembly_matches_installed_bytes": delete_callback is not None,
        },
        "menu_predicate": {
            "start": "0x10087eee", "end_exclusive": "0x10087f1b",
            "bytes": len(menu_predicate) if menu_predicate else 0,
            "disassembly_matches_installed_bytes": menu_predicate is not None,
        },
    },
    "findings": {
        "helper_loads_dag_child_count_at_plus_0x90": contains(child_count, "8B8190000000C3"),
        "menu_inverts_zero_test_to_enable_empty_only": contains(menu_predicate, "E80DAC430085C00F94C0"),
        "callback_rejects_null_dag": contains(delete_callback, "8B7D0885FF744E"),
        "callback_reads_child_count": contains(delete_callback, "8BCFE80E5F430085C07F43"),
        "callback_skips_when_child_count_positive": contains(delete_callback, "85C07F43"),
        "callback_begins_named_undo_action": contains(delete_callback, "6A0068006BAA1068006BAA10"),
        "target_recorder_accepts_the_dag_pointer": contains(record_target, "8B4D08E8122DE7FF"),
        "target_recorder_uses_operation_collection_at_plus_0x84": contains(record_target, "8D8E84000000"),
        "target_recorder_inserts_into_collection": contains(record_target, "E8A05AECFF"),
        "callback_passes_the_dag_to_transaction_context": contains(delete_callback, "578BCEE8E85E4300"),
        "callback_finishes_transaction": contains(delete_callback, "8B900C010000FFD2"),
        "menu_label": image.string(0x10AA6B00),
    },
    "interpretation": "Delete Empty Dag is enabled for a DAG whose +0x90 child count is zero. Its callback checks the count again, opens a named undo transaction, passes the selected DAG to a helper that records it in an operation-context collection, and calls the transaction-finalization interface. These bytes do not establish how or when the graph element is physically removed, how incoming references are repaired, or which notifications are emitted.",
    "limitations": [
        "Static x86 control-flow evidence only; the installed SFM UI callback was not invoked by this verifier.",
        "The +0x90 field is identified as child count by its direct read and zero/positive control flow; its native type declaration is unavailable here.",
        "The registered helper records the target in a context collection; its caller and transaction finalizer semantics are not fully decompiled.",
        "The callback's registration and finalization calls alone do not prove that graph removal or reference repair occurs.",
    ],
}
checks = [
    value for key, value in receipt["findings"].items()
    if key != "menu_label"
]
receipt["all_match"] = (
    all(item["disassembly_matches_installed_bytes"] for item in receipt["ranges"].values())
    and all(checks)
    and receipt["findings"]["menu_label"] == "Delete Empty Dag"
    and receipt["sha256"] == "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
)
Path("outputs/delete-empty-dag-handler-verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({
    "sha256": receipt["sha256"],
    "ranges": receipt["ranges"],
    "findings": receipt["findings"],
    "all_match": receipt["all_match"],
}, indent=2))
if not receipt["all_match"]:
    sys.exit(1)
