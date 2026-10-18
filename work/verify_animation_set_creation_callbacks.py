import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
raw = binary.read_bytes()
im = Image(binary)
digest = hashlib.sha256(raw).hexdigest()
expected_digest = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
if digest != expected_digest:
    raise AssertionError(f"unexpected installed ifm.dll hash {digest}")

disassembly = Path("work/ifm-disassembly.txt")


def decode_span(start, end):
    chunks = []
    cursor = start
    active = False
    for line in disassembly.open(errors="replace"):
        match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
        if match:
            va = int(match.group(1), 16)
            if va < start:
                continue
            if va >= end:
                break
            active = True
            if va != cursor:
                raise AssertionError(f"non-contiguous disassembly at {va:#x}, expected {cursor:#x}")
            chunk = bytes.fromhex(match.group(2))
            if va + len(chunk) > end:
                raise AssertionError(f"range ends inside instruction at {va:#x}")
            chunks.append(chunk)
            cursor += len(chunk)
        elif active and cursor < end and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
            chunk = bytes.fromhex(line.strip())
            if cursor + len(chunk) > end:
                raise AssertionError(f"range ends inside continued instruction at {cursor:#x}")
            chunks.append(chunk)
            cursor += len(chunk)
    if cursor != end:
        raise AssertionError(f"disassembly ends at {cursor:#x}, expected {end:#x}")
    code = b"".join(chunks)
    if raw[im.offset(start):im.offset(start) + len(code)] != code:
        raise AssertionError(f"installed DLL differs from disassembly at {start:#x}")
    return code


actions = [
    {
        "command_id": 0x1B9,
        "label_va": 0x10AA71B8,
        "label": "Create Animation Set for New Camera",
        "wrapper": (0x1008CDA0, 0x1008CDBD),
        "delegated_creator": 0x102234A0,
        "creator_name_va": 0x10ACEC6C,
    },
    {
        "command_id": 0x1BA,
        "label_va": 0x10AA7174,
        "label": "Create Animation Set for New Light",
        "wrapper": (0x1008CDC0, 0x1008CDDD),
        "delegated_creator": 0x10223710,
        "creator_name_va": 0x10ACEC88,
    },
    {
        "command_id": 0x1BB,
        "label_va": 0x10AA7130,
        "label": "Create Animation Set for New Model",
        "wrapper": (0x1008CDE0, 0x1008CE07),
        "delegated_creator": 0x10125160,
        "creator_name_va": 0x10ACECA4,
    },
    {
        "command_id": 0x1BC,
        "label_va": 0x10AA70D8,
        "label": "Create Animation Set for New Particle System",
        "wrapper": (0x1008CE10, 0x1008CE6F),
        "delegated_creator": 0x101D57D0,
        "creator_name_va": None,
    },
]

# The named camera/light creators also expose useful static evidence about
# category bins and generated labels. Check these exact string references in
# the installed DLL, then compare the surrounding instructions against bytes.
creator_evidence = [
    {
        "name": "camera",
        "start": 0x102234A0,
        "end": 0x10223710,
        "category_va": 0x10AB1910,
        "category": "Cameras",
        "generated_name_va": 0x10AD84EC,
        "generated_name": "camera%d",
        "dialog_va": 0x10ACEC6C,
        "dialog": "CreateAnimationSetAndCamera",
        "helper_call": 0x10223634,
        "helper_target": 0x1072CF10,
        "success_target": 0x102236AE,
        "null_target": 0x10223642,
        "call_branch_bytes": bytes.fromhex("E8 D7 98 50 00 8B F8 83 C4 0C 85 FF 75 6C"),
    },
    {
        "name": "light",
        "start": 0x10223710,
        "end": 0x102239A0,
        "category_va": 0x10AB1908,
        "category": "Lights",
        "generated_name_va": 0x10AD84F8,
        "generated_name": "light%d",
        "dialog_va": 0x10ACEC88,
        "dialog": "CreateAnimationSetAndLight",
        "helper_call": 0x10223870,
        "helper_target": 0x1072D020,
        "success_target": 0x102238EA,
        "null_target": 0x1022387E,
        "call_branch_bytes": bytes.fromhex("E8 AB 97 50 00 8B F8 83 C4 0C 85 FF 75 6C"),
    },
]

