import hashlib
import json
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

# x86 MSVC RTTI complete-object-locator immediately precedes a vftable.
vftable = 0x10AC51E8
locator = im.word(vftable - 4)
type_descriptor = im.word(locator + 12)
hierarchy = im.word(locator + 16)
if im.string(type_descriptor + 8) != ".?AVCQTimelineEditorController@@":
    raise AssertionError("candidate vftable RTTI does not identify CQTimelineEditorController")
base_count = im.word(hierarchy + 8)
base_array = im.word(hierarchy + 12)
base_types = []
for index in range(base_count):
    descriptor = im.word(base_array + 4 * index)
    base_types.append(im.string(im.word(descriptor) + 8))
if base_types != [".?AVCQTimelineEditorController@@", ".?AVQObject@@"]:
    raise AssertionError(f"unexpected candidate hierarchy {base_types}")

slots = {
    "toggle_snap_callback_slot_0x57c": {
        "thunk": 0x10027230,
        "vtable_slot": vftable + 0x57C,
        "entry": im.word(vftable + 0x57C),
        "import_iat": 0x10A7D948,
        "import_name": "?data@QListWidgetItem@@UBE?AVQVariant@@H@Z",
    },
    "toggle_snap_to_frame_callback_slot_0x578": {
        "thunk": 0x100270C0,
        "vtable_slot": vftable + 0x578,
        "entry": im.word(vftable + 0x578),
        "import_iat": 0x10A7D944,
        "import_name": "?setBackgroundColor@QListWidgetItem@@UAEXABVQColor@@@Z",
    },
}
for name, item in slots.items():
    thunk_offset = im.offset(item["thunk"])
    thunk = raw[thunk_offset:thunk_offset + 8]
    entry_offset = im.offset(item["entry"])
    entry = raw[entry_offset:entry_offset + 6]
    expected_thunk = b"\x8b\x01\xff\xa0" + (item["vtable_slot"] - vftable).to_bytes(4, "little")
    if thunk != expected_thunk:
        raise AssertionError(f"{name} dispatch thunk changed: {thunk.hex()}")
    if entry[:2] != b"\xff\x25" or int.from_bytes(entry[2:6], "little") != item["import_iat"]:
        raise AssertionError(f"{name} entry is not the expected import forwarding stub: {entry.hex()}")
    item["thunk_bytes"] = thunk.hex(" ")
    item["entry_bytes"] = entry.hex(" ")

report = {
    "dll": str(binary),
    "sha256": digest,
    "candidate_vftable": hex(vftable),
    "rtti_locator": hex(locator),
    "rtti_type": im.string(type_descriptor + 8),
    "rtti_base_types": base_types,
    "candidate_slots": slots,
    "import_symbols": {hex(key): value for key, value in {
        0x10A7D944: slots["toggle_snap_to_frame_callback_slot_0x578"]["import_name"],
        0x10A7D948: slots["toggle_snap_callback_slot_0x57c"]["import_name"],
    }.items()},
    "conclusion": "RTTI confirms a CQTimelineEditorController vftable candidate, but its entries at the two snap callback offsets forward to QListWidgetItem methods. This contradiction does not identify the snap handlers; runtime callback target and actual handler bodies remain unresolved.",
}
Path("outputs/timeline-snap-vtable-candidate-audit.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps(report, indent=2))
