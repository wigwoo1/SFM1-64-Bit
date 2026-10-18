"""Byte-check native Rename Dag and Rename AnimationSet callback paths."""
import hashlib
import json
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
disassembly = Path("work/ifm-disassembly.txt")
expected_sha = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
image = Image(binary)
digest = hashlib.sha256(image.b).hexdigest()
if digest != expected_sha:
    raise SystemExit(f"installed ifm.dll changed: {digest}")

def check_span(va, expected_hex, label):
    expected = bytes.fromhex(expected_hex)
    actual = image.b[image.offset(va):image.offset(va) + len(expected)]
    if actual != expected:
        raise SystemExit(f"{label} mismatch at 0x{va:08x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual), "label": label}

spans = [
    check_span(0x101D6930,
        "55 8b ec 83 ec 4c a1 48 15 f2 10 33 c5 89 45 fc 53 56 8b 75 08 57 85 f6 0f 84 59 03 00 00 8b ce e8 fb c1 2e 00 89 45 c0 85 c0 0f 84 47 03 00 00",
        "Rename Dag null guard and DAG context lookup"),
    check_span(0x101D698E,
        "6a ff 68 f0 f7 ac 10 c6 45 fb 00 ff d7 6a ff 68 14 6b aa 10 89 45 cc ff d7 83 c4 0c 89 45 d0",
        "Rename Dag New Name prompt title and label construction"),
    check_span(0x101D69AD,
        "8b c4 c7 00 00 00 00 00 8d 45 fb 50 8d 4d f0 51 6a 00 8d 55 cc 52 8d 45 d0 50 8d 4d f4 6a 00 51 ff 15 48 cd a7 10",
        "Rename Dag input dialog dispatch"),
    check_span(0x101D6C17,
        "8b 0d 1c a5 e9 10 8b 01 8b 90 08 01 00 00 6a 00 68 14 6b aa 10 68 14 6b aa 10 ff d2",
        "Rename Dag named operation begin"),
    check_span(0x101D6C54,
        "ff d2 8b 4d 08 8b 49 18 8d 45 dc 50 e8 9b f4 18 00",
        "Rename Dag writes the accepted name through the datamodel helper"),
    check_span(0x101D6C7E,
        "8b 0d 1c a5 e9 10 8b 11 8b 82 0c 01 00 00 ff d0",
        "Rename Dag named operation completion"),
    check_span(0x101D74C0,
        "55 8b ec 83 ec 50 a1 48 15 f2 10 33 c5 89 45 fc 53 56 8b 75 08 57 85 f6 0f 84 36 04 00 00",
        "Rename AnimationSet null guard"),
    check_span(0x101D74DE,
        "8b 0d 1c a5 e9 10 8b 01 8b 80 c8 00 00 00 68 a4 7d aa 10 8d 55 b0 52 ff d0 8b 00 6a 00 6a 01 51 8b cc 56 89 01 e8 a8 60 fa ff",
        "Rename AnimationSet resolves animationSets context"),
    check_span(0x101D7544,
        "6a ff 68 f0 f7 ac 10 c6 45 fb 00 ff d7 6a ff 68 d8 6a aa 10 89 45 cc ff d7 83 c4 0c 89 45 d8",
        "Rename AnimationSet New Name prompt title and label construction"),
    check_span(0x101D7575,
        "8d 45 cc 50 8d 4d d8 51 8d 55 f4 6a 00 52 ff 15 48 cd a7 10",
        "Rename AnimationSet input dialog dispatch"),
    check_span(0x101D77C3,
        "8b 0d 1c a5 e9 10 8b 11 8b 82 08 01 00 00 6a 00 68 d8 6a aa 10 68 d8 6a aa 10 ff d0",
        "Rename AnimationSet named operation begin"),
    check_span(0x101D78AE,
        "53 e8 dc 1b 55 00 8b f0 83 c4 04 85 f6 74 2e",
        "Rename AnimationSet optional related-element resolver call"),
    check_span(0x101D78DF,
        "8b 4e 18 8d 45 e8 50 e8 15 e8 18 00",
        "Rename AnimationSet applies the same accepted name to a resolved related element"),
    check_span(0x10729490,
        "55 8b ec 8b 4d 08 83 ec 0c 53 56 57 85 c9 0f 84 8f 00 00 00 e8 17 9d 8d ff",
        "Related-element resolver null guard and selection collection lookup"),
    check_span(0x107294C0,
        "8b 03 8b 0d 1c a5 e9 10 8b 11 8d 04 b8 8b 00 50 8b 42 48 ff d0 8b f0 85 f6 74 52",
        "Related-element resolver walks selection entries"),
    check_span(0x107294F3,
        "8d 4d f4 51 56 e8 a3 f7 ff ff 83 c4 08 33 f6 8b 44 b5 f4 85 c0 74 1d",
        "Related-element resolver extracts candidate relation records"),
    check_span(0x1072950A,
        "6a 00 6a 00 51 83 c0 04 8b cc 50 c7 01 ff ff ff ff e8 f0 08 9f ff 83 c4 10 85 c0 75 0e",
        "Related-element resolver type-checks extracted candidates"),
    check_span(0x10728CA0,
        "55 8b ec 56 8b 75 08 57 85 f6 74 3b 51 8b 0d 0c 16 eb 10 8b c4 89 08 8b 16 8b 42 10 8b ce ff d0 84 c0 74 23 8b ce e8 65 31 da ff 8b ce 8b f8 e8 7c 31 da ff 8b 4d 0c 89 39 89 41 04 c7 41 08 00 00 00 00",
        "Related-record extractor interface branch and two-slot result"),
    check_span(0x10728CE7,
        "6a 00 68 a8 53 aa 10 8b ce e8 0b 53 c3 ff 8b ce 84 c0 74 29 8b 7d 0c 68 d8 54 aa 10 c7 07 00 00 00 00 e8 f2 ed 8f ff 68 c4 54 aa 10 8b ce 89 47 08 e8 e3 ed",
        "Related-record extractor probes rightValue and reads paired channel fields"),
    check_span(0x10728D24,
        "68 58 e0 a8 10 e8 d2 ed 8f ff 8b 4d 0c 5f 89 01 c7 41 04 00 00 00 00 c7 41 08 00 00 00 00",
        "Related-record extractor fallback to a single channel field"),
]

strings = {
    "prompt": image.string(0x10ACF7F0),
    "renameDag": image.string(0x10AA6B14),
    "renameAnimationSet": image.string(0x10AA6AD8),
    "animationSetsAttribute": image.string(0x10AA7DA4),
    "rightValue": image.string(0x10AA53A8),
    "leftValueChannel": image.string(0x10AA54D8),
    "rightValueChannel": image.string(0x10AA54C4),
    "channel": image.string(0x10A8E058),
}
expected_strings = {
    "prompt": "New Name:",
    "renameDag": "Rename Dag",
    "renameAnimationSet": "Rename AnimationSet",
    "animationSetsAttribute": "animationSets",
    "rightValue": "rightValue",
    "leftValueChannel": "leftvaluechannel",
    "rightValueChannel": "rightvaluechannel",
    "channel": "channel",
}
if strings != expected_strings:
    raise SystemExit(f"native rename strings changed: {strings}")

receipt = {
    "binary": str(binary),
    "sha256": digest,
    "method": "Focused installed-binary byte checks for native DAG and AnimationSet rename paths",
    "verifiedSpans": spans,
    "strings": strings,
    "findings": [
        "Rename Dag and Rename AnimationSet are distinct native callbacks with separate operation names and selection/context setup, though both construct a New Name dialog.",
        "The AnimationSet callback resolves the animationSets attribute context before opening the dialog.",
        "After acceptance, the AnimationSet callback conditionally invokes 0x10729490 and, when a related-element result is returned, passes that element to the same datamodel name setter with the dialog result.",
        "The helper 0x10729490 walks a selection-backed collection and extracts/type-checks related records. The exact class and user-facing role of the optional secondary element remain unresolved.",
        "The verified extractor 0x10728CA0 returns three candidate slots: an interface-backed pair (with the third slot null), or datamodel fields named leftvaluechannel/rightvaluechannel when rightValue is present, or a single channel field otherwise. This establishes channel-oriented candidates in the generic resolver, not a backing gameModel/camera rename rule.",
        "Both callbacks bracket accepted edits with a named operation scope and invoke operation completion; this static route does not establish undo replay behavior.",
    ],
    "limits": [
        "No live SFM dialog interaction or runtime DMX diff was observed.",
        "The optional secondary AnimationSet rename target remains unidentified. Benchmark model/camera name correlations do not establish the native rename mapping; do not guess its mapping in the prototype.",
        "The candidate document rename currently journals one element name only; it does not claim complete native AnimationSet rename parity.",
    ],
    "all_match": True,
}
Path("outputs/hierarchy-rename-callback-verification.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"sha256": digest, "spans": len(spans), "strings": strings, "all_match": True}, indent=2))