for creator in creator_evidence:
    code = decode_span(creator["start"], creator["end"])
    for key in ("category", "generated_name", "dialog"):
        actual = im.string(creator[f"{key}_va"])
        if actual != creator[key]:
            raise AssertionError(f"{creator['name']} {key}: expected {creator[key]!r}, got {actual!r}")
    for value in (creator["category_va"], creator["generated_name_va"]):
        va = value
        expected_ref = b"\x68" + va.to_bytes(4, "little")
        if expected_ref not in code:
            raise AssertionError(f"{creator['name']} creator does not push expected string pointer {value}")
    call_offset = creator["helper_call"] - int(creator["start"])
    if code[call_offset:call_offset + len(creator["call_branch_bytes"])] != creator["call_branch_bytes"]:
        raise AssertionError(f"{creator['name']} helper/null/success branch bytes changed")

    # Both native creators open a named editor operation through slot +0x108
    # before adding elements, then close it through +0x10C on the success path.
    # This pattern is the same one byte-checked for the native rename scope.
    scope = {
        "camera": {
            "begin_slot_load": 0x102234BB,
            "begin_call": 0x102234E6,
            "success_complete_slot_load": 0x102236DA,
            "failure_complete_slot_load": 0x1022365E,
        },
        "light": {
            "begin_slot_load": 0x1022372B,
            "begin_call": 0x10223756,
            "success_complete_slot_load": 0x10223916,
            "failure_complete_slot_load": 0x1022389A,
        },
    }[creator["name"]]
    expected = {
        "begin_slot_load": bytes.fromhex("8B 90 08 01 00 00"),
        "begin_call": bytes.fromhex("FF D2"),
        "success_complete_slot_load": bytes.fromhex("8B 82 0C 01 00 00 FF D0"),
        "failure_complete_slot_load": bytes.fromhex("8B 82 0C 01 00 00 FF D0"),
    }
    for key, va in scope.items():
        offset = va - int(creator["start"])
        code_bytes = expected[key]
        if code[offset:offset + len(code_bytes)] != code_bytes:
            raise AssertionError(f"{creator['name']} operation-scope bytes changed at {va:#x}")
    creator["named_operation_scope"] = {
        "operation": creator["dialog"],
        "begin_service_slot": "0x108",
        "begin_slot_load": hex(scope["begin_slot_load"]),
        "begin_call": hex(scope["begin_call"]),
        "complete_service_slot": "0x10C",
        "success_complete_slot_load": hex(scope["success_complete_slot_load"]),
        "failure_complete_slot_load": hex(scope["failure_complete_slot_load"]),
        "evidence_scope": "native named-operation bracket; does not prove undo replay details",
    }
    creator["helper_call"] = hex(creator["helper_call"])
    creator["helper_target"] = hex(creator["helper_target"])
    creator["non_null_continuation"] = hex(creator["success_target"])
    creator["null_cleanup"] = hex(creator["null_target"])
    del creator["success_target"]
    del creator["null_target"]
    del creator["call_branch_bytes"]
    creator["start"] = hex(creator["start"])
    creator["end_exclusive"] = hex(creator["end"])
    creator["category_va"] = hex(creator["category_va"])
    creator["generated_name_va"] = hex(creator["generated_name_va"])
    creator["dialog_va"] = hex(creator["dialog_va"])
    del creator["end"]

# Verify the complete camera-control helper and the exact static structure of
# its eight-entry loop.  This establishes what data is forwarded to the
# shared control builder without guessing the builder's UI/DMX semantics.
camera_helper_start = 0x1072CF10
camera_helper_end = 0x1072D012
camera_helper = decode_span(camera_helper_start, camera_helper_end)


def require_helper_bytes(va, expected):
    offset = va - camera_helper_start
    actual = camera_helper[offset:offset + len(expected)]
    if actual != expected:
        raise AssertionError(
            f"camera helper bytes at {va:#x}: expected {expected.hex(' ')}, got {actual.hex(' ')}"
        )


