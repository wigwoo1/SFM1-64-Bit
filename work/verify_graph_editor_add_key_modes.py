"""Verify the installed SFM Graph Editor Add Key mode menu-to-state path."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pe_image import Image


DLL = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
LISTING = Path("work/ifm-disassembly.txt")
SOURCE_RECOVERY = Path("outputs/graph-editor-keymode-construction-recovery.json")
HANDLER_RECOVERY = Path("outputs/graph-editor-keymode-handler-recovery.json")
SELECTION_RECOVERY = Path("outputs/graph-editor-keymode-selection-recovery.json")
OUTPUT = Path("outputs/graph-editor-add-key-modes-verification.json")

img = Image(str(DLL))
listing = {}
row_re = re.compile(r"\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} ?)+)\s*(.*)")
for line in LISTING.open(encoding="utf-8", errors="replace"):
    match = row_re.match(line)
    if match:
        listing[int(match.group(1), 16)] = {
            "bytes": bytes.fromhex(match.group(2)).hex(),
            "instruction": match.group(3).strip(),
        }


def verified(va):
    row = listing[va]
    offset = img.offset(va)
    actual = img.b[offset:offset + len(bytes.fromhex(row["bytes"]))]
    assert actual.hex() == row["bytes"], f"installed bytes differ at {va:#x}"
    return {"va": hex(va), **row}


construction = json.loads(SOURCE_RECOVERY.read_text(encoding="utf-8"))
handler = json.loads(HANDLER_RECOVERY.read_text(encoding="utf-8"))
callback = json.loads(SELECTION_RECOVERY.read_text(encoding="utf-8"))
existing = {int(s["va"], 16): s for s in construction["construction_window"]["verified_spans"]}
assert callback["common_callback"] == "0x100d61d0"
assert callback["setter"] == "0x100bf630"
assert "0x100d4ee0" == handler["keymode_preparation"]["entry"]

# Each action occupies a QAction pointer slot at its window's +0xcc sequence,
# shares the common callback, and captures the corresponding integer mode.
actions = [
    ("Curve Fit", 0, 0x0CC, 0x100D8DD3, 0x100D8DE3, 0x100D8DEB, 0x100D8DF3, 0x100D8E1C, 0x100D8E0C),
    ("Linear", 1, 0x0D0, 0x100D8E76, 0x100D8E86, 0x100D8807, 0x100D8E96, 0x100D8EBF, 0x100D8EAF),
    ("Spline", 2, 0x0D4, 0x100D8F19, 0x100D8F29, 0x100D8F39, 0x100D8F39, 0x100D8F66, 0x100D8F56),
    ("Stepped", 3, 0x0D8, 0x100D8FC7, 0x100D8FB4, 0x100D8FE5, 0x100D8FE5, 0x100D9012, 0x100D9002),
]
results = []
for label, mode, member, alloc_va, cb_va, mode_va, mode_store_va, member_store_va, text_va in actions:
    block = [existing[va] for va in sorted(existing) if alloc_va <= va <= member_store_va]
    assert block, f"missing construction span for {label}"
    member_store = existing[member_store_va]
    assert f"[esi+{member:08X}h]" in member_store["instruction"], f"missing action member offset for {label}"
    cb = existing[cb_va]
    assert f"{0x100D61D0:08X}h" in cb["instruction"], f"wrong callback for {label}"
    if label == "Stepped":
        assert "[eax+10h],edx" in existing[0x100D8FDC]["instruction"]
    if mode == 0:
        assert existing[mode_va]["instruction"] == "xor         ecx,ecx"
        assert existing[mode_store_va]["instruction"] == "mov         dword ptr [eax+20h],ecx"
        mode_capture = [verified(mode_va), verified(mode_store_va)]
    elif mode == 1:
        assert existing[mode_store_va]["instruction"] == "mov         dword ptr [eax+20h],ebx"
        ebx_init = verified(mode_va)
        assert ebx_init["instruction"] == "mov         ebx,1"
        mode_capture = [verified(mode_store_va), ebx_init]
    else:
        assert existing[mode_store_va]["instruction"] == f"mov         dword ptr [eax+20h],{mode}"
        mode_capture = verified(mode_store_va)
    label_va = next(int(s["string_va"], 16) for s in construction["construction_window"]["string_candidates"] if s["text"] == label)
    assert img.string(label_va) == label
    results.append({
        "label": label,
        "mode_value": mode,
        "action_member_offset": hex(member),
        "callback": "0x100d61d0",
        "evidence": [verified(alloc_va), verified(cb_va), *([verified(0x100D8FDC)] if label == "Stepped" else []), mode_capture, verified(member_store_va), verified(text_va)],
    })

setter = [verified(va) for va in range(0x100BF630, 0x100BF63D) if va in listing]
assert any("[ecx+0Ch],eax" in s["instruction"] for s in setter)
refresh = [verified(va) for va in range(0x100D61D0, 0x100D6240) if va in listing]
assert refresh
report = {
    "binary": str(DLL),
    "sha256": hashlib.sha256(img.b).hexdigest(),
    "actions": results,
    "setter_evidence": setter,
    "refresh_callback_evidence": refresh,
    "checked_state_entry": handler["keymode_preparation"]["entry"],
    "verified_behavior": "Four Graph Editor Add Key QAction entries capture modes 0-3 and dispatch through a common callback that stores the mode and refreshes the UI; a separate routine checks the matching QAction state against the stored value.",
    "limitations": "Static installed-binary evidence only. This receipt does not prove runtime menu display, interpolation/key-edit behavior, or integration into the x64 replacement UI.",
}
OUTPUT.write_text(json.dumps(report, indent=2), encoding="utf-8")
print(json.dumps({"verified_actions": len(results), "sha256": report["sha256"], "output": str(OUTPUT)}, indent=2))
