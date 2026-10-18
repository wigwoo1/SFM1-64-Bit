import hashlib
import json
import re
from pathlib import Path

from pe_image import Image


SOURCE = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
DISASM = Path("work/ifm-disassembly.txt")
LOOKUP = (0x10197E20, 0x10197EAA)
RIPPLE_LABEL_HELPER = (0x10198350, 0x101983AA)
ACTION_LABEL_CALLER = (0x10187400, 0x101876B4)
ACTION_CODE_PRODUCER = (0x10166990, 0x10166B2F)
ACTION_DISPATCH = (0x10188F1A, 0x10188FD7)
OPERATION_FACTORIES = [
    (0x1016A340, 0x1016A3D9, "Drag Edge Operation factory wrapper"),
    (0x1016A3E0, 0x1016A479, "Drag Clip Operation factory wrapper"),
    (0x1016A480, 0x1016A521, "Drag Film InOut Operation factory wrapper"),
]
CODE_PRODUCERS = [
    (0x10197B70, 0x10197BB1, "in-edge action code helper", (1, 2, 3, 4), {"000": 1, "001": 2, "010": 4, "011": 1, "100": 1, "101": 1, "110": 1, "111": 3}),
    (0x10197BC0, 0x10197C01, "out-edge action code helper", (5, 6, 7, 8), {"000": 5, "001": 6, "010": 8, "011": 5, "100": 5, "101": 5, "110": 5, "111": 7}),
    (0x10197C10, 0x10197C45, "clip-level action code helper", (14, 15, 16), {"000": 0, "001": 0, "010": 0, "011": 0, "100": 0, "101": 15, "110": 14, "111": 16}),
    (0x10197C50, 0x10197C9B, "paired in/out action code helper", (9, 10, 11, 12, 13), {"000": 9, "001": 10, "010": 12, "011": 13, "100": 9, "101": 10, "110": 9, "111": 11}),
]
TABLE = 0x10197EAC


def instruction_table():
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


def recover(image, table, start, end):
    rows = []
    cursor = start
    while cursor < end:
        raw, text = table[cursor]
        assert cursor + len(raw) <= end, (hex(cursor), len(raw), hex(end))
        assert image.b[image.offset(cursor):image.offset(cursor) + len(raw)] == raw
        rows.append({"va": hex(cursor), "bytes": raw.hex(), "instruction": text})
        cursor += len(raw)
    assert cursor == end
    return rows


def c_string(image, va):
    offset = image.offset(va)
    end = image.b.index(0, offset)
    return image.b[offset:end].decode("ascii")


image = Image(SOURCE)
instructions = instruction_table()
lookup_code = recover(image, instructions, *LOOKUP)
ripple_code = recover(image, instructions, *RIPPLE_LABEL_HELPER)
caller_code = recover(image, instructions, *ACTION_LABEL_CALLER)
producer_code = recover(image, instructions, *ACTION_CODE_PRODUCER)
dispatch_code = recover(image, instructions, *ACTION_DISPATCH)
operation_factories = [
    {"label": label, "start": hex(start), "end_exclusive": hex(end), "instructions": recover(image, instructions, start, end)}
    for start, end, label in OPERATION_FACTORIES
]
code_producers = []
for start, end, label, output_codes, truth_table in CODE_PRODUCERS:
    if start == 0x10197B70:
        # The linear disassembly entered this function two bytes late, so verify
        # and restore the actual function prologue before using its decoded body.
        prologue = image.b[image.offset(start):image.offset(start) + 3]
        assert prologue == bytes.fromhex("558bec")
        rows = [
            {"va": hex(start), "bytes": "55", "instruction": "push ebp"},
            {"va": hex(start + 1), "bytes": "8bec", "instruction": "mov ebp,esp"},
        ] + recover(image, instructions, start + 3, end)
    else:
        rows = recover(image, instructions, start, end)
    found_codes = set()
    for row in rows:
        match = re.search(r"mov\s+eax,([0-9A-F]+)(h?)$", row["instruction"])
        if match:
            found_codes.add(int(match[1], 16 if match[2] else 10))
        if re.search(r"xor\s+eax,eax", row["instruction"]):
            found_codes.add(0)
    assert found_codes == set(output_codes) | ({0} if 0 in truth_table.values() else set()), (label, found_codes, output_codes)
    assert set(truth_table.values()) <= set(output_codes) | {0}
    code_producers.append({"label": label, "start": hex(start), "end_exclusive": hex(end), "input_order": ["byte [ebp+8]", "byte [ebp+0xC]", "byte [ebp+0x10]"], "output_codes": list(output_codes), "truth_table": truth_table, "bytes": sum(len(bytes.fromhex(row["bytes"])) for row in rows), "instructions": rows})

