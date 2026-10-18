"""Verify selected editor tool-dictionary Init bytes in the installed ifm.dll."""
import hashlib
import json
import pathlib
import struct
import sys

TARGET = pathlib.Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
BASE = 0x10000000


def main():
    data = TARGET.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        raise SystemExit(f"installed ifm.dll changed: {digest}")

    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise SystemExit("invalid PE signature")
    sections = []
    count = struct.unpack_from("<H", data, pe + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe + 20)[0]
    opt = pe + 24
    image_base = struct.unpack_from("<I", data, opt + 28)[0]
    if image_base != BASE:
        raise SystemExit(f"unexpected image base: {image_base:#x}")
    for i in range(count):
        off = opt + opt_size + 40 * i
        virtual_size, rva, raw_size, raw = struct.unpack_from("<IIII", data, off + 8)
        sections.append((rva, max(virtual_size, raw_size), raw))

    def read_va(va, size):
        rva = va - BASE
        for start, extent, raw in sections:
            if start <= rva and rva + size <= start + extent:
                file_off = raw + rva - start
                return data[file_off:file_off + size]
        raise ValueError(f"VA not in section: {va:#x}")

    # Contiguous instruction spans at the dictionary Init entry and its exit.
    checks = [
        (0x10989CC0, "833d44c7f41000740c6a006a00e87e23ffff83c408e806b70a00"),
        (0x10989CDF, "0f57c0505050506a0283ec0c"),
        (0x10989D09, "8b0dacafe7108b018b1083c42068b4b3dd10ffd284c07511"),
        (0x10989D21, "689cb3dd10ff1564e4a71083c40433c0c3"),
        (0x10989D32, "b801000000c3"),
        (0x10989D40, "e83b7100008b0dacafe7108b018b5004ffd2e8c9b60a00"),
        (0x10989E01, "833d44c7f41000745983"),
        (0x1097C0B0, "8b16a1302efd108b520c8b7e04508bceffd28b068b502c8bceffd28bf785ff75df5f"),
        (0x1097C100, "8b0d44c7f41085c9743e803d342efd100074358b018b15b0e3f1108b808000000052ffd08b0d44c7f4108b11a1b0e3f1108b522c50ffd2c705b0e3f110ffffffffc605342efd1000c3"),
    ]
    verified = []
    for va, expected_hex in checks:
        expected = bytes.fromhex(expected_hex)
        actual = read_va(va, len(expected))
        if actual != expected:
            raise SystemExit(f"byte mismatch at {va:#x}: {actual.hex()} != {expected.hex()}")
        verified.append({"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual)})

    # The first three installed secondary-interface entries match the public
    # IToolSystem contract: name, Init, Shutdown. Keep later slots unresolved.
    table = read_va(0x10AEB01C, 12)
    expected_table = struct.pack("<III", 0x102D2DA0, 0x102CB030, 0x102CFAA0)
    if table != expected_table:
        raise SystemExit(f"IToolSystem prefix mismatch: {table.hex()}")
    ui_keys = [
        (0x102CB247, 0x10AE9158, "FullscreenViewport", "constructed"),
        (0x102CB277, 0x10AE9148, "#BxToolTimeline", "constructed"),
        (0x102CB2A8, 0x10AE9134, "#BxToolClipBrowser", "constructed"),
        (0x102CB2DB, 0x10ACF534, "Properties", "constructed"),
        (0x102CB33F, 0x10AE9118, "#BxToolAnimationSetEditor", "constructed"),
        (0x102CB38E, 0x10AE9100, "#BxToolEngineViewport", "constructed"),
        (0x102CB42D, 0x10AE9100, "#BxToolEngineViewport", "constructed_again"),
        (0x102CB4C5, 0x10AE90F4, "InfoPage", "constructed"),
        (0x102CB597, 0x10AE90DC, "#BxElementProperties", "layout_resolved"),
        (0x102CB5D9, 0x10AE90CC, "#BxFileManager", "layout_resolved"),
        (0x102CB61B, 0x10AE9118, "#BxToolAnimationSetEditor", "layout_resolved"),
        (0x102CB65D, 0x10AE9100, "#BxToolEngineViewport", "layout_resolved"),
        (0x102CB6A2, 0x10AE90B8, "SecondaryViewport", "layout_resolved"),
        (0x102CB6DF, 0x10AE90A8, "#SFMInfoPage", "layout_resolved"),
    ]
    verified_ui_keys = []
    for instruction_va, string_va, text, role in ui_keys:
        expected_push = b"\x68" + struct.pack("<I", string_va)
        actual_push = read_va(instruction_va, 5)
        actual_text = read_va(string_va, len(text.encode("ascii")) + 1)
        if actual_push != expected_push:
            raise SystemExit(f"UI string reference mismatch at {instruction_va:#x}: {actual_push.hex()}")
        if actual_text != text.encode("ascii") + b"\0":
            raise SystemExit(f"UI string mismatch at {string_va:#x}: {actual_text!r}")
        verified_ui_keys.append({
            "instruction_va": f"0x{instruction_va:08x}",
            "string_va": f"0x{string_va:08x}",
            "key": text,
            "init_role": role,
            "instruction_bytes": actual_push.hex()
        })

    # The Additional Editors menu wires localized actions to callback thunks.
    # These checks establish the installed code's action/delegate linkage; the
    # thunk's vtable slot is observed but its runtime implementation is unnamed.
    menu_checks = [
        (0x100AD457, "8b5508c7400c01000000c700d498aa10c74008c898aa10b9508f0a1089481033c98948148950188945e8eb07", "Motion Editor callback delegate fields"),
        (0x100AD48A, "6aff6a5b8d45f0506a006a006a0068320000018d4de4ff1570c7a7108b4de8506a0151", "Motion Editor action registration and localized caption"),
        (0x100AD517, "c7400c01000000c700d498aa10c74008c898aa10b930900a1089481033c98948148b4d088948188945e8eb07", "Graph Editor callback delegate fields"),
        (0x100AD54A, "6aff6a5c8d55f0526a006a006a0068330000018d4de4ff1570c7a71050", "Graph Editor action registration and localized caption"),
        (0x100A8F50, "8b01ffa048010000", "Motion Editor callback thunk to object-vtable byte offset 0x148"),
        (0x100A9030, "8b01ffa04c010000", "Graph Editor callback thunk to object-vtable byte offset 0x14c"),
        (0x100A9130, "8b01ffa050010000", "Clip Editor callback thunk to object-vtable byte offset 0x150"),
    ]
    verified_menu = []
    for va, expected_hex, meaning in menu_checks:
        expected = bytes.fromhex(expected_hex)
        actual = read_va(va, len(expected))
        if actual != expected:
            raise SystemExit(f"Additional Editors menu byte mismatch at {va:#x}: {actual.hex()} != {expected.hex()}")
        verified_menu.append({"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual), "observed": meaning})

    # Preserve the CIFMTool table comparison as a negative control. These
    # same offsets also exist here, but CQSFMController owns the named Qt
    # editor-window slots (see verify_editor_activation_dispatch.py).
    editor_primary_vtable = 0x10AEB0F4
    editor_secondary_vtable = 0x10AEB01C
    expected_primary_slots = {0x148: 0x109A4360, 0x14C: 0x109A4460}
    primary_slot_matches = []
    for byte_offset, expected_target in expected_primary_slots.items():
        actual_target = struct.unpack("<I", read_va(editor_primary_vtable + byte_offset, 4))[0]
        if actual_target != expected_target:
            raise SystemExit(f"CIFMTool primary vtable mismatch at +{byte_offset:#x}: {actual_target:#x} != {expected_target:#x}")
        primary_slot_matches.append({"byte_offset": f"0x{byte_offset:x}", "slot_index": byte_offset // 4, "target": f"0x{actual_target:08x}", "target_bytes": read_va(actual_target, 24).hex()})
    secondary_slots = {}
    for byte_offset in expected_primary_slots:
        secondary_slots[f"0x{byte_offset:x}"] = f"0x{struct.unpack('<I', read_va(editor_secondary_vtable + byte_offset, 4))[0]:08x}"
    constructor_vtable_spans = []
    for va, expected_hex in (
        (0x102D1AEB, "c706f4b0ae10"),
        (0x102D1AF1, "c786440200001cb0ae10"),
    ):
        actual = read_va(va, len(bytes.fromhex(expected_hex)))
        if actual.hex() != expected_hex:
            raise SystemExit(f"CIFMTool constructor vtable write mismatch at {va:#x}: {actual.hex()}")
        constructor_vtable_spans.append({"va": f"0x{va:08x}", "bytes": actual.hex()})

    out = pathlib.Path("outputs/sfm64-editor-tool-init-byte-verification.json")
    out.write_text(json.dumps({
        "target": str(TARGET),
        "sha256": digest,
        "image_base": f"0x{BASE:08x}",
        "architecture": "x86 installed binary; this is reverse-engineering evidence, not x64 code",
        "init_entry": "ifm.dll VTOOLDICTIONARY003 slot 3 at 0x10989cc0",
        "checks": verified,
        "result": "passed",
        "scope": "Selected instruction spans verified directly against the current installed DLL. Function interpretation is limited to visible calls, branches, globals, and string references; it does not verify runtime invocation or visible panel behavior."
    }, indent=2), encoding="utf-8")
    lifecycle = pathlib.Path("outputs/sfm64-editor-lifecycle-notification-audit.json")
    lifecycle.write_text(json.dumps({
        "target": str(TARGET),
        "sha256": digest,
        "startupEntry": "VTOOLDICTIONARY003 Init candidate 0x10989cc0 calls 0x1097c050 when host-global 0x10f4c744 is non-null.",
        "startupNotificationHelper": {
            "entry": "0x1097c050",
            "observed": [
                "Uses host-global 0x10f4c744 and suppresses re-entry with byte 0x10fd2e34.",
                "Obtains and retains a host-provided token from vtable byte offset +0x20.",
                "Walks linked records rooted at 0x10fd2e28; calls each record's vtable offsets +0x0c with global argument 0x10fd2e30, then +0x2c without an explicit argument.",
                "Calls host vtable byte offsets +0x7c and +0xa0 after record notification, then clears the record-list global."
            ],
            "limits": "Host and listener slot semantics are unnamed. This is a lifecycle callback path, not proof of panel creation, docking, or visible UI."
        },
        "shutdownNotificationHelper": {
            "entry": "0x1097c100",
            "observed": [
                "Requires the host global and an active notification token.",
                "Calls host vtable byte offset +0x80 with the retained token.",
                "Calls host vtable byte offset +0x2c with the token, then invalidates the token and clears the active flag."
            ],
            "limits": "The callback meanings and runtime effects remain unknown; no visible panel or window transition is established."
        },
        "verifiedRanges": [x for x in verified if x["va"] in ("0x1097c0b0", "0x1097c100")],
        "result": "passed",
        "architecture": "The inspected SFM DLL is x86; its object layout and byte offsets are not x64 ABI definitions."
    }, indent=2), encoding="utf-8")
    ui = pathlib.Path("outputs/sfm64-editor-init-ui-layout-audit.json")
    ui.write_text(json.dumps({
        "target": str(TARGET),
        "sha256": digest,
        "architecture": "installed editor is x86; addresses and object sizes here are evidence about that build, not x64 ABI values",
        "public_contract_reference": "https://raw.githubusercontent.com/ValveSoftware/source-sdk-2013/master/src/public/toolframework/itoolsystem.h",
        "installed_tool_interface": {
            "vtable_va": "0x10aeb01c",
            "prefix_entries": [
                {"slot": 0, "method": "GetToolName", "target": "0x102d2da0"},
                {"slot": 1, "method": "Init", "target": "0x102cb030"},
                {"slot": 2, "method": "Shutdown", "target": "0x102cfaa0"}
            ],
            "prefix_bytes": table.hex(),
            "status": "The first three installed vtable entries match the public IToolSystem declaration. No assumption is made about later installed slots."
        },
        "init_panel_keys": verified_ui_keys,
        "behavior_summary": "CIFMTool::Init performs factory/runtime setup, constructs or resolves several named UI controls, and associates controls through the editor layout service. The labels identify UI resource keys and do not by themselves prove exact window visibility, docking geometry, restored state, or user interaction behavior.",
        "observed_control_keys": [
            "FullscreenViewport", "#BxToolTimeline", "#BxToolClipBrowser", "Properties",
            "#BxToolAnimationSetEditor", "#BxToolEngineViewport", "InfoPage",
            "#BxElementProperties", "#BxFileManager", "SecondaryViewport", "#SFMInfoPage"
        ],
        "not_yet_recovered": [
            "Visible dock arrangement and panel geometry",
            "Semantic Graph Editor and Motion Editor activation behavior (menu actions and candidate receiver slots are now mapped)",
            "Menu and toolbar assembly",
            "Runtime state restoration and screenshot comparison",
            "Native x64 implementation"
        ],
        "result": "passed"
    }, indent=2), encoding="utf-8")
    menu = pathlib.Path("outputs/sfm64-editor-additional-menu-audit.json")
    menu.write_text(json.dumps({
        "target": str(TARGET),
        "sha256": digest,
        "architecture": "installed editor is x86; observed offsets are not x64 ABI values",
        "entries": [
            {"caption": "Motion Editor", "actionKey": "ShowMotionEditor", "delegateConstruction": "0x100ad457", "actionConstruction": "0x100ad48a", "actionObjectMemberOffset": "0x178", "thunk": "0x100a8f50", "thunkVirtualByteOffset": "0x148"},
            {"caption": "Graph Editor", "actionKey": "ShowGraphEditor", "delegateConstruction": "0x100ad517", "actionConstruction": "0x100ad54a", "actionObjectMemberOffset": "0x17c", "thunk": "0x100a9030", "thunkVirtualByteOffset": "0x14c"},
            {"caption": "Clip Editor", "actionKey": "ShowTimeline", "thunk": "0x100a9130", "thunkVirtualByteOffset": "0x150"}
        ],
        "verifiedRanges": verified_menu,
        "interpretation": "SFM constructs Motion/Graph menu actions, stores callback delegates in editor-object fields, and registers those actions through shared helper calls. Their thunks dispatch at byte offsets 0x148 and 0x14c. The receiving class and method semantics are recovered in outputs/sfm64-editor-action-dispatch-audit.json; runtime menu display, exact pane visibility, dock layout, and restored state remain unverified.",
        "activationDispatchAudit": "outputs/sfm64-editor-action-dispatch-audit.json",
        "result": "passed"
    }, indent=2), encoding="utf-8")
    receiver = pathlib.Path("outputs/sfm64-cifmtool-menu-receiver-candidate.json")
    receiver.write_text(json.dumps({
        "target": str(TARGET), "sha256": digest,
        "architecture": "x86 installed build; vtable and object offsets are not x64 ABI definitions",
        "typeEvidence": {"type": "CIFMTool", "primaryVguiPanelVtable": f"0x{editor_primary_vtable:08x}", "secondaryIToolSystemVtable": f"0x{editor_secondary_vtable:08x}", "constructorWrites": constructor_vtable_spans},
        "cifmToolOffsetComparison": {"primaryVtableTargets": primary_slot_matches, "secondaryIToolSystemTargetsAtSameOffsets": secondary_slots},
        "interpretation": "Rejected receiver hypothesis: CIFMTool happens to have methods at the same byte offsets, but its primary vtable entries are not the handlers for these named editor-window slots. The CQSFMController Qt meta-method table, generated dispatch switch, RTTI and vtable entries provide the receiver/handler evidence in outputs/sfm64-editor-action-dispatch-audit.json.",
        "supersededBy": "outputs/sfm64-editor-action-dispatch-audit.json",
        "result": "superseded"
    }, indent=2), encoding="utf-8")
    print(f"verified {len(verified)} code spans, {len(verified_ui_keys)} editor UI key references, {len(verified_menu)} menu spans; sha256={digest}; outputs={out},{lifecycle},{ui},{menu},{receiver}")


if __name__ == "__main__":
    main()
