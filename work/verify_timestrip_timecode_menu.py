"""Verify the Timestrip Time Code Display menu's mode routing and checks."""
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


def string(va):
    offset = im.offset(va)
    return im.b[offset:im.b.index(b"\0", offset)].decode("ascii")


def check(va, expected_hex, label):
    expected = bytes.fromhex(expected_hex)
    actual = data(va, len(expected))
    if actual != expected:
        raise SystemExit(f"{label} mismatch at {va:#x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual), "label": label}


inventory = json.loads((ROOT / "outputs" / "editor-qt-method-inventory.json").read_text(encoding="utf-8"))
objects = [entry for entry in inventory["objects"] if entry["class_name"] == "CQTimestripWidget"]
if len(objects) != 1 or [(m["local_id"], m["signature"]) for m in objects[0]["methods"]] != [
    (0, "OnCustomContextMenuRequested(QPoint)"),
    (1, "OnKeyPressedEvent(QKeyEvent*)"),
    (2, "OnKeyReleasedEvent(QKeyEvent*)"),
]:
    raise SystemExit("unexpected CQTimestripWidget Qt method inventory")

call = data(0x1007C6A7, 5)
call_target = 0x1007C6A7 + 5 + struct.unpack("<i", call[1:])[0] if call[0] == 0xE8 else 0
if call_target != 0x1019C6E0:
    raise SystemExit("Timestrip context-menu local ID 0 no longer routes to its recorded handler")
if word(0x1019B10B + 2) != 0x10AC75E4:
    raise SystemExit("CQTimestripWidget constructor vtable identity mismatch")

verified = [
    check(0x1027ADE0, "55 8b ec 56 8b f1 8b 0d 1c a5 e9 10 8b 01 8b 90 e4 01 00 00 68 00 80 00 00 6a 00 68 a4 3d ae 10 ff d2 8b 45 08 50 e8 45 b4 e2 ff 8b c8 e8 be 0e e2 ff", "global Set Time Display Mode callback entry and setter dispatch"),
    check(0x1027AE12, "8b 8e 84 06 00 00 6a 00 51 8b 0d 68 25 f3 10 e8 ca 40 04 00", "global set-mode editor refresh"),
    check(0x1027AE40, "56 57 8b f9 8b 0d 1c a5 e9 10 8b 01 8b 90 e4 01 00 00 68 00 80 00 00 6a 00 68 bc 3d ae 10 ff d2 e8 eb b3 e2 ff 8b c8 e8 44 0f e2 ff", "global Next Time Display Mode callback entry and current-mode read"),
    check(0x1027AE6C, "8d 70 01 81 e6 03 00 00 80 79 05 4e 83 ce fc 46", "global next-mode increment and four-value wrap"),
    check(0x1027AE7C, "8b 0d 1c a5 e9 10 8b 01 8b 90 e4 01 00 00 68 00 80 00 00 6a 00 68 a4 3d ae 10 ff d2 56 e8 b2 b3 e2 ff 8b c8 e8 2b 0e e2 ff", "global next-mode dispatch to setter"),
    check(0x1027AEA5, "8b 87 84 06 00 00 8b 0d 68 25 f3 10 6a 00 50 e8 37 40 04 00", "global next-mode editor refresh"),
    check(0x100B4A20, "55 8b ec 83 ec 1c 53 56 b3 20 57 88 5d ec".replace(" ", ""), "time-display shortcut tooltip builder prologue"),
    check(0x100B4A72, "68 c0 c2 aa 10", "tooltip builder reference to time-display shortcut resource"),
    check(0x101B0FE1, "8b 4e 50 89 45 e8 83 c4 08 8d 45 e8 50 e8 2d 3a f0 ff", "Global Time control shortcut-tooltip builder call"),
    check(0x101B11E3, "8d 4d e8 51 8b 4e 54 89 45 e8 e8 2e 38 f0 ff", "Local Shot Time control shortcut-tooltip builder call"),
    check(0x10AAC2C0, "%1 %2[Left] global selection of next time display mode - [Shift][Left] changes only locally".encode("ascii").hex(), "time-display keyboard shortcut tooltip resource"),
    check(0x1019B10B, "c7 06 e4 75 ac 10", "CQTimestripWidget constructor vtable"),
    check(0x1007C660, "55 8b ec 83 7d 0c 00 75 43 8b 45 10 83 e8 00 74 2c", "generated Timestrip Qt local-slot dispatch"),
    check(0x1007C68A, "8b 45 14 8b 48 04 8b 11 8b 4d 08 52 e8 05 e8 11 00", "Timestrip OnKeyPressedEvent local-slot route to shared key-event method"),
    check(0x1007C677, "8b 45 14 8b 48 04 8b 11 8b 4d 08 52 e8 18 e8 11 00", "Timestrip OnKeyReleasedEvent local-slot route to shared key-event method"),
    check(0x1019AEA0, "8b 49 1c 8b 01 8b 50 10 ff d2 8b c8 ff 15 c0 ba a7 10 c2 04 00", "complete shared Timestrip key-event slot method"),
    check(0x1007C69D, "8b 45 14 8b 48 04 51 8b 4d 08 e8 34 00 12 00", "Timestrip context-menu callback call"),
    check(0x1019C6E0, "55 8b ec 83 ec 54 53 56 57 33 db 6a 14", "Timestrip context-menu handler prologue"),
    check(0x1019C903, "68 80 7b ac 10", "Time Code Display submenu label load"),
    check(0x1009BCD0, "55 8b ec 83 ec 18 53 56", "Global time-display mode callback prologue"),
    check(0x1009BCDD, "39 b3 f8 00 00 00", "Global callback stored-mode comparison at owner offset +0xf8"),
    check(0x1009BCFD, "68 24 8a aa 10 ff d7", "Global TimeDisplayMode setting key load and lookup"),
    check(0x1009BD3A, "6a ff 68 1c 8a aa 10", "Global settings scope string"),
    check(0x1009BD86, "89 b3 f8 00 00 00", "Global mode publication at owner offset +0xf8"),
    check(0x1009BD92, "56 e8 88 32 22 00", "Global mode listener notification call"),
    check(0x1019C97F, "c7 45 d4 00 00 00 00", "Total Seconds callback mode value"),
    check(0x1019C9AC, "b8 d0 bc 09 10", "mode-zero SetTimeDisplayMode callback argument"),
    check(0x1019CA5E, "c7 45 d4 01 00 00 00", "Total Frames callback mode value"),
    check(0x1019CA8E, "b8 d0 bc 09 10", "mode-one SetTimeDisplayMode callback argument"),
    check(0x1019CB3D, "c7 45 d4 02 00 00 00", "Timecode:Seconds callback mode value"),
    check(0x1019CB70, "b8 d0 bc 09 10", "mode-two SetTimeDisplayMode callback argument"),
    check(0x1019CC21, "c7 45 d4 03 00 00 00", "Timecode:Frames callback mode value"),
    check(0x1019CC28, "c7 45 c4 d0 bc 09 10", "mode-three SetTimeDisplayMode callback argument"),
    check(0x1019CA25, "83 7e 24 00", "mode-zero checked-state comparison"),
    check(0x1019CA2C, "0f 94 c2", "mode-zero checked-state result"),
    check(0x1019CB04, "83 7e 24 01", "mode-one checked-state comparison"),
    check(0x1019CB08, "0f 94 c0", "mode-one checked-state result"),
    check(0x1019CBE8, "83 7e 24 02", "mode-two checked-state comparison"),
    check(0x1019CBEC, "0f 94 c1", "mode-two checked-state result"),
    check(0x1019CCDB, "83 7e 24 03", "mode-three checked-state comparison"),
    check(0x1019CCDF, "0f 94 c0", "mode-three checked-state result"),
    check(0x1019CD0F, "68 3c c4 aa 10", "Local mode subgroup label load"),
    check(0x1019AB20, "55 8b ec 83 ec 18 8b 45 08 56 57 8b f1", "Local time-display mode setter prologue"),
    check(0x1019AB32, "89 46 24", "Local mode publication at Timestrip offset +0x24"),
    check(0x1019ABC4, "8b ce ff 15 c0 ba a7 10", "Local mode listener notification call"),
    check(0x1019CD81, "c7 45 f8 00 00 00 00", "Local Total Seconds mode value"),
    check(0x1019CDBC, "b8 20 ab 19 10", "Local mode-zero setter callback"),
    check(0x1019CE35, "83 7e 24 00", "Local mode-zero checked-state comparison"),
    check(0x1019CE6E, "c7 45 c8 01 00 00 00", "Local Total Frames mode value"),
    check(0x1019CE9E, "b8 20 ab 19 10", "Local mode-one setter callback"),
    check(0x1019CF20, "83 7e 24 01", "Local mode-one checked-state comparison"),
    check(0x1019CF59, "c7 45 c8 02 00 00 00", "Local Timecode:Seconds mode value"),
    check(0x1019CF89, "b8 20 ab 19 10", "Local mode-two setter callback"),
    check(0x1019D00B, "83 7e 24 02", "Local mode-two checked-state comparison"),
    check(0x1019D044, "c7 45 c8 03 00 00 00", "Local Timecode:Frames mode value"),
    check(0x1019D04B, "c7 45 b4 20 ab 19 10", "Local mode-three setter callback"),
    check(0x1019D0EE, "83 7e 24 03", "Local mode-three checked-state comparison"),
    check(0x1019C7E2, "48 83 f8 05 0f 87 72 09 00 00 ff 24 85 68 d1 19 10", "six-way hit-test result normalization and jump dispatch"),
    check(0x1019C7F3, "8b 4e 1c 8b 11 8b 52 30 e9 e2 00 00 00", "hit-test branch 1 routes through owner slot +0x30"),
    check(0x1019C800, "8b 55 e8 8b 4e 1c 8b 01 8b 40 2c 52 8b 55 f4 56 52 ff d0", "hit-test branch 2 routes through owner slot +0x2c"),
    check(0x1019C818, "8b 4e 1c 8b 11 8b 52 34 e9 bd 00 00 00", "hit-test branch 3 routes through owner slot +0x34"),
    check(0x1019C825, "8b 4e 1c 8b 01 8b 50 0c ff d2 6a 08", "hit-test branches 4-6 enter toggle-action builder"),
    check(0x1019C870, "b8 9c 7b ac 10 75 05 b8 94 7b ac 10", "toggle-action label selection: Bookmarks or Keys"),
    check(0x1019C8D5, "e8 06 5d f1 ff 8b 4e 1c 8b 11 8b 52 38", "toggle-action path joins owner slot +0x38 dispatch"),
    check(0x1019D168, "18 c8 19 10 f3 c7 19 10 18 c8 19 10 00 c8 19 10 25 c8 19 10 25 c8 19 10", "six hit-test jump-table entries"),
]

hit_test_labels = {
    0x10AC7B94: "Keys",
    0x10AC7B9C: "Bookmarks",
    0x10AC7B80: "Time Code Display",
    0x10AAC43C: "Local",
}
for label_va, expected_label in hit_test_labels.items():
    if string(label_va) != expected_label:
        raise SystemExit(f"Timestrip menu label mismatch at {label_va:#x}: {string(label_va)!r}")
dispatch_table = [word(0x1019D168 + 4 * i) for i in range(6)]
expected_dispatch = [0x1019C818, 0x1019C7F3, 0x1019C818, 0x1019C800, 0x1019C825, 0x1019C825]
if dispatch_table != expected_dispatch:
    raise SystemExit(f"Timestrip hit-test dispatch table mismatch: {[hex(x) for x in dispatch_table]}")

mode_labels = [
    (0, 0x10A90B88, "Total Seconds"),
    (1, 0x10A90B78, "Total Frames"),
    (2, 0x10A90B64, "Timecode:Seconds"),
    (3, 0x10A90B54, "Timecode:Frames"),
]
if (string(0x10AA8A1C), string(0x10AA8A24)) != ("Global", "TimeDisplayMode"):
    raise SystemExit("global time-display setting scope/key mismatch")
provider_audit = json.loads((ROOT / "outputs" / "sfm64-timestrip-action-provider-audit.json").read_text(encoding="utf-8"))
callback_audit = json.loads((ROOT / "outputs" / "sfm64-timestrip-add-bookmark-callback-audit.json").read_text(encoding="utf-8"))
if provider_audit.get("sha256") != digest or provider_audit.get("result") != "passed":
    raise SystemExit("linked action-provider audit is missing or for a different binary")
if callback_audit.get("sha256") != digest or callback_audit.get("result") != "passed":
    raise SystemExit("linked Add Bookmark callback audit is missing or for a different binary")
modes = []
for mode, label_va, expected_label in mode_labels:
    actual = string(label_va)
    if actual != expected_label:
        raise SystemExit(f"time display mode label mismatch at {label_va:#x}: {actual!r}")
    modes.append({"value": mode, "label": actual, "checkedWhenStoredModeEquals": mode})

report = {
    "target": str(TARGET),
    "sha256": digest,
    "evidenceType": "installed x86 ifm.dll byte verification; not an x64 ABI description",
    "class": "CQTimestripWidget",
    "qtSignalSlot": "OnCustomContextMenuRequested(QPoint)",
    "handler": "0x1019c6e0",
    "shortcutResource": {
        "va": "0x10aac2c0",
        "text": string(0x10AAC2C0),
        "evidence": "byte-verified static resource; runtime placeholder values and key routing are not verified",
    },
    "shortcutTooltipConstruction": {
        "function": "0x100b4a20",
        "verifiedCallsites": ["0x100b4a72", "0x101b0fee", "0x101b11ed"],
        "controls": ["Global Time", "Local Shot Time"],
        "finding": "The installed x86 UI initializer invokes the tooltip builder for controls labeled Global Time and Local Shot Time; the builder loads the verified shortcut resource.",
        "limit": "does not prove the tooltip is currently visible or that Left/Shift+Left dispatch to the mode setter",
    },
    "globalModeActions": {
        "setCallback": "0x1027ade0",
        "nextCallback": "0x1027ae40",
        "setActionName": string(0x10AE3DA4),
        "nextActionName": string(0x10AE3DBC),
        "finding": "The registered global Next callback reads the current mode, increments it with four-value wraparound, and dispatches Set Time Display Mode; both callbacks refresh editor state.",
        "limit": "These byte-verified command callbacks do not prove the tooltip's Left/Shift+Left keys route to them, nor do they establish the separate local-cycle shortcut path."
    },
    "keyEventSlots": {
        "class": "CQTimestripWidget",
        "methods": ["OnKeyPressedEvent(QKeyEvent*)", "OnKeyReleasedEvent(QKeyEvent*)"],
        "qtMetacall": "0x1007c660",
        "sharedMethod": "0x1019aea0",
        "finding": "Generated Qt local-slot dispatch maps local IDs 1 and 2 to the same shared method and passes the QKeyEvent pointer. The complete 21-byte method body does not read its stack argument; it queries the associated interface through vtable offset +0x10 and calls an imported service.",
        "limit": "This shared slot is not evidence of key-specific routing: it contains no key-code or modifier branch. Whether Left/Shift+Left is handled through another event path and reaches Global/Local mode actions remains unresolved."
    },
    "actionProvider": {
        "audit": "outputs/sfm64-timestrip-action-provider-audit.json",
        "function": "0x100b25e0",
        "coverage": "complete 1,869-byte body / 645 instructions revalidated against the same installed ifm.dll",
        "summary": "Bookmark, curve and key actions; singular/plural command variants depend on selection-count branches.",
        "tracedCallback": {
            "label": "Add Bookmark at Head",
            "registeredCallback": "0x100b1570",
            "audit": "outputs/sfm64-timestrip-add-bookmark-callback-audit.json",
            "scope": "Time acquisition/conversion and dispatch into the shared Add Bookmark At Time helper are byte-verified; downstream transaction/result remains untested.",
        },
        "limitation": "Other action callback side effects, live popup order/presentation, and runtime menu selection are not verified.",
    },
    "menuGroups": [
        {
            "label": "Time Code Display",
            "settingScope": string(0x10AA8A1C),
            "settingKey": string(0x10AA8A24),
            "callback": "0x1009bcd0",
            "observedEffect": "updates the setting owner field +0xf8 and calls the global time-display listener service",
            "options": modes,
        },
        {
            "label": string(0x10AAC43C),
            "callback": "0x1019ab20",
            "observedEffect": "writes the Timestrip object's mode field +0x24 and calls its listener service",
            "options": modes,
        },
    ],
    "hitTestContext": {
        "normalization": "The handler decrements the hit-test result and dispatches indices 0..5; other values take the default route.",
        "dispatchTable": [f"0x{entry:08x}" for entry in dispatch_table],
        "branches": [
            {"indices": [0, 2], "verifiedAction": "invoke owner virtual slot byte offset +0x34"},
            {"indices": [1], "verifiedAction": "invoke owner virtual slot byte offset +0x30"},
            {"indices": [3], "verifiedAction": "invoke owner virtual slot byte offset +0x2c with context arguments"},
            {"indices": [4, 5], "verifiedAction": "build checkable action using Keys or Bookmarks label from a boolean virtual query, then invoke owner virtual slot byte offset +0x38"},
        ],
        "limitations": "The semantic names and runtime effects of the owner virtual slots are not established; Keys/Bookmarks are verified labels, not proof of the exact state mutation.",
    },
    "checkedState": {
        "observed": "Both option groups compare the current Timestrip object's field +0x24 against values 0..3 when setting checked state.",
        "caveat": "The global callback stores its value separately at owner offset +0xf8. Runtime behavior must establish how that state relates to the Timestrip field used for these checkmarks.",
    },
    "verifiedRanges": verified,
    "limitations": [
        "This trace recovers the six-way static dispatch, labels and callback storage; it does not dynamically open the menu or test a user's selection.",
        "Exact menu nesting, branch availability, and the meaning/effects of owner virtual slots +0x2c, +0x30, +0x34, and +0x38 remain unresolved.",
        "The global setting's persistence/session behavior and exact display formatting are not verified here.",
        "The installed target is x86; this is not an x64 object-layout or ABI specification.",
        "No replacement-editor screenshot is available because the x64 editor UI remains unimplemented.",
    ],
    "result": "passed",
}
out = ROOT / "outputs" / "sfm64-timestrip-timecode-menu-audit.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")

