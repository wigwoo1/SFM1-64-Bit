"""Recover the shared transformation used by the native AnimationSet copy callback."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

BINARY = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
DISASSEMBLY = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/copy-animation-set-transform-recovery.json")
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
START, END = 0x101DB030, 0x101DB583

image = Image(BINARY)
digest = hashlib.sha256(image.b).hexdigest()
if digest != EXPECTED_SHA256:
    raise SystemExit(f"installed ifm.dll changed: {digest}")

first_line = re.compile(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)$")
continuation = re.compile(r"^\s{10,}([0-9A-F]{2}(?:\s+[0-9A-F]{2})*)\s*$")
rows, current, entered = [], None, False
for line in DISASSEMBLY.open(errors="replace"):
    match = first_line.match(line)
    if match:
        va = int(match.group(1), 16)
        if va < START:
            continue
        if current:
            rows.append(current)
            current = None
        if va >= END:
            entered = True
            break
        entered = True
        current = [va, bytearray.fromhex(match.group(2)), re.sub(r"\s+", " ", match.group(3).strip())]
    elif entered and current:
        match = continuation.match(line)
        if match:
            current[1].extend(bytes.fromhex(match.group(1)))
if current:
    rows.append(current)

cursor = START
direct_calls = []
instructions = []
for va, raw, text in rows:
    if va != cursor:
        raise SystemExit(f"helper disassembly gap at {cursor:#x}: next instruction {va:#x}")
    on_disk = image.b[image.offset(va):image.offset(va) + len(raw)]
    if bytes(raw) != on_disk:
        raise SystemExit(f"helper/disassembly byte mismatch at {va:#x}")
    instructions.append({"va": va, "bytes": bytes(raw).hex(), "text": text})
    call = re.search(r"\bcall\s+(10[0-9A-F]{6})\b", text, re.I)
    if call:
        direct_calls.append({"site": f"0x{va:08x}", "target": f"0x{int(call.group(1), 16):08x}"})
    cursor += len(raw)
if cursor != END:
    raise SystemExit(f"helper range ended at {cursor:#x}, expected {END:#x}")

def checked_call(va, expected, target):
    raw = image.b[image.offset(va):image.offset(va) + 5]
    actual_target = va + 5 + int.from_bytes(raw[1:5], "little", signed=True) if raw[:1] == b"\xE8" else 0
    if raw != bytes.fromhex(expected) or actual_target != target:
        raise SystemExit(f"call site mismatch at {va:#x}: {raw.hex()} -> {actual_target:#x}")
    return {"site": f"0x{va:08x}", "bytes": raw.hex(), "target": f"0x{actual_target:08x}"}

call_sites = [
    checked_call(0x10092098, "e8938f1400", START),
    checked_call(0x101D272D, "e8fe880000", START),
]
required_targets = {"0x107293e0", "0x107279a0", "0x1072ba10"}
observed_targets = {entry["target"] for entry in direct_calls}
if not required_targets.issubset(observed_targets):
    raise SystemExit(f"expected helper edges missing: {sorted(required_targets - observed_targets)}")

landmarks = {
    "collectionCountAtPlus0x0c": any(i["va"] == 0x101DB03F and i["text"] == "mov esi,dword ptr [edi+0Ch]" for i in instructions),
    "inputRecordStride0x0c": any(i["va"] == 0x101DB170 and i["text"] == "mov eax,dword ptr [ebp-14h]" for i in instructions) and any(i["va"] == 0x101DB19F and "add esi,0Ch" in i["text"] for i in instructions),
    "outputRecordStride0x48": any(i["va"] == 0x101DB43A and "add dword ptr [ebp-44h],48h" in i["text"] for i in instructions),
    "candidateResolver": any(i["va"] == 0x101DB319 and "call 107293E0" in i["text"] for i in instructions),
    "resultBuilder": any(i["va"] == 0x101DB397 and "call 107279A0" in i["text"] for i in instructions),
}
if not all(landmarks.values()):
    raise SystemExit(f"helper landmarks missing: {landmarks}")

receipt = {
    "binary": str(BINARY),
    "sha256": digest,
    "method": "Full x86 helper range matched against disassembly and installed bytes; direct call sites independently checked against rel32 targets",
    "helper": {"start": f"0x{START:08x}", "end_exclusive": f"0x{END:08x}", "bytes": cursor - START,
               "instruction_count": len(instructions), "direct_calls": direct_calls},
    "call_sites": call_sites,
    "verified_landmarks": landmarks,
    "observations": [
        "The helper receives a vector-like object whose item count is read at +0x0c and iterates input records at a 0x0c-byte stride.",
        "It constructs internal per-record state with a 0x48-byte stride, resolves candidate handles through the data-model service, and type-checks them against several global runtime descriptors.",
        "Each record passes through helper 0x107293E0; result construction calls 0x107279A0, and the helper also calls 0x1072BA10 for a related-object scan.",
        "The same helper is called by the registered Copy AnimationSet(s) callback and another routine at 0x101D272D, so the helper itself is not proven copy-specific.",
    ],
    "limits": [
        "The runtime identities of the descriptor globals and exact semantics of 0x107293E0 and 0x107279A0 remain unresolved.",
        "This trace does not prove the user-visible clipboard payload, paste behavior, complete selection filtering, or undo semantics.",
        "Static x86 evidence only; no live SFM clipboard or paste behavior was observed.",
    ],
    "all_match": True,
}
OUTPUT.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"sha256": digest, "helper_bytes": cursor - START, "instructions": len(instructions),
                  "direct_calls": len(direct_calls), "call_sites": call_sites, "all_match": True}, indent=2))
