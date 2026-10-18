"""Verify key paths in the installed Animation Set delete callback."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
expected_sha = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
disassembly = Path("work/ifm-disassembly.txt")
image = Image(binary)
digest = hashlib.sha256(image.b).hexdigest()
if digest != expected_sha:
    raise SystemExit(f"installed ifm.dll changed: {digest}")

def verify_span(va, hex_bytes, label):
    expected = bytes.fromhex(hex_bytes)
    actual = image.b[image.offset(va):image.offset(va) + len(expected)]
    if actual != expected:
        raise SystemExit(f"{label} mismatch at {va:#x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "length": len(actual), "bytes": actual.hex(), "label": label}

spans = [
    verify_span(0x1008E570, "55 8b ec 83 ec 10 53 56 57 8b f9 8b 0d 1c a5 e9 10 8b 01 8b 57 18 8b 40 48 52 89 7d fc ff d0 85 c0", "callback prologue and selection collection lookup"),
    verify_span(0x1008E597, "8b 0d 1c a5 e9 10 8b 11 8b 82 08 01 00 00 6a 00 68 40 78 aa 10 68 40 78 aa 10", "begin named Delete AnimationSets transaction"),
    verify_span(0x1008E5EF, "e8 4c d2 42 00 8b c8 8b 59 0c 4b 89 4d f8 79 34", "selection count and reverse iteration setup"),
    verify_span(0x1008E705, "56 e8 d5 d3 69 00 8b 4d f8 83 c4 04 53 e8 c9 9e 2f 00", "eligible-item operation dispatch"),
    verify_span(0x1008E72E, "8b 35 fc b8 a7 10 6a ff 68 08 78 aa 10 ff d6 6a ff 68 f4 77 aa 10", "current-camera refusal warning and Invalid Operation dialog"),
    verify_span(0x1008E7A7, "8b 0d 1c a5 e9 10 8b 11 8b 82 0c 01 00 00 ff d0", "batch transaction completion call"),
    verify_span(0x1072BAE0, "55 8b ec 83 ec 1c 53 56 57 8b 7d 08 33 f6 3b fe 0f 84 fa 01 00 00", "delete helper null guard and relationship processing setup"),
    verify_span(0x1072BAF6, "57 e8 44 c7 da ff 8b d8 83 c4 04 89 5d 08", "resolve the first supported associated-object relation"),
    verify_span(0x1072BB4C, "e8 af bc ea ff 8b 45 f0 83 c4 08 89 45 fc 85 c0 7e 16", "related-object list acquisition and count loop setup"),
    verify_span(0x1072BBC4, "8b 92 c8 00 00 00 68 d4 a7 a8 10 8d 45 f8 50 ff d2", "query the named children relation from the associated object"),
    verify_span(0x1072BB68, "57 e8 a2 aa ea ff 46 3b 75 fc 7c ea", "per-related-reference cleanup dispatch loop"),
    verify_span(0x1072BC10, "8b 87 a4 00 00 00 8b 04 98 8b 0d 1c a5 e9 10 8b 11 50 8b 42 48 ff d0", "resolve entries from the associated-object collection at plus 0xa4"),
    verify_span(0x1072BC62, "6a 01 53 e8 d6 3d c3 ff 6a 00 57 e8 ce 3d c3 ff", "associated-object notification and target notification dispatch"),
    verify_span(0x1072BC81, "8b 34 b8 85 f6 74 21 8b ce e8 e1 97 ea ff 84 c0 75 13", "dependent-object list cleanup decision"),
    verify_span(0x105D6610, "55 8b ec 8b 45 08 56 50 8b f1 e8 e1 ee ff ff 83 f8 ff 74 0c", "reference-list lookup and removal helper"),
    verify_span(0x105D6624, "50 8d 8e a0 00 00 00 e8 b0 1f db ff", "matched reference removed through collection helper"),
    verify_span(0x104D8240, "55 8b ec 56 8b 75 08 85 f6 75 05 33 c0 5e 5d c3 68 d8 f5 a8 10 8b ce e8 f4 04 d0 ff 85 c0 75 3c 68 50 e7 ac 10 8b ce e8 54 f7 ff ff 85 c0 75 2c 68 40 7b aa 10 8b ce e8 14 81 bb ff 85 c0 75 1c 68 a0 f2 ac 10 8b ce e8 e4 ae d0 ff 85 c0 75 0c 68 58 e7 ac 10 8b ce e8 d4 ae d0 ff 5e 5d c3", "ordered associated-object attribute lookup helper"),
]

calls = []
pattern = re.compile(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)$")
for line in disassembly.open(errors="replace"):
    match = pattern.match(line)
    if not match:
        continue
    va = int(match.group(1), 16)
    if not (0x1008E570 <= va < 0x1008E7F2):
        continue
    code = bytes.fromhex(match.group(2))
    if image.b[image.offset(va):image.offset(va) + len(code)] != code:
        raise SystemExit(f"disassembly/binary mismatch at {va:#x}")
    target = re.search(r"\bcall\s+(?:dword ptr\s+)?(10[0-9A-F]{6})h?\b", match.group(3), re.I)
    if target:
        calls.append({"site": f"0x{va:08x}", "target": f"0x{int(target.group(1),16):08x}", "verified_instruction_bytes": code.hex()})

strings = {
    "transaction": image.string(0x10AA7840),
    "cameraWarning": image.string(0x10AA7808),
    "dialogTitle": image.string(0x10AA77F4),
    "childrenAttribute": image.string(0x10A8A7D4),
    "associatedObjectAttributes": [image.string(address) for address in (0x10A8F5D8, 0x10ACE750, 0x10AA7B40, 0x10ACF2A0, 0x10ACE758)],
}
if strings != {"transaction": "Delete AnimationSets", "cameraWarning": "Can't delete the animationset for the current camera!", "dialogTitle": "Invalid Operation", "childrenAttribute": "children", "associatedObjectAttributes": ["camera", "light", "gameModel", "particle system", "dag"]}:
    raise SystemExit(f"expected callback strings changed: {strings}")

receipt = {
    "binary": str(binary), "sha256": digest,
    "method": "Installed ifm.dll callback byte verification plus matching disassembly instructions and string references",
    "callback": {"start": "0x1008e570", "endExclusive": "0x1008e7f2", "disassembly_instruction_count": sum(1 for line in disassembly.open(errors="replace") if (m := pattern.match(line)) and 0x1008E570 <= int(m.group(1),16) < 0x1008E7F2), "verifiedSpans": spans, "directCalls": calls, "strings": strings},
    "observations": [
        "The callback reads a selection collection from its state object and iterates the captured count in reverse order.",
        "It opens a named batch operation using the Delete AnimationSets string, performs per-item eligibility checks, and dispatches eligible entries through helper calls at 0x1072BAE0 and 0x103885E0.",
        "A dedicated refusal path displays 'Can't delete the animationset for the current camera!' under the 'Invalid Operation' title.",
        "The batch path invokes operation-context completion interfaces after processing the selection."
        ,"The dispatched helper at 0x1072BAE0 obtains a related-object collection, iterates its entries through 0x105D6610, inspects associated/dependent collections, and dispatches target/related-object notifications."
        ,"The helper at 0x105D6610 searches for the supplied relation key via 0x105D5500 and, when found, removes the corresponding entry from the owner collection through 0x103885E0."
        ,"The helper's virtual attribute query uses the exact installed key 'children' and later scans a handle collection stored at object offset +0xA4; it type-checks resolved entries before dispatching cleanup and notification paths. That collection's role remains unresolved."
        ,"Associated-object resolver 0x104D8240 probes five exact relation names in order: camera, light, gameModel, particle system, and dag. The delete helper uses its result to enter the associated-object cleanup path; which relation exists for a selection is data-dependent."
    ],
    "limits": [
        "The selection collection's full ownership and exact item type predicates are not reconstructed here.",
        "The relation cleanup helper is structurally understood, but the identities/ownership semantics of all traversed collections and exact object graph effects are not fully reconstructed; physical AnimationSet removal, all incoming-reference repair, dependent-control behavior, and undo replay are not proven.",
        "Static x86 code evidence only; callback execution and menu/runtime parity were not observed in SFM."
    ],
    "all_match": True
}
Path("outputs/animation-set-delete-handler-verification.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"sha256": digest, "spans": len(spans), "directCalls": len(calls), "strings": strings, "all_match": True}, indent=2))