worklist_path = ROOT / "outputs" / "editor-priority-handler-map.json"
worklist = json.loads(worklist_path.read_text(encoding="utf-8"))
matches = [entry for entry in worklist["handlers"] if entry.get("class_name") == "CQTimestripWidget" and entry.get("signature") == "OnCustomContextMenuRequested(QPoint)"]
if len(matches) != 1:
    raise SystemExit(f"expected one CQTimestripWidget context-menu worklist row, got {len(matches)}")
matches[0]["status"] = "Global/Local time-display paths and six hit-test branches (including Keys/Bookmarks labels), option values 0..3, callback storage and checked-state code byte-verified; virtual-action semantics and runtime selection/persistence remain unverified"
matches[0].setdefault("evidence", [])
evidence_path = "outputs/sfm64-timestrip-timecode-menu-audit.json"
if evidence_path not in matches[0]["evidence"]:
    matches[0]["evidence"].append(evidence_path)
provider_evidence_path = "outputs/sfm64-timestrip-action-provider-audit.json"
if provider_evidence_path not in matches[0]["evidence"]:
    matches[0]["evidence"].append(provider_evidence_path)
callback_evidence_path = "outputs/sfm64-timestrip-add-bookmark-callback-audit.json"
if callback_evidence_path not in matches[0]["evidence"]:
    matches[0]["evidence"].append(callback_evidence_path)
