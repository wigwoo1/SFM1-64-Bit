"""Byte-audit the installed helper that builds AnimationSet clipboard entries."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

BINARY = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
DISASSEMBLY = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/native-animation-set-clipboard-builder-recovery.json")
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
RANGES = {
    "shared_transform": (0x101DB030, 0x101DB583),
    "clipboard_builder": (0x107279A0, 0x107293E0),
    "typed_candidate_append": (0x107293E0, 0x10729490),
}

image = Image(BINARY)
digest = hashlib.sha256(image.b).hexdigest()
if digest != EXPECTED_SHA256:
    raise SystemExit(f"installed ifm.dll changed: {digest}")

ordered = sorted((start, end, name) for name, (start, end) in RANGES.items())
rows = {name: [] for name in RANGES}
first_line = re.compile(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)$")
continuation = re.compile(r"^\s{10,}([0-9A-F]{2}(?:\s+[0-9A-F]{2})*)\s*$")
current = None
range_index = [0]

def store_current():
    if current is None:
        return
    va = current[0]
    while range_index[0] < len(ordered) and va >= ordered[range_index[0]][1]:
        range_index[0] += 1
    if range_index[0] < len(ordered):
        start, end, name = ordered[range_index[0]]
        if start <= va < end:
            rows[name].append(current)

for line in DISASSEMBLY.open(errors="replace"):
    match = first_line.match(line)
    if match:
        store_current()
        va = int(match.group(1), 16)
        current = [va, bytearray.fromhex(match.group(2)), re.sub(r"\s+", " ", match.group(3).strip())]
    elif current is not None:
        match = continuation.match(line)
        if match:
            current[1].extend(bytes.fromhex(match.group(1)))
store_current()

instructions = {}
direct_calls = {}
for name, (start, end) in RANGES.items():
    cursor = start
    instructions[name] = []
    direct_calls[name] = []
    for va, raw, text in rows[name]:
        if va != cursor:
            raise SystemExit(f"{name} disassembly gap at {cursor:#x}: next {va:#x}")
        disk = image.b[image.offset(va):image.offset(va) + len(raw)]
        if bytes(raw) != disk:
            raise SystemExit(f"{name} disassembly differs from installed bytes at {va:#x}")
        instructions[name].append({"va": va, "bytes": bytes(raw).hex(), "text": text})
        call = re.search(r"\bcall\s+(10[0-9A-F]{6})\b", text, re.I)
        if call:
            direct_calls[name].append({"site": f"0x{va:08x}", "target": f"0x{int(call.group(1),16):08x}"})
        cursor += len(raw)
    if cursor != end:
        raise SystemExit(f"{name} ended at {cursor:#x}, expected {end:#x}")

def check_call(va, expected_bytes, expected_target):
    raw = image.b[image.offset(va):image.offset(va) + 5]
    target = va + 5 + int.from_bytes(raw[1:5], "little", signed=True) if raw[:1] == b"\xE8" else 0
    if raw != bytes.fromhex(expected_bytes) or target != expected_target:
        raise SystemExit(f"call-site mismatch at {va:#x}: {raw.hex()} -> {target:#x}")
    return {"site": f"0x{va:08x}", "bytes": raw.hex(), "target": f"0x{target:08x}"}

call_sites = [
    check_call(0x101DB397, "e804c65400", 0x107279A0),
    check_call(0x101DB319, "e8c2e05400", 0x107293E0),
]

string_addresses = {0x10A8A800, 0x10ACE710, 0x10ABDE34, 0x10BC2280, 0x10BC226C,
                    0x10BC2264, 0x10A8F5E4}
for entry in instructions["clipboard_builder"]:
    for match in re.finditer(r"(?<![0-9A-F])10[0-9A-F]{6}h\b", entry["text"], re.I):
        address = int(match.group(0)[:-1], 16)
        try:
            value = image.string(address)
        except Exception:
            continue
        if value and value.isprintable():
            string_addresses.add(address)
string_refs = [{"address": f"0x{address:08x}", "value": image.string(address)}
               for address in sorted(string_addresses)]
expected_values = {"Clipboard", "animset", "target", "channelsclip", "timechannelsclip", "RigList", "element"}
if not expected_values.issubset({item["value"] for item in string_refs}):
    raise SystemExit(f"expected builder strings missing: {string_refs}")

typed_rows = instructions["typed_candidate_append"]
if not any("[ecx+14h]" in i["text"] for i in typed_rows):
    raise SystemExit("typed append helper does not read the source item count")
if not any("10EB6B0Ch" in i["text"] for i in typed_rows):
    raise SystemExit("typed append descriptor reference missing")

receipt = {
    "binary": str(BINARY), "sha256": digest,
    "method": "Three complete helper ranges matched against disassembly and installed ifm.dll bytes; helper call targets independently decoded from installed rel32 instructions",
    "ranges": {name: {"start": f"0x{start:08x}", "end_exclusive": f"0x{end:08x}",
                      "bytes": end-start, "instruction_count": len(instructions[name]),
                      "direct_call_count": len(direct_calls[name]), "direct_calls": direct_calls[name]}
               for name, (start, end) in RANGES.items()},
    "call_sites": call_sites,
    "clipboard_builder_string_references": string_refs,
    "observations": [
        "The shared result builder at 0x107279A0 directly references a Clipboard container and fields named animset, target, channelsclip, timechannelsclip, RigList, and element.",
        "The builder iterates a collection at object offset +0x0c, type-checks candidate items against a global runtime descriptor, and constructs clipboard-entry data using multiple attribute and value helpers.",
        "The typed append helper at 0x107293E0 reads a source count at +0x14, checks each candidate against descriptor storage at 0x10EB6B0C, and appends accepted references to a caller-provided vector.",
        "The shared transform calls both helpers, while its own caller is not copy-specific; the native Copy AnimationSet(s) callback invokes the shared transform and receives the builder's results.",
    ],
    "limits": [
        "The descriptor at 0x10EB6B0C and the runtime meanings of the attribute/value helpers remain unidentified.",
        "The full Clipboard graph semantics, exact per-selection filtering, paste operation, and undo behavior are not proven by these spans.",
        "Static x86 code evidence only; no live SFM copy/paste interaction was observed.",
    ],
    "all_match": True,
}
OUTPUT.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"sha256": digest, "ranges": {name: {"bytes": end-start, "instructions": len(instructions[name])}
                                                for name,(start,end) in RANGES.items()},
                  "clipboard_strings": sorted(expected_values), "call_sites": call_sites, "all_match": True}, indent=2))
