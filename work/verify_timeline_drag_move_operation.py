import hashlib
import json
import re
from pathlib import Path

from pe_image import Image


SOURCE = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
DISASM = Path("work/ifm-disassembly.txt")
RANGES = [
    (0x1016A530, 0x1016A5D9, "generic clip move factory"),
    (0x1016A5E0, 0x1016A62B, "film clip move factory prefix"),
    (0x10193010, 0x101933BA, "generic clip move operation constructor"),
    (0x10192D50, 0x10192EA4, "generic clip move callback at vtable slot 0"),
    (0x10192EB0, 0x10193004, "generic clip move callback at vtable slot 1"),
    (0x10196420, 0x101964E9, "Drop Clips Operation constructor"),
    (0x101963E0, 0x10196419, "Drop Clips Operation callback at vtable slot 13"),
    (0x101981A0, 0x10198228, "Drop Clips Operation shared event bridge"),
    (0x100B67D0, 0x100B67F2, "shared operation-manager forwarding method"),
]
VTABLES = {
    0x10AC6AE4: {0: 0x10192D50, 1: 0x10192EB0},
    0x10AC6D24: {8: 0x101964F0, 13: 0x101963E0},
}


def instructions():
    found = {}
    previous = None
    for line in DISASM.open(encoding="utf-8", errors="replace"):
        match = re.match(r"\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)", line)
        if match:
            previous = int(match[1], 16)
            found[previous] = (bytes.fromhex(match[2]), match[3].strip())
        else:
            continuation = re.match(r"^\s{12,}((?:[0-9A-F]{2}(?:\s+|$))+)$", line.rstrip())
            if continuation and previous is not None:
                raw, text = found[previous]
                found[previous] = (raw + bytes.fromhex(continuation[1]), text)
    return found


def recover(table, start, end):
    result = []
    cursor = start
    while cursor < end:
        raw, text = table[cursor]
        assert cursor + len(raw) <= end, (hex(cursor), len(raw), hex(end))
        result.append({"va": hex(cursor), "bytes": raw.hex(), "instruction": text})
        cursor += len(raw)
    assert cursor == end
    return result


image = Image(SOURCE)
table = instructions()
vtable_evidence = []
for table_va, expected in VTABLES.items():
    for slot, target in expected.items():
        actual = image.word(table_va + slot * 4)
        assert actual == target, (hex(table_va), slot, hex(actual), hex(target))
        vtable_evidence.append({"table": hex(table_va), "slot": slot, "method_va": hex(actual)})
spans = []
for start, end, label in RANGES:
    rows = recover(table, start, end)
    for row in rows:
        offset = image.offset(int(row["va"], 16))
        raw = bytes.fromhex(row["bytes"])
        assert image.b[offset:offset + len(raw)] == raw, row["va"]
    spans.append({"label": label, "start": hex(start), "end_exclusive": hex(end), "bytes": sum(len(bytes.fromhex(row["bytes"])) for row in rows), "instructions": rows})

receipt = {
    "binary": str(SOURCE),
    "binary_sha256": hashlib.sha256(image.b).hexdigest(),
    "evidence": {
        "operation_labels": {
            "generic": {"va": "0x10ac2f64", "text": "Drag Move Clips Operation"},
            "film_clip": {"va": "0x10ac2f80", "text": "Drag Move Film Clips Operation"},
            "drop": {"va": "0x10ac6d78", "text": "Drop Clips Operation"},
        },
        "observations": [
            "The generic factory requests an operation using the Drag Move Clips Operation label, allocates 0xC8 bytes, and calls constructor 0x10193010 with the source operation inputs.",
            "The film-clip factory independently requests Drag Move Film Clips Operation and its allocation size is 0x120 bytes.",
            "The constructor processes the clip-related input collections and computes a displacement as a supplied time minus a time resolved through the timeline interface, storing the result at operation offset +0x70; full input/output units and field naming remain unverified.",
            "The installed move-operation vtable slots 0 and 1 point to separate callbacks. Both walk the operation's 0x14-byte records and call the timeline property setter with different record fields (+4 versus +8); these appear to represent paired before/after positions, but the callback lifecycle and commit/undo labels are not yet recovered.",
            "The Drop Clips Operation constructor copies a counted array of 32-bit entries into operation-owned storage beginning at +0xB8. Its verified vtable slot 13 callback forwards that collection and other arguments through generic helper 0x101981A0, which conditionally updates a context flag and dispatches through the global operation manager's vtable slot +0x1F8. Destination insertion, gap/ripple behavior, and handler lifecycle remain unresolved.",
        ],
    },
    "verified_spans": spans,
    "verified_vtable_entries": vtable_evidence,
    "limitations": "Static installed-binary evidence only. No live SFM drag was performed. Collection field meanings, time snapping, overlap policy, gaps/ripple behavior, operation commit/undo, and matching x64 implementation remain unresolved.",
}
Path("outputs/timeline-drag-move-operation-recovery.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
print(json.dumps({"sha256": receipt["binary_sha256"], "spans": [(s["label"], s["bytes"], len(s["instructions"])) for s in spans], "observations": receipt["evidence"]["observations"]}, indent=2))