require_helper_bytes(0x1072CF85, bytes.fromhex("BE C8 F6 EF 10"))  # mov esi, camera table
require_helper_bytes(0x1072CFD8, bytes.fromhex("81 FE 48 F7 EF 10"))  # compare with table end
require_helper_bytes(0x1072CFCD, bytes.fromhex("E8 0E 99 FF FF"))  # shared control builder
require_helper_bytes(0x1072CFFA, bytes.fromhex("E8 41 9D FF FF"))  # post-loop setup helper
require_helper_bytes(0x1072D001, bytes.fromhex("E8 AA FC FF FF"))  # final helper
camera_table_start = 0x10EFF6C8
camera_table_end = 0x10EFF748
camera_table_stride = 16
camera_control_count = (camera_table_end - camera_table_start) // camera_table_stride
if camera_control_count != 8 or (camera_table_end - camera_table_start) % camera_table_stride:
    raise AssertionError("camera control table no longer has eight 16-byte records")

camera_helper_evidence = {
    "start": hex(camera_helper_start),
    "end_exclusive": hex(camera_helper_end),
    "byte_length": len(camera_helper),
    "table_start": hex(camera_table_start),
    "table_end_exclusive": hex(camera_table_end),
    "table_record_stride_bytes": camera_table_stride,
    "table_record_count": camera_control_count,
    "observed_operations": [
        "walks the 16-byte camera-control records in order",
        "passes record-associated values and default/min/max float values to helper 0x107268E0",
        "calls helper 0x10726D40 after the loop",
        "calls helper 0x1072CCB0 and returns the created object pointer",
    ],
    "limitations": "The shared helper 0x107268E0 is structurally decoded separately, but its setup helpers remain semantically opaque; this evidence does not establish widget types, full DMX schema, units, undo behavior, or runtime results.",
}

# This generic creator is the routine invoked once for every camera property.
# Its field-name strings and executable bytes provide evidence of native
# channel/expression wiring, but not enough to reproduce the complete editor
# widget or document schema.
control_builder_start = 0x107268E0
control_builder_end = 0x10726D3C
control_builder = decode_span(control_builder_start, control_builder_end)

numeric_entry_call_va = 0x107269DA
numeric_entry_call = control_builder[numeric_entry_call_va - control_builder_start: numeric_entry_call_va - control_builder_start + 5]
if numeric_entry_call != bytes.fromhex("E8 31 FD FF FF"):
    raise AssertionError("numeric-control helper call to 0x10726710 changed")
if numeric_entry_call_va + 5 + int.from_bytes(numeric_entry_call[1:], "little", signed=True) != 0x10726710:
    raise AssertionError("numeric-control call target is not 0x10726710")


def require_control_builder_bytes(va, expected):
    offset = va - control_builder_start
    actual = control_builder[offset:offset + len(expected)]
    if actual != expected:
        raise AssertionError(
            f"shared control-builder bytes at {va:#x}: expected {expected.hex(' ')}, got {actual.hex(' ')}"
        )


for va, expected in [
    (0x10726959, "80 FA 03"),
    (0x1072697E, "68 08 E0 A8 10"),
    (0x10726A11, "68 58 E0 A8 10"),
    (0x10726B0A, "68 C4 4A AE 10"),
    (0x10726B1B, "68 BC 4A AE 10"),
    (0x10726C77, "68 F4 F9 AC 10"),
    (0x10726CFB, "68 90 D6 AA 10"),
]:
    require_control_builder_bytes(va, bytes.fromhex(expected))

control_builder_strings = {
    "value": (0x10A8E008, "value"),
    "channel": (0x10A8E058, "channel"),
    "expression": (0x10AE4AC4, "lerp(value, lo, hi)"),
    "expression_attribute": (0x10AE4ABC, "expr"),
    "lower_bound": (0x10A8E01C, "lo"),
    "upper_bound": (0x10A8E020, "hi"),
    "rescale_expression_format": (0x10BC219C, "%s_rescale"),
    "scaled_channel_format": (0x10BC2188, "scaled_%s_channel"),
    "result_attribute": (0x10ACF9F4, "result"),
    "default_value_attribute": (0x10AAD690, "defaultValue"),
}
for field, (va, expected) in control_builder_strings.items():
    actual = im.string(va)
    if actual != expected:
        raise AssertionError(f"control-builder {field}: expected {expected!r}, got {actual!r}")
    if b"\x68" + va.to_bytes(4, "little") not in control_builder:
        raise AssertionError(f"control builder does not reference expected {field} string at {va:#x}")

