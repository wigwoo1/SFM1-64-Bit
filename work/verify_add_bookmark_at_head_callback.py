"""Verify the Timestrip Add Bookmark action's registered callback path."""
import hashlib
import json
import pathlib
import re
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pe_image import Image

ROOT = pathlib.Path(__file__).resolve().parents[1]
TARGET = pathlib.Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
DISASSEMBLY = ROOT / "work" / "ifm-disassembly.txt"
image = Image(TARGET)
digest = hashlib.sha256(image.b).hexdigest()
if digest != EXPECTED_SHA256:
    raise SystemExit(f"installed ifm.dll changed: {digest}")


def raw(va, size):
    offset = image.offset(va)
    return image.b[offset:offset + size]


def load_disassembly_index():
    index = {}
    with DISASSEMBLY.open("r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            match = re.match(r"\s+([0-9A-Fa-f]{8}):\s+((?:[0-9A-Fa-f]{2}(?:\s+|$))+)", line)
            if match:
                index[int(match.group(1), 16)] = bytes.fromhex(match.group(2))
    return index


disassembly_index = load_disassembly_index()


def verify_span(va, expected_hex, label):
    expected = bytes.fromhex(expected_hex)
    actual = raw(va, len(expected))
    if actual != expected:
        raise SystemExit(f"{label} mismatch at {va:#x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual), "label": label}


def verify_function(start, end):
    rows = []
    expected = start
    # A few long instruction byte strings wrap to address-less lines in the
    # source disassembly. Keep their trailing byte addresses explicit here.
    wrapped_by_function = {
        0x1017F0E0: {0x1017F10B: b"\xFF", 0x1017F11F: b"\x00"},
        0x10377870: {0x10377890: b"\x10"},
        0x1029A950: {0x1029A96E: b"\x00", 0x1029A9DB: b"\x00", 0x1029A9EA: b"\x00", 0x1029A9F3: b"\x00", 0x1029A9FC: b"\x00"},
        0x10295800: {0x10295810: b"\xAB\x10"},
    }
    wrapped = wrapped_by_function.get(start, {})
    while expected < end:
        code = disassembly_index.get(expected)
        if code is None:
            tail = wrapped.get(expected)
            if tail is None or raw(expected, len(tail)) != tail:
                raise SystemExit(f"disassembly byte missing/mismatched at {expected:#x}")
            expected += len(tail)
            continue
        if raw(expected, len(code)) != code:
            raise SystemExit(f"disassembly/in-place binary mismatch at {expected:#x}")
        rows.append((expected, code))
        expected += len(code)
    calls = []
    for va, code in rows:
        if len(code) == 5 and code[0] == 0xE8:
            target = va + 5 + struct.unpack("<i", code[1:])[0]
            calls.append({"site": f"0x{va:08x}", "target": f"0x{target:08x}"})
    return rows, calls


registration = json.loads((ROOT / "outputs" / "graph-editor-bookmark-command-registration-recovery.json").read_text(encoding="utf-8"))
matches = [entry for entry in registration["actions"] if entry["command_key"] == "AddBookmarkAtHead"]
if len(matches) != 1 or matches[0]["display_label"] != "Add Bookmark at Head" or matches[0]["callback_va"].lower() != "0x100b1570":
    raise SystemExit("bookmark action registration does not resolve uniquely to expected callback")

handler_rows, handler_calls = verify_function(0x100B1570, 0x100B15E3)
add_rows, add_calls = verify_function(0x100B1490, 0x100B156D)
lookup_rows, lookup_calls = verify_function(0x1017F0E0, 0x1017F1C6)
handle_rows, handle_calls = verify_function(0x10335830, 0x10335834)
append_rows, append_calls = verify_function(0x10377870, 0x1037789F)
completion_rows, completion_calls = verify_function(0x100B11A0, 0x100B11EF)
release_rows, release_calls = verify_function(0x100AFD70, 0x100AFD99)
time_query_rows, time_query_calls = verify_function(0x1029A950, 0x1029AA1A)
time_convert_rows, time_convert_calls = verify_function(0x10295800, 0x10295854)
handler_targets = {int(call["target"], 16) for call in handler_calls}
add_targets = {int(call["target"], 16) for call in add_calls}
for address in [0x1029A950, 0x100A75D0, 0x10295800, 0x1097B020, 0x100B1490]:
    if address not in handler_targets:
        raise SystemExit(f"Add Bookmark callback missing expected call target {address:#x}")
for address in [0x1017F0E0, 0x100AFE70, 0x1017EF80, 0x10366370, 0x1035FC60, 0x10335830, 0x10377870, 0x100B11A0, 0x100AFD70]:
    if address not in add_targets:
        raise SystemExit(f"shared bookmark helper missing expected call target {address:#x}")
if image.string(0x10ACFB18) != "frameRate":
    raise SystemExit("time conversion setting string changed")

verified = [
    verify_span(0x100B1570, "55 8b ec a1 68 25 f3 10 83 ec 0c 56", "Add Bookmark at Head callback entry and time-service lookup"),
    verify_span(0x100B1585, "85 f6 74 54 8d 4d fc 51 8b ce e8 bc 93 1e 00", "time-service availability and time-state query"),
    verify_span(0x100B1594, "6a 00 e8 35 60 ff ff 83 c4 04 84 c0 74 30", "time query gate"),
    verify_span(0x100B15A2, "8d 55 f8 52 8b ce e8 53 42 1e 00", "time conversion input"),
    verify_span(0x100B15C1, "8d 45 f4 50 8d 4d fc e8 53 9a 8c 00 8b 08 89 4d fc", "convert time value before dispatch"),
    verify_span(0x100B15D2, "8b 55 fc 52 8b cf e8 b3 fe ff ff", "dispatch converted time to shared bookmark helper"),
    verify_span(0x100B1490, "55 8b ec 83 ec 08 56 57 8b f9", "shared Add Bookmark At Time helper entry"),
    verify_span(0x100B14A6, "85 f6 0f 84 b7 00 00 00 8b 4f 08 8b 01 8b 50 08 53 ff d2", "active bookmark-set/provider guards"),
    verify_span(0x100B14D8, "ff d2 33 c0 50 8b 45 08 50 53 56 e8 f8 db 0c 00", "Add Bookmark At Time operation dispatch"),
    verify_span(0x100B14EF, "8b 4e 18 8b 51 08 8b 42 38 6a 00 50 68 c8 bb aa 10 e8 6b e9 ff ff", "bookmark element creation path"),
    verify_span(0x100B1548, "e8 53 fc ff ff 8b c8 e8 1c e8 ff ff", "bookmark completion helpers"),
    verify_span(0x100B14D8, "ff d2 33 c0 50 8b 45 08 50 53 56 e8 f8 db 0c 00 83 c4 10 85 c0 75 65", "zero-tolerance existing-bookmark lookup and duplicate guard branch"),
    verify_span(0x1017F151, "8b 76 64 2b 75 f8 89 75 08 78 05 8d 45 08 eb 08 f7 de 89 75 f4 8d 45 f4 8b 00 3b 45 14 7f 06 89 7d fc 89 45 14", "absolute timestamp distance compared against caller tolerance"),
    verify_span(0x1017F17C, "8b 45 fc 85 c0 78 3a 8b 0b 8b 04 81", "return selected element only when a candidate satisfies tolerance"),
    verify_span(0x100B1515, "8b 10 8b 4f 68 83 c4 18 8d 45 fc 50 89 55 fc e8 47 4e 2b 00", "resolve value then pass it to the target element's typed attribute writer"),
    verify_span(0x103663CC, "0f b7 46 10 83 e0 1f 83 c0 fe 83 f8 05 77 33", "typed attribute writer dispatches by target type tag"),
    verify_span(0x103663E2, "83 3f 00 8b 56 04 0f 95 c1 88 0a", "boolean attribute conversion and write to target storage"),
    verify_span(0x103663EF, "66 0f 6e 07 8b 56 04 0f 5b c0 f3 0f 59 05 d4 df a8 10 f3 0f 11 02", "integer-backed value conversion and write to target storage"),
    verify_span(0x10366407, "8b 46 04 8b 0f 89 08", "default scalar payload copy into target storage"),
    verify_span(0x10366444, "8b 4d 08 51 8b 4e 08 e8 a0 8b ff ff 8b 0d 18 a5 e9 10 0f b6 c3 f7 d8 1b c0 25 00 f0 ff ff 05 00 20 00 00 09 81 80 c1 00 00", "attribute change callback and global invalidation flags"),
    verify_span(0x10335830, "8b 41 1c c3", "element handle accessor returns wrapper field at offset +0x1c"),
    verify_span(0x10377870, "55 8b ec 83 ec 0c 8b 41 18 8b 55 08 8b 49 0c 52 89 45 f8 8b 40 04 51 8d 4d f4 c7 45 f4 ac 7d b3 10 89 45 fc e8 77 5f ff ff 8b e5 5d c2 04 00", "append element handle through a reference-array wrapper"),
    verify_span(0x100B11A0, "a1 1c bc f2 10 57 33 ff 3b c7 75 41 56 6a 1c e8 cc d8 1a 00 8b f0 83 c4 04 3b f7 74 28 57 8b ce ff 15 88 b9 a7 10 c7 06 14 bb aa 10 89 7e 08 89 7e 0c 89 7e 10 89 7e 14 89 7e 18 8b c6 5e a3 1c bc f2 10 5f c3 33 c0 a3 1c bc f2 10 5e 5f c3", "lazy initialize singleton completion object"),
    verify_span(0x100AFD70, "56 57 8b f9 33 f6 39 77 14 7e 1b eb 03 8d 49 00 8b 47 08 8b 04 b0 8b 48 08 8b 11 8b 42 10 ff d0 46 3b 77 14 7c ea 5f 5e c3", "iterate completion object's reference list and invoke element release virtual slot"),
]

report = {
    "target": str(TARGET),
    "sha256": digest,
    "evidenceType": "direct installed x86 code-byte comparison plus verified action-registration receipt",
    "actionRegistration": {
        "commandKey": matches[0]["command_key"],
        "commandKeyVA": matches[0]["command_key_va"],
        "displayLabel": matches[0]["display_label"],
        "callback": matches[0]["callback_va"],
        "registrationReceipt": "outputs/graph-editor-bookmark-command-registration-recovery.json",
    },
    "callback": {
        "function": "0x100b1570",
        "verifiedBytes": sum(len(code) for _, code in handler_rows),
        "instructionCount": len(handler_rows),
        "directCalls": handler_calls,
        "observedBehavior": "Checks the time-service object, applies a gate, retrieves and converts a time-like value, then passes it to 0x100b1490.",
        "timeSemantics": "The callback queries a service-provided time-like integer, checks a separate operation gate, then converts a split value through a routine that reads the named frameRate setting before dispatch. The queried service branches on internal mode fields and a sentinel value. Static control flow does not identify the resulting value as the UI playhead or establish whether 'Head' means playhead, clip head, or another timeline clock.",
        "timeProviderQuery": {
            "function": "0x1029a950",
            "verifiedBytes": sum(len(code) for _, code in time_query_rows),
            "instructionCount": len(time_query_rows),
            "directCalls": time_query_calls,
            "observedBehavior": "Checks a global service gate, tests several mode fields and a 0x80000000 sentinel, and selects either a stored time-like field or a calculated/alternate path. The owning interface type and semantic meaning of its fields are unresolved.",
        },
        "frameRateConversion": {
            "function": "0x10295800",
            "verifiedBytes": sum(len(code) for _, code in time_convert_rows),
            "instructionCount": len(time_convert_rows),
            "directCalls": time_convert_calls,
            "settingName": image.string(0x10ACFB18),
            "observedBehavior": "Reads the frameRate setting with a default float fallback, then converts the supplied split numeric/time value through the downstream conversion helper. The conversion does not by itself identify which UI clock supplied the value.",
        },
    },
    "sharedBookmarkHelper": {
        "function": "0x100b1490",
        "verifiedBytes": sum(len(code) for _, code in add_rows),
        "instructionCount": len(add_rows),
        "directCalls": add_calls,
        "observedBehavior": "Queries the callback object's active bookmark-set/provider interfaces, invokes an operation labeled 'Add Bookmark At Time' with the supplied time, and enters the bookmark element creation/completion path when the operation gate allows it.",
        "downstreamCallTargets": [f"0x{x:08x}" for x in sorted(add_targets)],
        "duplicateTimestampGuard": {
            "function": "0x1017f0e0",
            # Include the two single-byte continuation bytes printed on
            # address-less wrapped lines in the source disassembly.
            "verifiedBytes": sum(len(code) for _, code in lookup_rows) + 2,
            "instructionCount": len(lookup_rows),
            "directCalls": lookup_calls,
            "observedBehavior": "Scans candidate elements, resolves valid element interfaces, compares each stored timestamp at offset +0x64 to the supplied time using absolute integer distance, and returns a matching element only when the distance is at most the caller tolerance. The Add Bookmark helper passes tolerance 0 and branches around creation when this lookup returns non-null; this statically establishes exact-timestamp duplicate suppression along this path.",
        },
        "creationAndValuePath": {
            "targetField": "element +0x68 (object returned by the 'bookmark' type resolver)",
            "typedAttributeWriter": {
                "function": "0x10366370",
                "evidence": "The caller passes the resolved one-DWORD source payload to this helper; the verified helper branches on the destination type tag, writes converted/copied data via the destination storage pointer at +0x4, and runs its change callback/invalidation path.",
                "interpretation": "Typed data-model attribute write. This does not independently prove the semantic name of the attribute or its persisted value after save/reopen.",
            },
            "collectionAppend": {
                "handleAccessor": "0x10335830",
                "appendHelper": "0x10377870",
                "appendHelperVerifiedBytes": sum(len(code) for _, code in append_rows) + 1,
                "observedBehavior": "The accessor returns a field at +0x1c from the created object. The wrapper uses element-handle reference-array vtable 0x10B37DAC and passes the handle plus current count to 0x1036D810; this matches the separately recovered control-children array append path.",
                "relatedRecovery": "outputs/control-children-array-append-recovery.json",
            },
            "completionCallbacks": {
                "singletonFunction": "0x100b11a0",
                "singletonVerifiedBytes": sum(len(code) for _, code in completion_rows),
                "releaseFunction": "0x100afd70",
                "releaseVerifiedBytes": sum(len(code) for _, code in release_rows),
                "observedBehavior": "The helper lazily allocates and initializes a 0x1c-byte singleton; the caller then iterates its reference array and invokes each entry's virtual +0x10 callback. The entry type and precise completion meaning remain unnamed.",
            },
        },
    },
    "verifiedRanges": verified,
    "limitations": [
        "The target is 32-bit x86; these bytes do not define the replacement's x64 ABI.",
        "The downstream data-model transaction and exact persisted bookmark result are not independently traced or runtime-tested here.",
        "The caption suggests a playhead/head-time command, but the exact meaning of the time service is not proven from the callback alone.",
        "No live menu selection, undo behavior, save/reopen result, or replacement UI screenshot was tested.",
    ],
    "result": "passed",
}
out = ROOT / "outputs" / "sfm64-timestrip-add-bookmark-callback-audit.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
print(f"verified Add Bookmark at Head callback ({len(handler_rows)} instructions) and shared helper ({len(add_rows)} instructions), plus {len(verified)} focused spans; sha256={digest}")
