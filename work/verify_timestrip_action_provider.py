"""Revalidate the action provider reached from the Timestrip context popup."""
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
image = Image(TARGET)
digest = hashlib.sha256(image.b).hexdigest()
if digest != EXPECTED_SHA256:
    raise SystemExit(f"installed ifm.dll changed: {digest}")

provider_path = ROOT / "outputs" / "graph-editor-interaction-action-provider-verification.json"
provider = json.loads(provider_path.read_text(encoding="utf-8"))
if provider["binary_sha256"] != digest or provider["function_start"] != "0x100B25E0" or provider["function_end_exclusive"] != "0x100B2D2D":
    raise SystemExit("existing action-provider evidence does not match the current target/function")
callback_path = ROOT / "outputs" / "sfm64-timestrip-add-bookmark-callback-audit.json"
callback_report = json.loads(callback_path.read_text(encoding="utf-8"))
if callback_report.get("sha256") != digest or callback_report.get("result") != "passed":
    raise SystemExit("Add Bookmark callback receipt is missing or does not match installed ifm.dll")
if callback_report.get("actionRegistration", {}).get("callback") != "0x100B1570":
    raise SystemExit("Add Bookmark callback receipt targets an unexpected handler")

instructions = provider["instructions"]
if len(instructions) != 645 or provider["instruction_count"] != len(instructions):
    raise SystemExit("unexpected action-provider instruction inventory")
expected_va = int(provider["function_start"], 16)
verified_bytes = 0
for entry in instructions:
    va = int(entry["va"], 16)
    raw = bytes.fromhex(entry["bytes"])
    if va != expected_va:
        raise SystemExit(f"action-provider instruction gap/overlap at {va:#x}, expected {expected_va:#x}")
    offset = image.offset(va)
    if image.b[offset:offset + len(raw)] != raw:
        raise SystemExit(f"action-provider bytes differ from installed DLL at {va:#x}")
    expected_va += len(raw)
    verified_bytes += len(raw)
if expected_va != int(provider["function_end_exclusive"], 16) or verified_bytes != provider["verified_bytes"]:
    raise SystemExit("action-provider instruction coverage does not exactly span the recorded function")

# Verify the Timestrip popup's direct call target into this exact provider.
call_va = 0x1019C8D5
call_raw = image.b[image.offset(call_va):image.offset(call_va) + 5]
if call_raw[0] != 0xE8:
    raise SystemExit("Timestrip-to-action-provider callsite is no longer a relative call")
call_target = call_va + 5 + struct.unpack("<i", call_raw[1:])[0]
if call_target != 0x100B25E0:
    raise SystemExit(f"Timestrip action-provider call target changed: {call_target:#x}")

# Extract literal QString labels passed to the provider and verify their bytes.
literal_strings = {}
for entry in instructions:
    raw = bytes.fromhex(entry["bytes"])
    if len(raw) >= 5 and raw[0] == 0x68:  # push imm32
        ptr = struct.unpack_from("<I", raw, 1)[0]
        try:
            value = image.string(ptr)
        except (ValueError, UnicodeDecodeError):
            continue
        literal_strings.setdefault(value, []).append(f"0x{int(entry['va'], 16):08x}")

expected_labels = {
    "Bookmark", "Curve", "Delete Bookmark", "Select All", "Clear Bookmark Selection",
    "Add Bookmark at Head", "Cut Bookmark", "Copy Bookmark", "Cut Bookmarks",
    "Copy Bookmarks", "Delete Bookmarks", "Paste Bookmark", "Paste Bookmarks",
    "Paste Bookmark from Curve", "Paste Bookmarks from Curves", "Cut Key", "Copy Key",
    "Cut Keys", "Copy Keys", "Paste Key", "Paste Keys", "Paste Key from Bookmark Time",
    "Paste Keys from Bookmark Times", "Clear Key Selection",
}
missing = sorted(expected_labels - literal_strings.keys())
if missing:
    raise SystemExit(f"action-provider labels are missing from installed code references: {missing}")
paste_curves_va = 0x10AABE74
if image.string(paste_curves_va) != "Paste Curves":
    raise SystemExit("Paste Curves localized-label string mismatch")

expected_imports = {
    "0x10A7BC24": "QWidget::addAction(QAction*)",
    "0x10A7C430": "QMenu::addSeparator()",
    "0x10A7CABC": "QAction::setEnabled(bool)",
    "0x10A7CCA8": "QAction::setText(QString const&)",
}
actual_imports = {entry["iat_va"]: entry["symbol"] for entry in provider["qt_imports"]}
for va, symbol in expected_imports.items():
    if actual_imports.get(va) != symbol:
        raise SystemExit(f"Qt import identity mismatch at {va}: {actual_imports.get(va)!r}")

report = {
    "target": str(TARGET),
    "sha256": digest,
    "evidenceType": "full action-provider instruction-byte revalidation against installed x86 ifm.dll",
    "providerFunction": {
        "start": provider["function_start"],
        "endExclusive": provider["function_end_exclusive"],
        "instructionCount": len(instructions),
        "verifiedBytes": verified_bytes,
        "contiguousCoverage": True,
    },
    "timestripCallsite": {
        "va": f"0x{call_va:08x}",
        "bytes": call_raw.hex(),
        "target": f"0x{call_target:08x}",
    },
    "actionGroups": provider["actions"],
    "tracedCallbacks": [
        {
            "commandKey": callback_report["actionRegistration"]["commandKey"],
            "displayLabel": callback_report["actionRegistration"]["displayLabel"],
            "callback": callback_report["callback"]["function"],
            "callbackAudit": "outputs/sfm64-timestrip-add-bookmark-callback-audit.json",
            "observedBehavior": callback_report["callback"]["observedBehavior"],
            "limitation": "Exact time-source semantics and downstream transaction result remain unverified.",
        }
    ],
    "referencedLiteralLabels": {label: sites for label, sites in sorted(literal_strings.items()) if label in expected_labels},
    "additionalResolvedLabel": {"va": f"0x{paste_curves_va:08x}", "text": image.string(paste_curves_va), "source": "provider's installed string table; its reference is through a stored action updated with QAction::setText"},
    "verifiedQtImports": [{"iat_va": va, "symbol": symbol} for va, symbol in expected_imports.items()],
    "observations": provider["observations"],
    "limitations": [
        "Selection-count branches and QAction construction are static findings; live selection counts and actual visible menu contents/order were not exercised.",
        "The shared provider's menu/action object presentation is tied to the Timestrip popup by the direct callsite, but this does not establish how every item is arranged by Qt at runtime.",
        "Callback side effects, undo history, and save/reopen results require separate tracing.",
        "The target is 32-bit x86; this is not an x64 ABI or replacement implementation.",
    ],
    "result": "passed",
}
out = ROOT / "outputs" / "sfm64-timestrip-action-provider-audit.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
print(f"verified complete {verified_bytes}-byte provider ({len(instructions)} instructions), {len(expected_labels)} action labels, and direct Timestrip callsite; sha256={digest}")