control_builder_evidence = {
    "start": hex(control_builder_start),
    "end_exclusive": hex(control_builder_end),
    "byte_length": len(control_builder),
    "field_strings": {name: {"va": hex(va), "value": value} for name, (va, value) in control_builder_strings.items()},
    "observed_operations": [
        "checks an input attribute type code against 3",
        "reads or creates value and channel fields",
        "uses expression text lerp(value, lo, hi), lo/hi bounds, and an expr field",
        "uses scaled-channel and rescale-expression name formats",
        "writes result and defaultValue fields and uses the supplied numeric bounds",
    ],
    "limitations": "Exact type naming, UI widget behavior, complete parent/bin structure, ownership, undo integration, and runtime behavior are not established by these observations.",
}

# Camera property setup creates a missing `value` field through the shared
# native attribute-creation routine. Verify both sides of that call and the
# callee's conditional submission to the editor's undo-record intake slot.
# This is static evidence that a creation path can emit a record; it does not
# prove that this branch is taken in every runtime state or that replay fully
# reverses camera creation.
attribute_create_call_va = 0x107269AC
attribute_create_call_bytes = bytes.fromhex("E8 3F AB C3 FF")
attribute_call_offset = attribute_create_call_va - control_builder_start
if control_builder[attribute_call_offset:attribute_call_offset + 5] != attribute_create_call_bytes:
    raise AssertionError("camera value-field creation call bytes changed")
if attribute_create_call_va + 5 + int.from_bytes(attribute_create_call_bytes[1:], "little", signed=True) != 0x103614F0:
    raise AssertionError("camera value-field creation target changed")
if control_builder[0x107269A3 - control_builder_start:0x107269A3 - control_builder_start + 14] != bytes.fromhex(
    "6A 03 68 08 E0 A8 10 8B CE E8 3F AB C3 FF"
):
    raise AssertionError("camera value-field creation arguments changed")

attribute_creation_start, attribute_creation_end = 0x103614F0, 0x103615D5
attribute_creation = decode_span(attribute_creation_start, attribute_creation_end)
for va, expected in [
    (0x10361570, "8B 82 F8 00 00 00"),  # controller's record-allowed service slot
    (0x10361577, "FF D0"),              # ask whether this owner can be recorded
    (0x10361579, "84 C0 74 2E"),        # skip record allocation if rejected
    (0x10361591, "E8 DA CA FF FF"),     # construct record object
    (0x103615A3, "8B 82 3C 01 00 00 FF D0"),  # controller slot +0x13C record intake
]:
    actual = attribute_creation[va - attribute_creation_start:va - attribute_creation_start + len(bytes.fromhex(expected))]
    if actual != bytes.fromhex(expected):
        raise AssertionError(f"attribute-creation history bytes changed at {va:#x}")
attribute_undo_record_evidence = {
    "camera_caller": {
        "helper_start": hex(control_builder_start),
        "call_site": hex(attribute_create_call_va),
        "call_target": hex(attribute_creation_start),
        "field_name": "value",
        "requested_type_code": 3,
        "call_and_arguments_bytes_verified": True,
    },
    "callee": {
        "start": hex(attribute_creation_start),
        "end_exclusive": hex(attribute_creation_end),
        "byte_length": len(attribute_creation),
        "record_gate_slot": "+0xF8",
        "record_constructor": "0x1035E070",
        "record_submission_slot": "+0x13C",
        "record_submission_sequence_bytes_verified": True,
    },
    "conclusion": "The camera property setup's missing-value-field path calls the generic attribute creator; that routine conditionally constructs and submits an attribute record through the controller's +0x13C intake when its +0xF8 eligibility gate allows it.",
    "limitations": "Static routing only. Whether the gate allows recording in a particular session, whether the record is included in the active Camera creation group, what exact replay mutates, and whether undo/redo fully reverses creation remain unverified.",
}

