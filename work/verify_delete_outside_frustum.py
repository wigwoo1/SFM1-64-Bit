"""Verify the installed SFM Delete-menu frustum actions and their shared removal path."""
import hashlib
import json
import re
from pathlib import Path

from pe_image import Image


DLL = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
LISTING = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/delete-outside-frustum-verification.json")
RANGES = [
    ("Models Outside Camera Frustum action", 0x101AD4C1, 0x101AD513),
    ("Particles Outside Camera Frustum action", 0x101AD54D, 0x101AD5B0),
    ("Shared camera-frustum action callback", 0x101A49C0, 0x101A4B86),
    ("Category filtering and child removal", 0x102B1620, 0x102B18C4),
    ("Collection child removal helper", 0x104C2B10, 0x104C2B4E),
]


def read_listing_ranges():
    groups = [[] for _ in RANGES]
    active = None
    row = re.compile(r"\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)")
    continuation = re.compile(r"\s{12}((?:[0-9A-F]{2} ?)+)\s*")
    with LISTING.open(encoding="ascii", errors="replace") as listing:
        for line in listing:
            match = row.match(line)
            if not match:
                extra = continuation.fullmatch(line.rstrip())
                if extra and active is not None:
                    active["bytes"] += bytes.fromhex(extra[1])
                continue
            va = int(match[1], 16)
            active = None
            for index, (_, lo, hi) in enumerate(RANGES):
                if lo <= va < hi:
                    active = {"va": va, "bytes": bytes.fromhex(match[2]),
                              "instruction": match[3].strip()}
                    groups[index].append(active)
                    break
    return groups


def main():
    image = Image(DLL)
    groups = read_listing_ranges()
    verified = []
    for (name, lo, hi), rows in zip(RANGES, groups):
        cursor = lo
        spans = []
        for row in rows:
            raw = row["bytes"]
            if row["va"] != cursor:
                raise AssertionError(f"{name}: disassembly gap at {cursor:#x}")
            off = image.offset(cursor)
            if image.b[off:off + len(raw)] != raw:
                raise AssertionError(f"installed ifm.dll mismatch at {cursor:#x}")
            spans.append({"va": hex(cursor), "bytes": raw.hex(),
                          "instruction": row["instruction"]})
            cursor += len(raw)
        if cursor != hi:
            raise AssertionError(f"{name}: range ended at {cursor:#x}, expected {hi:#x}")
        verified.append({"name": name, "start": hex(lo),
                          "end_exclusive": hex(hi), "spans": spans})

    strings = {
        "models_action": image.string(0x10AC8F50),
        "particles_action": image.string(0x10AC8F2C),
        "child_collection_attribute": image.string(0x10A8A7D4),
    }
    expected = {
        "models_action": "Models Outside Camera Frustum",
        "particles_action": "Particles Outside Camera Frustum",
        "child_collection_attribute": "children",
    }
    if strings != expected:
        raise AssertionError(f"unexpected installed labels/attribute: {strings!r}")

    # The callback passes its menu payload to the shared routine, which selects
    # one or both category loops by bits 1 and 2. The named action objects set
    # payloads 1 and 2 respectively (checked as instruction bytes above).
    result = {
        "binary": str(DLL),
        "sha256": hashlib.sha256(DLL.read_bytes()).hexdigest(),
        "verified_code_bytes": sum(hi - lo for _, lo, hi in RANGES),
        "ranges": verified,
        "strings": strings,
        "supported_findings": [
            "The two Delete-menu entries have different action text, command IDs (0x17c and 0x17d), and payloads (1 and 2), and both install the same callback at 0x101a49c0.",
            "The shared callback derives camera/frustum data from the active viewport and passes it with the selected action payload to 0x102b1620.",
            "The shared routine uses payload bits 1 and 2 to select separate object collections, tests candidate bounds through 0x10276010, then resolves the `children` attribute and removes the matching indexed child through 0x104c2b10.",
            "When any child is removed, the routine invokes additional dirty/update/rebuild paths.",
        ],
        "limitations": "The byte trace ties the named model and particle actions to payload-specific collections and a frustum predicate, but the predicate's true/false polarity and the complete object-type identity of the two collections have not been decoded. The user-facing 'Outside Camera Frustum' wording is visible in the installed menu; it is not used to overstate the predicate's internal polarity.",
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {result['verified_code_bytes']} installed bytes across {len(RANGES)} frustum-action ranges")


if __name__ == "__main__":
    main()