flag_extractors = {
    0x10166A45: "shr ecx,19h",
    0x10166A48: "shr eax,1Bh",
    0x10166A4E: "shr edx,1Ah",
    0x10166A56: "mov byte ptr [ebp-8],cl",
    0x10166A5F: "mov byte ptr [ebp+8],al",
    0x10166A67: "mov byte ptr [ebp-4],dl",
}
for va, expected_instruction in flag_extractors.items():
    actual_instruction = re.sub(r"\s+", " ", instructions[va][1]).strip()
    assert actual_instruction == expected_instruction, (hex(va), actual_instruction)

caller_xrefs = {
    0x10187C99: {"setup": (0x10187C93, 0x10187C99), "argument": "caller stack argument [ebp+8]", "instructions": ["mov edx,dword ptr [ebp+8]", "push edx", "mov ecx,esi"]},
    0x10187D8F: {"setup": (0x10187D89, 0x10187D8F), "argument": "address of caller object field [esi+0x40]", "instructions": ["lea eax,[esi+40h]", "push eax", "mov ecx,esi"]},
    0x1018982F: {"setup": (0x10189829, 0x1018982F), "argument": "address of local pair [ebp-0x30]", "instructions": ["lea edx,[ebp-30h]", "push edx", "mov ecx,esi"]},
}
for call_va, evidence in caller_xrefs.items():
    assert instructions[call_va][1].startswith("call        10187400"), (hex(call_va), instructions[call_va][1])
    start, end = evidence["setup"]
    cursor = start
    sequence = []
    while cursor < end:
        raw, text = instructions[cursor]
        actual = re.sub(r"\s+", " ", text).strip()
        sequence.append(actual)
        cursor += len(raw)
    assert sequence == evidence["instructions"], (hex(call_va), sequence)
    evidence["call_instruction"] = re.sub(r"\s+", " ", instructions[call_va][1]).strip()
    evidence["setup_start"] = hex(start)

actions = []
for action_code in range(1, 17):
    target = image.word(TABLE + (action_code - 1) * 4)
    target_row = instructions[target]
    match = re.search(r"mov\s+eax,([0-9A-F]+)h", target_row[1])
    assert match, (hex(target), target_row[1])
    label_va = int(match[1], 16)
    label = c_string(image, label_va)
    actions.append({"code": action_code, "case_va": hex(target), "label_va": hex(label_va), "label": label})

expected = [
    "Roll In", "Stick In", "Scale In", "Create Gap In",
    "Roll Out", "Stick Out", "Scale Out", "Create Gap Out",
    "In/Out Roll Roll", "In/Out Roll Stick", "In/Out Scale Scale", "In/Out Stick Roll",
    "In/Out Stick Stick", "Clip Roll", "Clip Roll Stick Roll", "Clip Scale",
]
assert [item["label"] for item in actions] == expected

# The editor consumer uses a compact byte table indexed by code - 1, then an
# indirect dword table. Verify every mapping from bytes in the installed image.
consumer_slot_table = 0x10189154
consumer_handler_table = 0x10189148
consumer_dispatch = []
for code in range(1, 17):
    slot = image.b[image.offset(consumer_slot_table + code - 1)]
    handler = image.word(consumer_handler_table + slot * 4)
    assert handler in instructions, (code, slot, hex(handler))
    consumer_dispatch.append({"code": code, "slot": slot, "handler_va": hex(handler)})
assert [x["slot"] for x in consumer_dispatch] == [0] * 8 + [1] * 5 + [2] * 3
assert [x["handler_va"] for x in consumer_dispatch] == ["0x10188f34"] * 8 + ["0x10188f82"] * 5 + ["0x10188f5b"] * 3

operation_strings = {
    "edge": {"va": hex(0x10AC2F20), "text": c_string(image, 0x10AC2F20)},
    "clip": {"va": hex(0x10AC2F34), "text": c_string(image, 0x10AC2F34)},
    "film_inout": {"va": hex(0x10AC2F48), "text": c_string(image, 0x10AC2F48)},
}
assert [x["text"] for x in operation_strings.values()] == ["Drag Edge Operation", "Drag Clip Operation", "Drag Film InOut Operation"]
paired_operation_string = {"va": hex(0x10AC6A60), "text": c_string(image, 0x10AC6A60)}
assert paired_operation_string["text"] == "Drag InOut Operation"