# Verify the following helper's two transform-control labels and loop. The
# helper is shared, so this records its observed structure without assigning
# undocumented meanings to every call it makes.
transform_helper_start = 0x10726D40
transform_helper_end = 0x10726F3C
transform_helper = decode_span(transform_helper_start, transform_helper_end)
transform_labels = [im.word(0x10EFF6AC), im.word(0x10EFF6B0)]
expected_transform_labels = [(0x10AA5858, "pos"), (0x10AA585C, "rot")]
if transform_labels != [va for va, _ in expected_transform_labels]:
    raise AssertionError(f"unexpected transform label table: {transform_labels!r}")
for va, expected in expected_transform_labels:
    if im.string(va) != expected:
        raise AssertionError(f"transform label at {va:#x} does not match {expected!r}")
if im.string(0x10ACFA08) != "%s_%s" or b"\x68\x08\xFA\xAC\x10" not in transform_helper:
    raise AssertionError("transform helper no longer uses the expected generated-name format")
for va, expected in [
    (0x10726DF6, "8B 0C B5 AC F6 EF 10"),
    (0x10726E3B, "E8 D0 F8 FF FF"),
    (0x10726EEC, "83 FE 02"),
]:
    offset = va - transform_helper_start
    if transform_helper[offset:offset + len(bytes.fromhex(expected))] != bytes.fromhex(expected):
        raise AssertionError(f"transform helper bytes changed at {va:#x}")

transform_helper_evidence = {
    "start": hex(transform_helper_start),
    "end_exclusive": hex(transform_helper_end),
    "byte_length": len(transform_helper),
    "loop_count": 2,
    "labels": [{"va": hex(va), "value": label} for va, label in expected_transform_labels],
    "generated_name_format": {"va": hex(0x10ACFA08), "value": "%s_%s"},
    "observed_operations": [
        "iterates over the pos and rot label table twice",
        "formats a generated name from two string values",
        "passes each generated entry through helper 0x10726710",
    ],
    "limitations": "The complete transform DMX hierarchy, helper ownership semantics, transform math, UI behavior, and runtime results remain unverified.",
}

if 0x10726E3B + 5 + int.from_bytes(bytes.fromhex("D0 F8 FF FF"), "little", signed=True) != 0x10726710:
    raise AssertionError("transform-control call target is not 0x10726710")
shared_entry_builder_evidence = {
    "target": "0x10726710",
    "numeric_control_call_site": hex(numeric_entry_call_va),
    "transform_control_call_site": "0x10726E3B",
    "statement": "the numeric-control helper calls this entry builder once per processed property; the transform helper calls it once per pos/rot iteration",
    "limitations": "The called entry builder's full ownership/undo semantics are not reconstructed here.",
}

# The native call at 0x10723E50 is conditionally reached when a channel is
# created with scale metadata. Check its full executable span and the key loop
# before describing it as channel-value rescaling; it is not treated as an
# undo/transaction implementation.
rescale_helper_start = 0x10723E50
rescale_helper_end = 0x10723F65
rescale_helper = decode_span(rescale_helper_start, rescale_helper_end)
for va, expected in [
    (0x10723E70, "E8 0B 8A D9 FF"),
    (0x10723E7A, "E8 01 8A D9 FF"),
    (0x10723EF4, "51"),
    (0x10723F07, "E8 94 2F DF FF"),
    (0x10723F23, "FF D2"),
    (0x10723F57, "FF D0"),
    (0x10723F5A, "3B F3 7C B6"),
]:
    offset = va - rescale_helper_start
    code = bytes.fromhex(expected)
    if rescale_helper[offset:offset + len(code)] != code:
        raise AssertionError(f"channel rescale helper bytes changed at {va:#x}")

rescale_helper_evidence = {
    "start": hex(rescale_helper_start),
    "end_exclusive": hex(rescale_helper_end),
    "byte_length": len(rescale_helper),
    "observed_operations": [
        "reads float values from two inputs",
        "computes differences and a float scale factor",
        "iterates the target channel's entries and writes adjusted float values",
    ],
    "interpretation": "channel-value rescaling is supported by the float math, per-entry update loop, and caller-generated scaled/rescale channel names",
    "limitations": "The exact mathematical contract and undo/transaction semantics are not recovered.",
}

