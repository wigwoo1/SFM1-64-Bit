"""Verify Graph Editor view Qt slot dispatch against the installed SFM DLL."""
import hashlib
import json
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pe_image import Image

ROOT = pathlib.Path(__file__).resolve().parents[1]
TARGET = pathlib.Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
im = Image(TARGET)
digest = hashlib.sha256(im.b).hexdigest()
if digest != EXPECTED_SHA256:
    raise SystemExit(f"installed ifm.dll changed: {digest}")


def data(va, length):
    offset = im.offset(va)
    return im.b[offset:offset + length]


def word(va):
    return struct.unpack("<I", data(va, 4))[0]


def check(va, expected_hex, label):
    expected = bytes.fromhex(expected_hex)
    actual = data(va, len(expected))
    if actual != expected:
        raise SystemExit(f"{label} mismatch at {va:#x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual), "label": label}


# CQGraphEditorView meta-call receives four locally registered slots. Resolve
# the first two generated cases through their jump-table rather than relying
# only on method names in Qt metadata.
check(0x1007421D, "53 56 6a 00 57 e8 e9 fe ff ff", "meta-call forwards generated local slot")
dispatch = 0x10074110
local0 = word(0x10074174)
local1 = word(0x10074178)
if (local0, local1) != (0x10074128, 0x1007413E):
    raise SystemExit(f"unexpected Graph Editor local-slot dispatch table: {local0:#x}, {local1:#x}")
verified = [
    check(0x10074110, "55 8b ec 83 7d 0c 00 75 57 8b 45 10 83 f8 03", "CQGraphEditorView generated meta-call dispatch"),
    check(local0, "8b 4d 08 8b 55 14 8b 01 8b 52 04 8b 80 c8 00 00 00 52 ff d0", "context-menu slot virtual dispatch"),
    check(local1, "8b 4d 08 8b 11 8b 82 cc 00 00 00 5d ff e0", "Escape slot virtual dispatch"),
    check(0x100D0678, "c7 06 5c e4 aa 10", "Graph Editor view constructor vtable assignment"),
    check(0x100CBA20, "55 8b ec 83 ec 0c 53 57 33 db", "Graph Editor context-menu handler prologue"),
    check(0x100CBA3C, "84 c0 0f 85 b6 00 00 00", "context-menu handler guard branch"),
    check(0x100CBA65, "56 6a 14 e8 13 30 19 00", "context-menu handler allocates 0x14-byte record when guards pass"),
    check(0x100D49F0, "55 8b ec 83 ec 08 53 56 57 33 db", "context-menu action-population helper prologue"),
    check(0x106D6C60, "c3", "Graph Editor Escape target body"),
]

view_vtable = 0x10AAE45C
context_target = word(view_vtable + 0xC8)
escape_target = word(view_vtable + 0xCC)
if context_target != 0x100CBA20 or escape_target != 0x106D6C60:
    raise SystemExit(f"view vtable targets changed: context={context_target:#x}, escape={escape_target:#x}")

report = {
    "target": str(TARGET),
    "sha256": digest,
    "evidenceType": "installed x86 ifm.dll byte verification; not an x64 ABI description",
    "class": "CQGraphEditorView",
    "qtLocalSlots": {
        "OnCustomContextMenuRequested(QPoint)": {
            "localId": 0,
            "generatedDispatch": f"0x{local0:08x}",
            "virtualByteOffset": "0xc8",
            "target": f"0x{context_target:08x}",
            "observed": "the target checks two global mode/state values and a view field before conditionally allocating a 0x14-byte record, passing it through context services, then calling the action-population helper; exact labels and visible menu actions remain unproved by this trace",
        },
        "OnEscapeHasBeenPressed()": {
            "localId": 1,
            "generatedDispatch": f"0x{local1:08x}",
            "virtualByteOffset": "0xcc",
            "target": f"0x{escape_target:08x}",
            "observed": "the installed target body is a single RET instruction; this slot has no local state-changing body in the inspected build",
        },
    },
    "verifiedRanges": verified,
    "limitations": [
        "The context helper chain has not been dynamically invoked and its allocated object/action captions are not fully identified.",
        "The one-instruction Escape target may represent an intentional no-op or inherited/default behavior; user-visible effect was not runtime-tested.",
        "No replacement editor UI or screenshot was produced by this static trace.",
    ],
    "result": "passed",
}
out = ROOT / "outputs" / "sfm64-graph-editor-view-dispatch-audit.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
worklist_path = ROOT / "outputs" / "editor-priority-handler-map.json"
worklist = json.loads(worklist_path.read_text(encoding="utf-8"))
evidence_path = "outputs/sfm64-graph-editor-view-dispatch-audit.json"
for signature in ("OnCustomContextMenuRequested(QPoint)", "OnEscapeHasBeenPressed()"):
    matches = [entry for entry in worklist["handlers"] if entry.get("class_name") == "CQGraphEditorView" and entry.get("signature") == signature]
    if len(matches) != 1:
        raise SystemExit(f"expected exactly one CQGraphEditorView worklist row for {signature}, got {len(matches)}")
    matches[0]["status"] = "Qt local-slot and concrete vtable dispatch byte-verified; complete runtime behavior remains unverified"
    matches[0].setdefault("evidence", [])
    if evidence_path not in matches[0]["evidence"]:
        matches[0]["evidence"].append(evidence_path)
worklist_path.write_text(json.dumps(worklist, indent=2), encoding="utf-8")
print(f"verified {len(verified)} Graph Editor view code spans; vtable=context {context_target:#x}, Escape {escape_target:#x}; sha256={digest}")
