import hashlib
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image


BINARY = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
DISASSEMBLY = Path("work/ifm-disassembly.txt")
REGISTRATION = Path("outputs/animation-set-hierarchy-context-menu-verification.json")
OUTPUT = Path("outputs/animation-set-group-selected-dags-handler-recovery.json")
START = 0x1008ED10
END = 0x1008EE33
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
HELPERS = {
    "0x10083de0": (0x10083DE0, 0x10083E76, []),
    "0x10083fe0": (0x10083FE0, 0x10084069, [0x10335830, 0x10083DE0]),
    "0x1008c2f0": (0x1008C2F0, 0x1008C334, [0x10335830, 0x10377900, 0x10377900]),
    "0x1008cec0": (0x1008CEC0, 0x1008CF1E, []),
    "0x1008e2e0": (0x1008E2E0, 0x1008E360, [0x104C2B00, 0x104C2E70, 0x104D63D0, 0x104D63D0, 0x1008D510, 0x1008E200, 0x1008D510]),
    "0x1008e3d0": (0x1008E3D0, 0x1008E420, [0x1008E2E0]),
    "0x104d63d0": (0x104D63D0, 0x104D6431, [0x10083FE0, 0x104C65C0, 0x10083FE0, 0x104C2B50]),
    "0x104c27f0": (0x104C27F0, 0x104C2815, [0x10335830, 0x101B5110]),
    "0x104c2b00": (0x104C2B00, 0x104C2B07, []),
    "0x104c2e70": (0x104C2E70, 0x104C2EC6, []),
    "0x104c2770": (0x104C2770, 0x104C27AC, [0x10335830, 0x10377870, 0x10377870]),
    "0x104c5ef0": (0x104C5EF0, 0x104C5F2A, [0x104C4300, 0x104C2770]),
    "0x104c2450": (0x104C2450, 0x104C245F, []),
}


def checked_disassembly_spans(image, spans):
    spans = sorted((start, end, name) for name, (start, end) in spans.items())
    rows = {name: [] for _, _, name in spans}
    range_index = 0
    current = None
    first_line = re.compile(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)$")
    continuation = re.compile(r"^\s{10,}([0-9A-F]{2}(?:\s+[0-9A-F]{2})*)\s*$")

    def store_current():
        nonlocal range_index
        if not current:
            return
        va = current[0]
        while range_index < len(spans) and va >= spans[range_index][1]:
            range_index += 1
        if range_index < len(spans):
            start, end, name = spans[range_index]
            if start <= va < end:
                rows[name].append(current)

    for line in DISASSEMBLY.open(errors="replace"):
        match = first_line.match(line)
        if match:
            store_current()
            va = int(match.group(1), 16)
            current = [va, bytearray.fromhex(match.group(2)), match.group(3).strip()]
        elif current:
            match = continuation.match(line)
            if match:
                current[1].extend(bytes.fromhex(match.group(1)))
    store_current()

    recovered_by_name = {}
    instructions_by_name = {}
    for start, end, name in spans:
        cursor = start
        recovered = bytearray()
        instructions = []
        for va, raw, text in rows[name]:
            if va != cursor:
                raise ValueError(f"{name}: non-contiguous disassembly at {va:#x}; expected {cursor:#x}")
            disk_bytes = image.b[image.offset(va):image.offset(va) + len(raw)]
            if bytes(raw) != disk_bytes:
                raise ValueError(f"{name}: disassembly differs from installed bytes at {va:#x}")
            recovered.extend(raw)
            cursor += len(raw)
            instructions.append({"va": va, "bytes": bytes(raw), "text": text})
        if cursor != end:
            raise ValueError(f"{name}: span ended at {cursor:#x}; expected {end:#x}")
        recovered_by_name[name] = bytes(recovered)
        instructions_by_name[name] = instructions
    return recovered_by_name, instructions_by_name