# Cross-check the editor-operation service slots used by Camera/Light against
# two generic operation-scope helpers in ifm.dll. Keep this as an interface
# correlation only: neither these wrappers nor the slot numbers establish
# which undo records are emitted or how replay behaves.
operation_setup_start, operation_setup_end = 0x10001500, 0x10001558
operation_cleanup_start, operation_cleanup_end = 0x10001560, 0x100015B8
operation_setup = decode_span(operation_setup_start, operation_setup_end)
operation_cleanup = decode_span(operation_cleanup_start, operation_cleanup_end)
for va, expected in [
    (0x1000152D, "8B 82 08 01 00 00"),  # service vtable slot +0x108
    (0x10001534, "FF D0"),              # invoke operation setup
    (0x10001541, "8B 92 E4 01 00 00"), # follow-up service slot +0x1E4
    (0x10001571, "8B 90 0C 01 00 00"), # service vtable slot +0x10C
    (0x10001577, "FF D2"),              # invoke operation completion
]:
    span = operation_setup if va < operation_setup_end else operation_cleanup
    start = operation_setup_start if va < operation_setup_end else operation_cleanup_start
    code = bytes.fromhex(expected)
    if span[va - start:va - start + len(code)] != code:
        raise AssertionError(f"operation-scope service dispatch bytes changed at {va:#x}")
operation_scope_service_evidence = {
    "setup_helper": {"start": hex(operation_setup_start), "end_exclusive": hex(operation_setup_end), "byte_length": len(operation_setup)},
    "cleanup_helper": {"start": hex(operation_cleanup_start), "end_exclusive": hex(operation_cleanup_end), "byte_length": len(operation_cleanup)},
    "service_slots": {"begin": "0x108", "complete": "0x10C", "follow_up_during_setup": "0x1E4"},
    "observed_operations": [
        "the setup helper invokes the service begin slot and then a separate setup-related service slot",
        "the cleanup helper invokes the service completion slot when its state check allows it",
        "the Camera and Light creator callbacks invoke the same begin and completion service slots with their own named labels",
    ],
    "limitations": "This correlates operation-scope service dispatch in several callbacks and helpers; slot implementations, undo record contents, rollback/cancel semantics, and runtime undo/redo behavior remain unverified.",
}

# Resolve the controller's actual file-backed vtable slots. This closes the
# gap between observing +0x108/+0x10C call sites and knowing which native
# routines those calls reach. The concrete group routines are also verified by
# the dedicated undo-group begin/end recovery artifacts.
controller_global = 0x10E9A51C
controller_object = 0x10F3D770
controller_vtable = 0x10B37094
if im.word(controller_global) != controller_object:
    raise AssertionError("SFM editor-controller global changed")
controller_slots = {
    "operation_begin": im.word(controller_vtable + 0x108),
    "operation_complete": im.word(controller_vtable + 0x10C),
    "operation_metadata_begin": im.word(controller_vtable + 0x1E4),
}
expected_controller_slots = {
    "operation_begin": 0x10353C50,
    "operation_complete": 0x10353C60,
    "operation_metadata_begin": 0x10353BA0,
}
if controller_slots != expected_controller_slots:
    raise AssertionError(f"controller operation slots changed: {controller_slots!r}")
operation_begin_thunk = decode_span(0x10353C50, 0x10353C5F)
operation_complete_thunk = decode_span(0x10353C60, 0x10353C81)
operation_metadata_thunk = decode_span(0x10353BA0, 0x10353BAF)
for span, start, expected_target in [
    (operation_begin_thunk, 0x10353C50, 0x103906F0),
    (operation_metadata_thunk, 0x10353BA0, 0x103906C0),
]:
    call_offset = span.index(b"\xE9")
    actual_target = start + call_offset + 5 + int.from_bytes(span[call_offset + 1:call_offset + 5], "little", signed=True)
    if actual_target != expected_target:
        raise AssertionError(f"controller operation thunk at {start:#x} targets {actual_target:#x}")
if b"\xE8\x12\xD4\x03\x00" not in operation_complete_thunk:
    raise AssertionError("operation-completion wrapper no longer calls group-finalization helper")
