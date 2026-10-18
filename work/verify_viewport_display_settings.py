"""Verify the installed SFM viewport Display submenu settings and handlers."""
import hashlib
import json
import re
from pathlib import Path

from pe_image import Image


DLL = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
LISTING = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/viewport-display-settings-verification.json")
RANGES = [
    ("Show Focal Plane registration", 0x101ACB31, 0x101ACBA5),
    ("Show Camera Frustum registration", 0x101ACBD4, 0x101ACC48),
    ("Show View Targets registration", 0x101ACC75, 0x101ACCEB),
    ("Focal Plane state getter", 0x10275EC0, 0x10275EC7),
    ("Camera Frustum state getter", 0x10275ED0, 0x10275ED7),
    ("View Targets state getter", 0x10275EE0, 0x10275EE7),
    ("Focal Plane setting handler", 0x10278830, 0x10278845),
    ("Camera Frustum setting handler", 0x10278850, 0x10278865),
    ("View Targets setting handler", 0x10278870, 0x10278885),
    ("Camera Frustum draw gate", 0x1029E685, 0x1029E69E),
    ("Camera Frustum geometry routine prologue", 0x10279120, 0x10279151),
]


def listing_ranges():
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
    verified_ranges = []
    total = 0
    for (name, lo, hi), spans in zip(RANGES, listing_ranges()):
        cursor = lo
        rows = []
        for span in spans:
            raw = span["bytes"]
            if span["va"] != cursor:
                raise AssertionError(f"{name}: listing gap at {cursor:#x}")
            offset = image.offset(cursor)
            if image.b[offset:offset + len(raw)] != raw:
                raise AssertionError(f"installed ifm.dll mismatch at {cursor:#x}")
            rows.append({"va": hex(cursor), "bytes": raw.hex(),
                          "instruction": span["instruction"]})
            cursor += len(raw)
        if cursor != hi:
            raise AssertionError(f"{name}: range ended at {cursor:#x}, expected {hi:#x}")
        total += hi - lo
        verified_ranges.append({"name": name, "start": hex(lo),
                                "end_exclusive": hex(hi), "spans": rows})

    labels = {
        "focal_plane": image.string(0x10A91234),
        "camera_frustum": image.string(0x10A9120C),
        "view_targets": image.string(0x10A911E8),
    }
    expected = {
        "focal_plane": "Show Focal Plane",
        "camera_frustum": "Show Camera Frustum",
        "view_targets": "Show View Targets",
    }
    if labels != expected:
        raise AssertionError(f"installed Display labels changed: {labels!r}")

    result = {
        "binary": str(DLL),
        "sha256": hashlib.sha256(DLL.read_bytes()).hexdigest(),
        "verified_code_bytes": total,
        "ranges": verified_ranges,
        "labels": labels,
        "settings": [
            {"label": labels["focal_plane"], "state_offset": "0x6c1", "handler": "0x10278830"},
            {"label": labels["camera_frustum"], "state_offset": "0x6c2", "handler": "0x10278850"},
            {"label": labels["view_targets"], "state_offset": "0x6c3", "handler": "0x10278870"},
        ],
        "findings": [
            "Each checkbox registration queries one dedicated byte in the viewport state object and passes the inverse value to its setting callback.",
            "Each callback writes that byte and calls the shared viewport-setting refresh routine at 0x10275f50.",
            "The installed camera viewport draw path reads the camera-frustum byte at 0x1029e685 and calls 0x10279120 only when the guide is enabled and the candidate camera differs from the active camera.",
            "The geometry routine prologue reads camera position/basis values and begins constructing the guide from camera projection extents; this identifies the native geometry path, but the full draw routine and focal-plane/view-target consumers remain to be recovered.",
        ],
        "limitations": "The trace verifies menu labels, state bytes, callback wiring, and the camera-frustum draw gate plus geometry-routine entry. It does not fully reconstruct camera-guide line submission, locate focal-plane/view-target draw consumers, or establish dynamic defaults in all SFM layouts.",
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {total} installed bytes across {len(RANGES)} viewport Display ranges")


if __name__ == "__main__":
    main()