def main():
    image = Image(BINARY)
    digest = hashlib.sha256(image.b).hexdigest()
    if digest != EXPECTED_SHA256:
        raise ValueError(f"unexpected installed ifm.dll hash: {digest}")

    registration = json.loads(REGISTRATION.read_text(encoding="utf-8"))
    if registration.get("binary", "").lower() != str(BINARY).lower():
        raise ValueError("hierarchy context-menu audit targets another binary")
    if registration.get("sha256") != digest or not registration.get("all_match"):
        raise ValueError("hierarchy action registration audit is absent or stale")

    span_specs = {"group_callback": (START, END)}
    span_specs.update({label: (bounds[0], bounds[1]) for label, bounds in HELPERS.items()})
    span_bytes, span_instructions = checked_disassembly_spans(image, span_specs)
    raw = span_bytes["group_callback"]
    instructions = span_instructions["group_callback"]
    direct_calls = []
    branches = []
    for instruction in instructions:
        text = instruction["text"]
        call = re.search(r"\bcall\s+([0-9A-F]{8})\b", text, re.IGNORECASE)
        if call:
            direct_calls.append(int(call.group(1), 16))
        branch = re.search(r"\b(j[a-z]+)\s+([0-9A-F]{8})\b", text, re.IGNORECASE)
        if branch:
            branches.append({
                "va": f"{instruction['va']:#010x}",
                "mnemonic": branch.group(1).lower(),
                "target": f"{int(branch.group(2), 16):#010x}",
            })

    expected_calls = [
        0x104C2B50, 0x104C27F0, 0x1008CEC0, 0x104C2440, 0x1035FC60,
        0x104C2B00, 0x104C2E70, 0x104C2440, 0x1008C2F0, 0x104C5EF0,
        0x1008E3D0, 0x104C2450, 0x104C5EF0,
    ]
    expected_branches = [
        ("je", 0x1008EE2C), ("je", 0x1008EE2B), ("js", 0x1008EE2B),
        ("jne", 0x1008ED97), ("jle", 0x1008EE1B), ("je", 0x1008EE14),
        ("jne", 0x1008EDF1), ("jmp", 0x1008EE14), ("jne", 0x1008EE14),
        ("jl", 0x1008EDC0),
    ]
    observed_branches = [(item["mnemonic"], int(item["target"], 16)) for item in branches]
    action_label = image.string(0x10AA6AEC)
    new_group_name = image.string(0x10AA7894)
    mask_offset = image.offset(0x10AA73B8)
    mask_words = list(struct.unpack_from("<8I", image.b, mask_offset))
    expected_mask_words = [0, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xE, 0x2, 0x4]
    mode_table_offset = image.offset(0x10083E78)
    typed_lookup_mode_targets = list(struct.unpack_from("<4I", image.b, mode_table_offset))
    expected_mode_targets = [0x10083E33, 0x10083E26, 0x10083E19, 0x10083E01]
    runtime_type_marker = image.word(0x10EB131C)
    if direct_calls != expected_calls:
        raise ValueError(f"direct-call sequence mismatch: {[hex(value) for value in direct_calls]}")
    if observed_branches != expected_branches:
        raise ValueError(f"branch sequence mismatch: {observed_branches}")
    if action_label != "Group Selected Dags" or new_group_name != "group1":
        raise ValueError(f"unexpected action/group labels: {action_label!r}, {new_group_name!r}")
    if mask_words != expected_mask_words:
        raise ValueError(f"kind-mask table excerpt mismatch: {[hex(value) for value in mask_words]}")
    if typed_lookup_mode_targets != expected_mode_targets:
        raise ValueError(f"typed lookup mode table mismatch: {[hex(value) for value in typed_lookup_mode_targets]}")

    helper_records = {}
    for label, (helper_start, helper_end, expected_helper_calls) in HELPERS.items():
        helper_bytes = span_bytes[label]
        helper_instructions = span_instructions[label]
        helper_text = [instruction["text"] for instruction in helper_instructions]
        helper_calls = []
        for instruction in helper_instructions:
            call = re.search(r"\bcall\s+([0-9A-F]{8})\b", instruction["text"], re.IGNORECASE)
            if call:
                helper_calls.append(int(call.group(1), 16))
        if helper_calls != expected_helper_calls:
            raise ValueError(f"helper {label} direct-call mismatch: {[hex(value) for value in helper_calls]}")
        if label == "0x1008e2e0":
            ordered_landmarks = [
                "call        104C2B00",
                "cmp         eax,1",
                "call        104C2E70",
                "call        104D63D0",
                "cmp         ecx,dword ptr [eax+14h]",
                "cmp         byte ptr [esi+00000088h],0",
            ]
            cursor = -1
            for landmark in ordered_landmarks:
                try:
                    cursor = next(i for i in range(cursor + 1, len(helper_text)) if landmark.lower() in helper_text[i].lower())
                except StopIteration as exc:
                    raise ValueError(f"descriptor builder is missing/out of order at {landmark!r}") from exc
            for landmark in ("call        1008D510", "call        1008E200"):
                if not any(landmark.lower() in line.lower() for line in helper_text):
                    raise ValueError(f"descriptor builder is missing controller lookup {landmark!r}")
        elif label == "0x104d63d0":
            ordered_landmarks = [
                "call        10083FE0",
                "call        104C65C0",
                "call        10083FE0",
                "call        104C2B50",
                "jne         104D63E0",
            ]
            cursor = -1
            for landmark in ordered_landmarks:
                try:
                    cursor = next(i for i in range(cursor + 1, len(helper_text)) if landmark.lower() in helper_text[i].lower())
                except StopIteration as exc:
                    raise ValueError(f"resolver walk is missing/out of order at {landmark!r}") from exc
        elif label == "0x10083fe0":
            ordered_landmarks = [
                "call        10335830",
                "mov         edx,dword ptr [esi+000001CCh]",
                "call        edx",
                "call        10083DE0",
                "mov         eax,dword ptr [edx+000001D0h]",
                "call        eax",
            ]
            cursor = -1
            for landmark in ordered_landmarks:
                try:
                    cursor = next(i for i in range(cursor + 1, len(helper_text)) if landmark.lower() in helper_text[i].lower())
                except StopIteration as exc:
                    raise ValueError(f"typed lookup path is missing/out of order at {landmark!r}") from exc
        elif label == "0x10083de0":
            for landmark in (
                "cmp         eax,3",
                "ja          10083E01",
                "mov         ecx,dword ptr ds:[10EB1AC4h]",
                "call        eax",
                "cmp         dword ptr [edi+14h],eax",
            ):
                if not any(landmark.lower() in line.lower() for line in helper_text):
                    raise ValueError(f"typed lookup mode body is missing {landmark!r}")
        helper_records[label] = {
            "end_exclusive": f"{helper_end:#010x}",
            "bytes": len(helper_bytes),
            "instruction_records": len(helper_instructions),
            "direct_call_targets": [f"{value:#010x}" for value in helper_calls],
            "verified_instruction_text": helper_text,
        }

    body = {
        "binary": str(BINARY),
        "sha256": digest,
        "method": "Animation Set Editor Group Selected Dags callback",
        "registration_audit": str(REGISTRATION),
        "function_span": {
            "start": f"{START:#010x}",
            "end_exclusive": f"{END:#010x}",
            "bytes": len(raw),
            "disassembly_matches_installed_bytes": True,
            "instruction_records": len(instructions),
        },
        "verified_strings": {
            "context_action": {"va": "0x10aa6aec", "value": action_label},
            "new_group_default_name": {"va": "0x10aa7894", "value": new_group_name},
        },
        "classifier_data": {
            "kind_mask_table_excerpt": {
                "va": "0x10aa73b8",
                "verified_dwords": [f"{value:#010x}" for value in mask_words],
                "domain_bound": False,
            },
            "typed_lookup_mode_table": {
                "va": "0x10083e78",
                "verified_targets": [f"{value:#010x}" for value in typed_lookup_mode_targets],
            },
            "object_factory_type_marker_storage": {
                "va": "0x10eb131c",
                "on_disk_dword": f"{runtime_type_marker:#010x}",
                "symbolic_meaning": "unresolved; runtime initialization not established",
            },
        },
        "direct_call_targets": [f"{value:#010x}" for value in direct_calls],
        "conditional_and_loop_branches": branches,
        "helper_spans": helper_records,
        "static_observations": [
            "A null callback argument returns without entering the mutation path.",
            "The callback resolves a selection/context object and returns when resolution fails or the selection index is negative.",
            "It begins a named operation using the exact Group Selected Dags string, then asks helper 0x1008CEC0 to create a new object using default name group1.",
            "If object creation fails, it calls the same global service through vtable offset 0x10c before returning.",
            "The success path obtains a collection count and iterates collection entries; one branch compares an entry with the callback argument, while another calls helper 0x1008E3D0 and branches on result 2.",
            "The loop calls 0x1008C2F0, 0x104C2450, and 0x104C5EF0 on distinct paths, then exits through the same global service vtable offset 0x10c.",
            "The selected collection stores its vector at object offset 0x84 and its count at offset 0x90; the count accessor returns that field, and the indexed accessor checks signed bounds before resolving and type-checking the stored handle.",
            "The indexed-write helper converts a nonnull object to a handle before forwarding the index/value pair to 0x10377900; for a null value it forwards the invalid-handle sentinel 0xffffffff.",
            "The child-container append helper rejects null, self, and already-present entries, otherwise it delegates to the vector append path.",
            "The group callback replaces the target collection slot with the new group's handle, appends the target to the new group's child container, and removes other entries whose 0x1008E3D0 classification is exactly 2 before appending them; that numeric classification is not assigned a guessed type name.",
            "The object-creation wrapper calls the global service at virtual offset 0x40, resolves the returned handle through offset 0x48, and returns the resolved object only if virtual offset 0x10 accepts the supplied runtime descriptor; that descriptor's symbolic type is unresolved.",
            "On the classifier path with kind index >= 4, 0x1008E3D0 returns 0 for a missing descriptor/flag object, 1 for no overlap with the per-kind mask table at 0x10AA73B8, 2 when a nonzero candidate flag set exactly equals the mask, and 3 for a nonzero partial overlap. Kind indices below 4 return the descriptor's field at offset +8 directly, so the Group Selected Dags comparison to 2 cannot yet be assigned one universal semantic. The mask entries' user-facing meanings remain unknown.",
            "Descriptor builder 0x1008E2E0 inspects the input object's child count at +0x90 and resolves child zero through 0x104D63D0 when the count is one; it compares the input object's +0x14 field with the resolved object's +0x14 field, otherwise it resolves the input itself. It then selects controller-provided descriptors through 0x1008D510 or 0x1008E200 according to a controller byte at +0x88.",
            "Resolver 0x104D63D0 calls typed lookup helper 0x10083FE0 in mode 0 with an invalid-handle sentinel. The helper dispatches through a four-entry mode table, queries a service relation, applies a runtime type test using descriptor storage at 0x10EB1AC4, and can follow the service's next-relation method. If that lookup fails, the resolver asks 0x104C65C0 for its audited DmeTransformControl candidate and passes the candidate to the same typed lookup; it then follows 0x104C2B50 to the ordinary owning DAG and repeats. The descriptor's symbolic name and the grouping predicate's exact semantic category remain unresolved.",
        ],
        "limitations": [
            "The identities and full semantics of the selection, type-test, collection-mutation, and transaction helpers are not inferred from call addresses alone.",
            "The exact resulting DAG topology, transform preservation, undo behavior, and notification behavior still need independent recovery or original-app observation.",
            "This is static installed-byte recovery, not a claim that the x64 recreation implements this action.",
        ],
        "all_checks_match": True,
    }
    OUTPUT.write_text(json.dumps(body, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "sha256": digest,
        "function_bytes": len(raw),
        "instruction_records": len(instructions),
        "direct_calls": len(direct_calls),
        "branches": len(branches),
        "helper_spans": len(helper_records),
        "context_action": action_label,
        "default_group_name": new_group_name,
        "all_checks_match": True,
        "limitations": body["limitations"],
    }, indent=2))


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"group-selected-dags handler recovery failed: {error}", file=sys.stderr)
        raise
