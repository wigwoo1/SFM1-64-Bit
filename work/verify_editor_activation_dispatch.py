"""Verify installed SFM Qt menu slots through editor routing methods."""
import hashlib
import json
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pe_image import Image

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


def c_string(va):
    result = bytearray()
    for offset in range(1024):
        value = data(va + offset, 1)[0]
        if value == 0:
            return result.decode("ascii")
        result.append(value)
    raise ValueError(f"unterminated metadata string at {va:#x}")


def check(va, expected_hex, label):
    expected = bytes.fromhex(expected_hex)
    actual = data(va, len(expected))
    if actual != expected:
        raise SystemExit(f"byte mismatch for {label} at {va:#x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual), "label": label}


def method_signature(meta_data, string_data, local_id, method_count):
    header = struct.unpack("<14I", data(meta_data, 56))
    if header[0] != 6 or header[4] != method_count or header[5] != 14:
        raise SystemExit(f"unexpected Qt metadata header at {meta_data:#x}: {header}")
    row_va = meta_data + (header[5] + local_id * 5) * 4
    row = struct.unpack("<5I", data(row_va, 20))
    return c_string(string_data + row[0]), row


# Metadata and vtable ownership: these tables resolve to the installed RTTI
# type descriptors, not just similarly named strings.
controller_metaobject = 0x10AA1024
controller_strings = 0x10AA0958
controller_data = 0x10AA0390
controller_vtable = 0x10AA9984
controller_col = word(controller_vtable - 4)
controller_td = word(controller_col + 12)
if im.string(controller_td + 8) != ".?AVCQSFMController@@":
    raise SystemExit("controller vtable RTTI mismatch")
if (word(controller_metaobject + 4), word(controller_metaobject + 8)) != (controller_strings, controller_data):
    raise SystemExit("CQSFMController Qt metadata pointer mismatch")

window_metaobject = 0x10AA14AC
window_strings = 0x10AA13A0
window_data = 0x10AA12B0
window_vtable = 0x10AAA87C
window_col = word(window_vtable - 4)
window_td = word(window_col + 12)
if im.string(window_td + 8) != ".?AVCQSFMMainWindow@@":
    raise SystemExit("main-window vtable RTTI mismatch")
if (word(window_metaobject + 4), word(window_metaobject + 8)) != (window_strings, window_data):
    raise SystemExit("CQSFMMainWindow Qt metadata pointer mismatch")

controller_names = {}
for local_id in range(71):
    signature, _row = method_signature(controller_data, controller_strings, local_id, 71)
    controller_names[local_id] = signature
window_menu_signature, _ = method_signature(window_data, window_strings, 8, 9)
if window_menu_signature != "OnAboutToShowAdditionalEditorsMenu()":
    raise SystemExit(f"unexpected main-window slot 8: {window_menu_signature}")

verified = []
verified.append(check(0x100796C0, "8b41048b401885c07505b82410aa10c3", "CQSFMController metaobject accessor"))
verified.append(check(0x100796D0, "558bec8b450c538b5d105657538bf98b4d085051", "CQSFMController qt_metacall entry"))
verified.append(check(0x100791B0, "558bec837d0c000f85d80300008b451083f8460f87cc030000ff248598950710", "CQSFMController generated slot switch"))
verified.append(check(0x10079BC0, "558bec8b450c538b5d105657538bf98b4d085051", "CQSFMMainWindow qt_metacall entry"))
verified.append(check(0x10079A90, "558bec837d0c000f85910000008b451083f8080f8785000000ff2485309b0710", "CQSFMMainWindow generated slot switch"))

menu_string_refs = [
    (0x100AD3EE, 0x10AAB93C, "Clip Editor"),
    (0x100AD37F, 0x10AAB948, "ShowTimeline"),
    (0x100AD4AE, 0x10AAB918, "Motion Editor"),
    (0x100AD43F, 0x10AAB928, "ShowMotionEditor"),
    (0x100AD56E, 0x10AAB8F8, "Graph Editor"),
    (0x100AD4FF, 0x10AAB908, "ShowGraphEditor"),
]
for instruction_va, string_va, expected_string in menu_string_refs:
    verified.append(check(instruction_va, "68" + struct.pack("<I", string_va).hex(), f"menu caption/action-key reference: {expected_string}"))
    if c_string(string_va) != expected_string:
        raise SystemExit(f"menu string mismatch at {string_va:#x}: {c_string(string_va)!r}")

actions = {
    49: ("ShowMotionEditor", "Motion Editor", 0x148, 2),
    50: ("ShowGraphEditor", "Graph Editor", 0x14C, 4),
    51: ("ShowTimeline", "Clip Editor", 0x150, 3),
}
editor_window_methods = []
for local_id in range(44, 59):
    signature = controller_names[local_id]
    if not (signature.startswith("OnShow") and signature.endswith("Widget()")):
        continue
    slot_index = local_id + 33
    byte_offset = slot_index * 4
    switch_target = word(0x10079598 + local_id * 4)
    expected_case = bytes.fromhex("8b4d088b118b82") + struct.pack("<I", byte_offset) + bytes.fromhex("5dffe0")
    actual_case = data(switch_target, len(expected_case))
    if actual_case != expected_case:
        raise SystemExit(f"generated case does not dispatch {signature} to slot {slot_index}: {actual_case.hex()}")
    handler = word(controller_vtable + byte_offset)
    record = {
        "localMethodId": local_id,
        "signature": signature,
        "virtualSlot": slot_index,
        "virtualByteOffset": f"0x{byte_offset:x}",
        "switchCase": f"0x{switch_target:08x}",
        "switchCaseBytes": actual_case.hex(),
        "handler": f"0x{handler:08x}",
    }
    if local_id in actions:
        action_key, caption, expected_byte_offset, selector = actions[local_id]
        if signature != {
            49: "OnShowMotionEditorWidget()",
            50: "OnShowGraphEditorWidget()",
            51: "OnShowTimelineEditorWidget()",
        }[local_id]:
            raise SystemExit(f"unexpected editor action slot name at local id {local_id}: {signature}")
        if byte_offset != expected_byte_offset:
            raise SystemExit(f"editor action slot offset mismatch for {signature}")
        record.update({"actionKey": action_key, "caption": caption, "routingSelector": selector})
        # The three seven-byte thunks pass their selector to the same router.
        thunk_expected = bytes([0x6A, selector]) + b"\xE8" + struct.pack("<i", 0x1009A820 - (handler + 7)) + b"\xC3"
        if data(handler, len(thunk_expected)) != thunk_expected:
            raise SystemExit(f"selector thunk mismatch for {signature}: {data(handler, len(thunk_expected)).hex()}")
        record["handlerBytes"] = thunk_expected.hex()
        record["sharedRouter"] = "0x1009a820"
    editor_window_methods.append(record)

if {m["localMethodId"] for m in editor_window_methods} != set(range(44, 59)):
    raise SystemExit("expected the full editor-window slot range 44..58")

# Layout commands are adjacent to the pane-show slots in the same Qt class.
# Keep them separate from widget-show actions because their entry points have
# different responsibilities and do not take a selector argument.
layout_methods = []
for local_id, expected_signature, expected_handler, expected_bytes in [
    (59, "OnShowDefaultLayout()", 0x1009A3B0, "8b018b51108b407c52ffd0c3"),
    (60, "OnShowDesktopWindow()", 0x1009AF20, "568bf18b4e08ff1504c9a7108b4e085eff25bccba710"),
]:
    signature = controller_names[local_id]
    if signature != expected_signature:
        raise SystemExit(f"unexpected layout action at local id {local_id}: {signature}")
    switch_target = word(0x10079598 + local_id * 4)
    if local_id == 59:
        byte_offset = 0x5C
        expected_case = bytes.fromhex("8b4d088b118b425c5dffe0")
    else:
        byte_offset = 0x170
        expected_case = bytes.fromhex("8b4d088b118b82700100005dffe0")
    slot_index = byte_offset // 4
    actual_case = data(switch_target, len(expected_case))
    if actual_case != expected_case:
        raise SystemExit(f"generated case does not dispatch {signature} to slot {slot_index}: {actual_case.hex()}")
    handler = word(controller_vtable + byte_offset)
    if handler != expected_handler:
        raise SystemExit(f"unexpected handler for {signature}: {handler:#x}")
    thunk = bytes.fromhex(expected_bytes)
    if data(handler, len(thunk)) != thunk:
        raise SystemExit(f"handler bytes mismatch for {signature}: {data(handler, len(thunk)).hex()}")
    record = {
        "localMethodId": local_id,
        "signature": signature,
        "virtualSlot": slot_index,
        "virtualByteOffset": f"0x{byte_offset:x}",
        "switchCase": f"0x{switch_target:08x}",
        "switchCaseBytes": actual_case.hex(),
        "handler": f"0x{handler:08x}",
        "handlerBytes": thunk.hex(),
    }
    layout_methods.append(record)
    verified.append(check(switch_target, expected_case.hex(), f"{signature} Qt meta-call dispatch"))
    verified.append(check(handler, expected_bytes, f"{signature} vtable target"))

# CQSFMMainWindow's slot 8 is the menu-population path; it jumps to slot 59,
# whose concrete body iterates/rebuilds menu entries.
main_window_case = word(0x10079B30 + 8 * 4)
if main_window_case != 0x10079B20:
    raise SystemExit(f"main-window menu case target mismatch: {main_window_case:#x}")
verified.append(check(main_window_case, "8b4d088b018b90ec0000005dffe2", "Additional Editors menu slot dispatch to CQSFMMainWindow slot 59"))
menu_population_target = word(window_vtable + 0xEC)
if menu_population_target != 0x100AEC60:
    raise SystemExit(f"main-window menu handler mismatch: {menu_population_target:#x}")
verified.append(check(menu_population_target, "558bec83ec34538bd98b4b505657895de8c745f8000000", "Additional Editors menu-population handler"))

# The shared handler compares the selected editor type and, on a change, calls
# the CIFMTool routing path with the selector and activation flags.
verified.append(check(0x1009A868, "8b0d6825f310e8cd1623008b4d083bc174106a006a01518b0d6825f310e8e63e2300", "shared active-editor selector and CIFMTool routing"))
router_calls = {
    "readCurrentSelection": 0x102CBF40,
    "setSelection": 0x102CE770,
}
for va, expected_target in ((0x1009A86E, router_calls["readCurrentSelection"]), (0x1009A885, router_calls["setSelection"])):
    raw = data(va, 5)
    if raw[0] != 0xE8:
        raise SystemExit(f"expected relative call at {va:#x}")
    target = va + 5 + struct.unpack("<i", raw[1:])[0]
    if target != expected_target:
        raise SystemExit(f"router call mismatch at {va:#x}: {target:#x} != {expected_target:#x}")

report = {
    "target": str(TARGET),
    "sha256": digest,
    "architecture": "installed SFM DLL is x86; byte offsets and enum values are evidence for this build, not x64 ABI definitions",
    "mainWindowMenuSlot": {
        "class": "CQSFMMainWindow",
        "metaMethodId": 8,
        "signature": window_menu_signature,
        "qtMetacall": "0x10079bc0",
        "generatedSwitch": f"0x{main_window_case:08x}",
        "vtableByteOffset": "0xec",
        "handler": f"0x{menu_population_target:08x}",
        "verifiedRttiName": im.string(window_td + 8),
    },
    "controllerClass": {
        "name": "CQSFMController",
        "metaobject": f"0x{controller_metaobject:08x}",
        "methodData": f"0x{controller_data:08x}",
        "methodCount": 71,
        "vtable": f"0x{controller_vtable:08x}",
        "verifiedRttiName": im.string(controller_td + 8),
        "editorWindowMethods": editor_window_methods,
        "layoutMethods": layout_methods,
    },
    "menuActionDispatch": [
        {
            "caption": actions[mid][1],
            "actionKey": actions[mid][0],
            "controllerSlot": next(x for x in editor_window_methods if x["localMethodId"] == mid),
        }
        for mid in (49, 50, 51)
    ],
    "sharedRouter": {
        "handler": "0x1009a820",
        "calls": {k: f"0x{v:08x}" for k, v in router_calls.items()},
        "observed": "reads the current selection through the CIFMTool singleton at global 0x10f32568; if the requested selector differs, forwards it with activation flags to the setter path and notifies the associated controller callback",
        "selectors": {"Motion Editor": 2, "Graph Editor": 4, "Clip Editor/Timeline": 3},
    },
    "verifiedRanges": verified,
    "interpretation": "The MainWindow's Qt slot names and targets the Additional Editors population handler. Its action delegates dispatch into CQSFMController's generated Qt meta-call table; the method IDs and primary vtable resolve Motion, Graph and Timeline actions to concrete one-integer selector thunks. The selector thunks call a shared router which compares and updates active editor selection through CIFMTool. The same controller metadata exposes OnShowDefaultLayout and OnShowDesktopWindow; both dispatch slots and target handlers are now byte-verified. This establishes static action-to-controller dispatch, but runtime menu display, live object capture identity, exact pane visibility/docking, and screenshot comparison remain unverified.",
    "supersedes": "The earlier CIFMTool primary-panel vtable match was coincidental; the generated Qt method table and the CQSFMController RTTI/vtable identify CQSFMController as the class implementing these editor-window slots.",
    "result": "passed",
}
out = pathlib.Path("outputs/sfm64-editor-action-dispatch-audit.json")
out.write_text(json.dumps(report, indent=2), encoding="utf-8")

# Make the prior, weaker receiver-candidate artifact explicitly stale instead
# of leaving it looking like a competing class identification.
old = pathlib.Path("outputs/sfm64-cifmtool-menu-receiver-candidate.json")
if old.exists():
    old_report = json.loads(old.read_text(encoding="utf-8"))
    old_report["result"] = "superseded"
    old_report["interpretation"] = "Superseded: matching the same slot offsets in CIFMTool was coincidental. The Qt meta-method names, generated switch cases, CQSFMController RTTI and CQSFMController vtable now identify the editor-window slot implementation. Retained only as an audit trail of the rejected candidate."
    old_report["supersededBy"] = str(out)
    old.write_text(json.dumps(old_report, indent=2), encoding="utf-8")

print(f"verified {len(verified)} byte spans, {len(editor_window_methods)} editor-window slots, and {len(layout_methods)} layout commands; sha256={digest}; output={out}")
