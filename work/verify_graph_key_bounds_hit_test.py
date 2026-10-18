"""Byte-verify SFM's graph selection-bounds key hit-test and its call paths."""
import hashlib
import json
import re
from pathlib import Path

from pe_image import Image

DLL = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
DISASM = Path("work/ifm-disassembly.txt")
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
RANGES = [
    (0x100C4B90, 0x100C4BF0, "undoable Select Keys candidate-list consumer"),
    (0x100D10C0, 0x100D12DD, "per-control graph-bounds key hit-test"),
    (0x100D3250, 0x100D34E0, "selection-state collector and fallbacks"),
    (0x100D34E0, 0x100D35B0, "mouse gesture bounds finalizer"),
    (0x100D35B0, 0x100D380A, "mouse gesture dispatcher"),
]

image = Image(DLL)
digest = hashlib.sha256(image.b).hexdigest()
assert digest == EXPECTED_SHA256, f"unexpected installed ifm.dll: {digest}"
records = [[] for _ in RANGES]
active = None
for line in DISASM.open(encoding="utf-8"):
    m = re.match(r"\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)", line)
    if m:
        va = int(m[1], 16)
        active = None
        for i, (start, end, _) in enumerate(RANGES):
            if start <= va < end:
                active = {"va": va, "bytes": bytearray.fromhex(m[2]), "instruction": m[3].strip()}
                records[i].append(active)
                break
        if va >= RANGES[-1][1]:
            break
        continue
    m = re.fullmatch(r"\s{12}((?:[0-9A-F]{2} ?)+)\s*", line.rstrip())
    if m and active is not None:
        active["bytes"].extend(bytearray.fromhex(m[1]))

verified = []
for (start, end, name), rows in zip(RANGES, records):
    cursor = start
    span = bytearray()
    for row in rows:
        assert row["va"] == cursor, f"{name}: disassembly gap at {cursor:#x}"
        span.extend(row["bytes"])
        cursor += len(row["bytes"])
    assert cursor == end, f"{name}: parsed through {cursor:#x}, expected {end:#x}"
    assert image.b[image.offset(start):image.offset(end)] == span, f"{name}: bytes differ from installed DLL"
    verified.append({"start": hex(start), "end_exclusive": hex(end), "bytes": len(span), "name": name})

hit_test = records[1]
instructions = "\n".join(row["instruction"] for row in hit_test)
for evidence in ("100BF5A0", "10738F80", "100DAFA0", "100C0DF0"):
    assert evidence in instructions, f"missing hit-test stage {evidence}"

receipt = {
    "installed_dll": str(DLL),
    "installed_dll_sha256": digest,
    "verified_ranges": verified,
    "verified_behavior": [
        "0x100D10C0 receives a non-null control and two four-integer range structures; it obtains the control's graph-view bounds and intersects them with the requested range.",
        "For each usable control it enumerates animation channels and their key samples, maps each sample through graph coordinate transforms, and applies inclusive comparisons against the selected range.",
        "A hit is appended to a 16-byte candidate-record array through 0x100C0DF0; this helper is storage insertion, not selection mutation.",
        "0x100D3250 invokes this helper while gathering the 'Select Keys' command's selected-control records; its caller passes a non-null bounds record when a gesture is finalized.",
        "0x100D34E0 constructs the bounds record from stored pointer endpoints with two-pixel padding and passes it to 0x100D3250; 0x100D35B0 is the adjacent mouse event dispatcher.",
        "The gesture finalizer supplies the graph control's +0x174 mode value to 0x100D3250; the collector forwards that value unchanged to 0x100C4B90, which passes it with the candidate list to 0x107227B0 inside the 'Select Keys' undo scope.",
        "The mouse dispatcher derives the mode from Qt keyboard modifier bits in its first event argument: neither Control nor Alt -> 0; both -> 1; Alt only -> 2; Control only -> 3. Shift (bit 25) is read separately and does not choose this mode.",
        "In 0x107227B0, mode 0 clears selection masks on existing selected controls and applies candidates; mode 1 keeps existing selection and applies candidates; mode 2 clears the packed candidate flags and XOR-updates the owning selection mask; mode 3 performs that clear/XOR only when the candidate's selected bit was already set.",
    ],
    "limitations": [
        "The code-to-modifier mode mapping is static evidence; exact user-facing naming for modes 2 and 3 and live confirmation of selection results remain unverified.",
        "The candidate record's four fields and the downstream undo-command application need a separate audit.",
        "No live click-and-drag interaction or visual parity was tested in SFM.",
        "This is reverse-engineering evidence, not implementation of native selection in the x64 app.",
    ],
}
receipt["references"] = [
    {"title": "Qt 4.7 Qt Namespace: KeyboardModifier values", "url": "https://doc.qt.io/Archives/qt-4.7/qt.html"}
]
Path("outputs/graph-key-bounds-hit-test-verification.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(f"Verified {sum(r['bytes'] for r in verified)} bytes across {len(verified)} ranges; hit-test helper 0x100D10C0 is connected to the finalized graph bounds path.")