worklist_path.write_text(json.dumps(worklist, indent=2), encoding="utf-8")

# These older inventories predate the now-verified popup/provider linkage.
key_inventory_path = ROOT / "outputs" / "graph-editor-key-command-inventory.json"
key_inventory = json.loads(key_inventory_path.read_text(encoding="utf-8"))
key_inventory["context"] = "This key-action fragment is part of the full 0x100B25E0-0x100B2D2C provider, which also handles Bookmark and Curve commands. The provider is directly called by the verified CQTimestripWidget custom-context popup builder; that base widget is instantiated for Graph Editor upper/lower strips."
key_inventory["caller_and_surface_linkage"] = [
    "outputs/timestrip-custom-context-popup-linkage-verification.json",
    "outputs/sfm64-timestrip-action-provider-audit.json",
]
key_inventory["limitations"] = [
    "The TimeStrip custom-context route is byte-linked to this provider, but exact live menu presentation/order and user selection were not exercised.",
    "Action enablement and callback side effects require separate tracing; Add Bookmark at Head is the only callback traced in outputs/sfm64-timestrip-add-bookmark-callback-audit.json.",
    "This action provider does not exhaust other key, curve, tangent, or editor-area context menus.",
    "No live SFM runtime menu or replacement-editor screenshot was available.",
]
key_inventory["menu_provider_evidence"] = "The provider is byte-linked to the TimeStrip custom-context popup through callsite 0x1019C8D5. Its Qt imports confirm QWidget::addAction, QMenu::addSeparator, QAction::setText and QAction::setEnabled. Runtime popup presentation remains untested."
key_inventory_path.write_text(json.dumps(key_inventory, indent=2), encoding="utf-8")

paste_inventory_path = ROOT / "outputs" / "graph-editor-bookmark-curve-paste-inventory.json"
paste_inventory = json.loads(paste_inventory_path.read_text(encoding="utf-8"))
paste_inventory["additional_cross_reference"] = "The shared Bookmark/Curve provider is directly called by the verified TimeStrip custom-context handler at 0x1019C8D5, which constructs the menu and selects Bookmark/Curve/key actions from current context. The AddBookmark callback registration also confirms action-to-callback construction for one command. See outputs/sfm64-timestrip-action-provider-audit.json and outputs/sfm64-timestrip-add-bookmark-callback-audit.json."
paste_inventory["limits"] = [
    "Static TimeStrip popup linkage is proven; exact live menu contents/order and user selection are untested.",
    "The paste callback effects and saved bookmark/curve result remain unverified.",
    "No live SFM screenshot is available.",
]
paste_inventory_path.write_text(json.dumps(paste_inventory, indent=2), encoding="utf-8")
print(f"verified {len(verified)} Timestrip menu code spans and four display modes; sha256={digest}")