receipt = {
    "binary": str(SOURCE),
    "binary_sha256": hashlib.sha256(image.b).hexdigest(),
    "verified_spans": [
        {"label": "clip action enum-to-label switch", "start": hex(LOOKUP[0]), "end_exclusive": hex(LOOKUP[1]), "bytes": sum(len(bytes.fromhex(row["bytes"])) for row in lookup_code), "instructions": lookup_code},
        {"label": "conditional ripple label helper", "start": hex(RIPPLE_LABEL_HELPER[0]), "end_exclusive": hex(RIPPLE_LABEL_HELPER[1]), "bytes": sum(len(bytes.fromhex(row["bytes"])) for row in ripple_code), "instructions": ripple_code},
        {"label": "caller of clip action label helper", "start": hex(ACTION_LABEL_CALLER[0]), "end_exclusive": hex(ACTION_LABEL_CALLER[1]), "bytes": sum(len(bytes.fromhex(row["bytes"])) for row in caller_code), "instructions": caller_code},
        {"label": "action-code producer called by label caller", "start": hex(ACTION_CODE_PRODUCER[0]), "end_exclusive": hex(ACTION_CODE_PRODUCER[1]), "bytes": sum(len(bytes.fromhex(row["bytes"])) for row in producer_code), "instructions": producer_code},
        {"label": "timeline action-code consumer dispatch", "start": hex(ACTION_DISPATCH[0]), "end_exclusive": hex(ACTION_DISPATCH[1]), "bytes": sum(len(bytes.fromhex(row["bytes"])) for row in dispatch_code), "instructions": dispatch_code},
        *code_producers,
        *[{**x, "bytes": sum(len(bytes.fromhex(r["bytes"])) for r in x["instructions"])} for x in operation_factories],
    ],
    "action_label_jump_table_va": hex(TABLE),
    "actions": actions,
    "action_code_groups": [
        {"codes": [1, 2, 3, 4], "labels": [item["label"] for item in actions[:4]]},
        {"codes": [5, 6, 7, 8], "labels": [item["label"] for item in actions[4:8]]},
        {"codes": [9, 10, 11, 12, 13], "labels": [item["label"] for item in actions[8:13]]},
        {"codes": [14, 15, 16], "labels": [item["label"] for item in actions[13:]]},
    ],
    "timeline_action_consumer": {
        "dispatch_span": [hex(ACTION_DISPATCH[0]), hex(ACTION_DISPATCH[1])],
        "compact_slot_table_va": hex(consumer_slot_table),
        "handler_pointer_table_va": hex(consumer_handler_table),
        "code_to_handler": consumer_dispatch,
        "operation_factories": [
            {"codes": [1, 2, 3, 4, 5, 6, 7, 8], "factory_va": "0x1016a340", "operation_label": operation_strings["edge"]},
            {"codes": [14, 15, 16], "factory_va": "0x1016a3e0", "operation_label": operation_strings["clip"]},
            {"codes": [9, 10, 11, 12, 13], "factory_va": "0x1016a480", "operation_label": operation_strings["film_inout"]},
        ],
        "notes": [
            "Codes 1-8 dispatch to a wrapper that requests a Drag Edge Operation.",
            "Codes 9-13 dispatch to a wrapper that requests a Drag Film InOut Operation, whose constructor passes the distinct internal Drag InOut Operation label to the shared base initializer.",
            "Codes 14-16 dispatch to a wrapper that requests a Drag Clip Operation.",
            "These are consumer-side routing groups; individual action-label semantics and operation commit/undo behavior remain unverified.",
        ],
    },
    "action_flag_inputs": {
        "packed_argument": "second stack argument to 0x10166990 (forwarded from the caller's argument at 0x10187400)",
        "bits": {"first_boolean": 25, "second_boolean": 26, "third_boolean": 27},
        "evidence": [{"va": hex(va), "instruction": instruction} for va, instruction in flag_extractors.items()],
        "note": "The bit positions and order are verified; their user-facing meanings are not yet recovered.",
    },
    "label_update_callers": [{"call_va": hex(call_va), **evidence} for call_va, evidence in caller_xrefs.items()],
    "callsite_observation": "The verified caller at 0x10187400 invokes the label helper twice on alternative branches, passing a 1 or 0 mode value. Its action-code input is returned by 0x10166990, which extracts bits 25, 26, and 27 from its second stack argument, then dispatches to the edge, paired-edge, or clip-level code helpers (0x10197B70, 0x10197BC0, 0x10197C10, 0x10197C50) based on interval comparisons. This links the label catalog to interval-dependent native formatting, but does not identify the owning UI dialog or prove clip-drop commit semantics.",
    "ripple_label": {
        "va": "0x10ac71ec",
        "text": ", Ripple",
        "note": "The verified helper appends this suffix under conditional branches; the suffix is presentation text and does not establish the drop operation's mutation policy.",
    },
    "limitations": "Static installed-binary evidence only. This recovers labels, enum mapping, consumer dispatch groups, and operation factory labels, not the dialog's complete choices, individual action semantics, drag/drop commit behavior, operation undo policy, or runtime visibility conditions for every label.",
}
Path("outputs/timeline-clip-action-labels.json").write_text(json.dumps(receipt, indent=2), encoding="utf-8")
print(json.dumps({"sha256": receipt["binary_sha256"], "actions": actions, "verified_spans": [(s["label"], s["bytes"]) for s in receipt["verified_spans"]]}, indent=2))
