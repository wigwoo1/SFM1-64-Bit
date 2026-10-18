"""Byte-check only the installed Copy AnimationSet(s) callback boundary."""
import hashlib, json, re, sys
from pathlib import Path
sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
expected_sha = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
image = Image(binary)
digest = hashlib.sha256(image.b).hexdigest()
if digest != expected_sha:
    raise SystemExit(f"installed ifm.dll changed: {digest}")

start, end = 0x10092010, 0x100922A4
rows, current, entered = [], None, False
for line in Path("work/ifm-disassembly.txt").open(errors="replace"):
    m = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
    if m:
        va = int(m.group(1), 16)
        if va < start:
            continue
        entered = True
        if va >= end:
            break
        if current:
            rows.append(current)
        current = [va, bytes.fromhex(m.group(2))]
    elif entered and current and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
        current[1] += bytes.fromhex(line.strip())
if current:
    rows.append(current)
raw = bytearray()
cursor = start
for va, chunk in rows:
    if va != cursor or image.b[image.offset(va):image.offset(va) + len(chunk)] != chunk:
        raise SystemExit(f"disassembly mismatch or gap at {hex(cursor)}")
    raw.extend(chunk)
    cursor += len(chunk)
if cursor != end:
    raise SystemExit(f"incomplete handler range: ended at {hex(cursor)}")

strings = {
    "warning_title": image.string(0x10AA7A74),
    "warning_body": image.string(0x10AA7A90),
}
expected = {
    "warning_title": "Copy AnimationSet Warning",
    "warning_body": "Some of the copied animation sets have locked controls or rigs on them, which could cause problems when pasting them." + chr(10) + "Do you want to continue?",
}
if strings != expected:
    raise SystemExit(f"warning strings mismatch: {strings!r}")
code = bytes(raw)
# The same helper has a second direct caller. Verify that callsite bytes and its rel32 target.
other_call_va = 0x101D272D
other_call = image.b[image.offset(other_call_va):image.offset(other_call_va) + 5]
other_call_target = other_call_va + 5 + int.from_bytes(other_call[1:5], "little", signed=True) if other_call[:1] == b"\xE8" else 0
def checked_callsite(va, expected_bytes, expected_target):
    actual = image.b[image.offset(va):image.offset(va) + 5]
    target = va + 5 + int.from_bytes(actual[1:5], "little", signed=True) if actual[:1] == b"\xE8" else 0
    return actual == bytes.fromhex(expected_bytes) and target == expected_target

shared_transform_warning_calls = {
    "shared_transform_to_warning_scan": checked_callsite(0x101DB148, "E8C3085500", 0x1072BA10),
    "warning_scan_resolves_control_node": checked_callsite(0x1072BA55, "E8B68FDAFF", 0x104D4A10),
    "warning_scan_calls_recursive_predicate": checked_callsite(0x1072BA5B, "E80005EAFF", 0x105CBF60),
}
findings = {
    "registered_callback_address": "0x10092010",
    "callback_range_matches_installed_binary": True,
    "callback_calls_unresolved_helper_0x101db030": bytes.fromhex("E8938F1400") in code,
    "same_helper_directly_called_at_0x101d272d": other_call == bytes.fromhex("E8FE880000") and other_call_target == 0x101DB030,
    "callback_uses_two_selection_conversion_paths": bytes.fromhex("E8D0B3FFFF") in code and bytes.fromhex("E8B9C0FFFF") in code,
    "callback_calls_predicate_0x105cbf60": bytes.fromhex("E8C69D5300") in code,
    "callback_calls_predicate_0x1072ee00": bytes.fromhex("E85ECC6900") in code,
    "callback_references_warning_title_and_body": bytes.fromhex("68907AAA10") in code and bytes.fromhex("68747AAA10") in code,
    "shared_vector_transform_calls_warning_scan": all(shared_transform_warning_calls.values()),
    "adjacent_reattach_callback_excluded": True,
}
checks = [k for k in findings if k not in ("registered_callback_address", "callback_range_matches_installed_binary", "adjacent_reattach_callback_excluded")]
receipt = {
    "binary": str(binary), "sha256": digest,
    "method": "Static x86 callback byte verification; disassembly bytes checked against installed ifm.dll",
    "range": {"start": hex(start), "end_exclusive": hex(end), "bytes": len(code)},
    "warning_strings": strings,
    "shared_transform_warning_calls": {
        "0x101db148_to_0x1072ba10": shared_transform_warning_calls["shared_transform_to_warning_scan"],
        "0x1072ba55_to_0x104d4a10": shared_transform_warning_calls["warning_scan_resolves_control_node"],
        "0x1072ba5b_to_0x105cbf60": shared_transform_warning_calls["warning_scan_calls_recursive_predicate"],
    },
    "verified_findings": findings,
    "behavior_supported": [
        "The registered Copy AnimationSet(s) callback begins at 0x10092010 and contains two selection conversion paths.",
        "The callback calls helper 0x101DB030 and predicates 0x105CBF60 and 0x1072EE00; their exact semantics are unresolved by this check.",
        "The same helper is directly called at 0x101D272D by another routine, so it is not established as a copy-specific serializer.",
        "The shared helper calls 0x1072BA10, which resolves collection items and calls the same recursive control predicate 0x105CBF60 used by the copy callback; this establishes a warning scan within the shared transformation path, not the complete clipboard payload.",
        "The callback references a warning stating that copied animation sets may have locked controls or rigs, then asks whether to continue.",
    ],
    "limits": [
        "This does not establish the complete transformation performed by helper 0x101DB030, exact selection filtering, clipboard contents, paste behavior, or undo effects.",
        "The adjacent function at 0x100922B0 and helper at 0x10226810 are excluded; their relationship to this callback is not established here.",
        "Static binary evidence only; no live menu, clipboard, or paste result was observed.",
    ],
}
receipt["all_match"] = all(findings[k] for k in checks)
Path("outputs/copy-animation-sets-verification.json").write_text(json.dumps(receipt, indent=2) + chr(10), encoding="utf-8")
print(json.dumps({"sha256": digest, "range_bytes": len(code), "warning_strings_match": True, "verified_findings": findings, "all_match": receipt["all_match"]}, indent=2))
if not receipt["all_match"]:
    sys.exit(1)