operation_controller_vtable_evidence = {
    "controller_global": hex(controller_global),
    "controller_object": hex(controller_object),
    "controller_vtable": hex(controller_vtable),
    "slots": {key: hex(value) for key, value in controller_slots.items()},
    "resolved_targets": {
        "operation_begin": "0x103906F0 (undo-group begin routine; independently byte-checked)",
        "operation_complete": "0x10391080 (undo-group completion routine; independently byte-checked)",
        "operation_metadata_begin": "0x103906C0 (separate depth/metadata routine)",
    },
    "observed_operations": [
        "the editor-controller vtable's +0x108 slot thunks to the same native routine independently recovered as undo-group begin",
        "the +0x10C slot's wrapper calls the same native routine independently recovered as undo-group completion",
        "the Camera and Light callbacks dispatch through those exact controller slots using their named labels",
    ],
    "limitations": "This establishes static routing from Camera/Light operation scopes into the native grouped-history begin/end routines. It does not show which records creation emits, prove those records reverse every creation mutation, or verify live undo/redo behavior.",
}

for action in actions:
    code = decode_span(*action["wrapper"])
    label = im.string(action["label_va"])
    if label != action["label"]:
        raise AssertionError(f"menu label mismatch for command {action['command_id']:#x}")
    calls = [
        action["wrapper"][0] + offset + 5 + int.from_bytes(code[offset + 1:offset + 5], "little", signed=True)
        for offset in range(max(0, len(code) - 4)) if code[offset] == 0xE8
    ]
    if action["delegated_creator"] not in calls:
        raise AssertionError(f"action {action['label']} does not call expected creator")
    if action["creator_name_va"] is not None:
        name = im.string(action["creator_name_va"])
        if not name.startswith("CreateAnimationSetAnd"):
            raise AssertionError(f"unexpected creator name {name}")
    else:
        name = None
    action["wrapper_bytes"] = len(code)
    action["wrapper_start"] = hex(action["wrapper"][0])
    action["wrapper_end_exclusive"] = hex(action["wrapper"][1])
    action["delegated_creator"] = hex(action["delegated_creator"])
    action["label_va"] = hex(action["label_va"])
    action["command_id"] = hex(action["command_id"])
    action["creator_name"] = name
    del action["wrapper"]

receipt = {
    "dll": str(binary),
    "sha256": digest,
    "actions": actions,
    "creator_evidence": creator_evidence,
    "camera_helper_evidence": camera_helper_evidence,
    "shared_control_builder_evidence": control_builder_evidence,
    "camera_attribute_undo_record_evidence": attribute_undo_record_evidence,
    "transform_helper_evidence": transform_helper_evidence,
    "shared_entry_builder_evidence": shared_entry_builder_evidence,
    "rescale_helper_evidence": rescale_helper_evidence,
    "operation_scope_service_evidence": operation_scope_service_evidence,
    "operation_controller_vtable_evidence": operation_controller_vtable_evidence,
    "recovered": "Animation Set Editor commands 0x1B9-0x1BC use separate callbacks. Camera and Light creator byte ranges reference Cameras/Lights category strings and camera%d/light%d name templates. Camera calls helper 0x1072CF10 and branches on its returned pointer: non-null goes to 0x102236AE; null goes to cleanup at 0x10223642. Light does the analogous check after helper 0x1072D020: non-null goes to 0x102238EA; null goes to cleanup at 0x1022387E. Their named scopes dispatch through editor-controller vtable slots +0x108/+0x10C, statically resolved to the independently recovered grouped-history begin/end routines 0x103906F0/0x10391080. The camera control builder creates a missing value attribute through 0x103614F0; that routine conditionally constructs and submits an attribute record through controller slot +0x13C when slot +0xF8 allows it. Actual gate state, group membership/replay behavior, broader graph mutation, and UI semantics remain unresolved.",
    "limitations": "Static code and string evidence only. The meaning of the helpers' return values, complete graph mutations, whether null corresponds to user cancellation, which undo records creation emits and whether replay reverses all changes, and runtime behavior have not yet been recovered or exercised; no prototype action implementation is claimed.",
}
Path("outputs/animation-set-creation-callbacks-recovery.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps(receipt, indent=2))
