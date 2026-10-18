"""Byte-check the installed SFM View-menu action setup and receivers."""
import json
import re
from pathlib import Path

from pe_image import Image


DLL = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
LISTING = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/view-menu-action-verification.json")
RANGES = [
    ("View menu action setup", 0x100A8C40, 0x100A8E12),
    ("primary-controls receiver", 0x1009C340, 0x1009C3AA),
    ("secondary-controls receiver", 0x1009C3B0, 0x1009C41A),
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
                    active["bytes"] += bytes.fromhex(extra[1]).hex()
                continue
            va = int(match[1], 16)
            active = None
            for index, (_, lo, hi) in enumerate(RANGES):
                if lo <= va < hi:
                    active = {"va": va, "bytes": bytes.fromhex(match[2]).hex(),
                              "instruction": match[3].strip()}
                    groups[index].append(active)
                    break
    return groups


def main():
    image = Image(DLL)
    groups = listing_ranges()
    result_ranges = []
    for (name, lo, hi), spans in zip(RANGES, groups):
        cursor = lo
        clean = []
        for span in spans:
            raw = bytes.fromhex(span["bytes"])
            if span["va"] != cursor:
                raise AssertionError(f"{name}: listing gap at {cursor:#x}")
            if image.b[image.offset(cursor):image.offset(cursor) + len(raw)] != raw:
                raise AssertionError(f"installed ifm.dll mismatch at {cursor:#x}")
            clean.append({"va": hex(cursor), "bytes": span["bytes"],
                          "instruction": span["instruction"]})
            cursor += len(raw)
        if cursor != hi:
            raise AssertionError(f"{name}: range ended at {cursor:#x}, expected {hi:#x}")
        result_ranges.append({"name": name, "start": hex(lo),
                              "end_exclusive": hex(hi), "spans": clean})

    expected = {
        0x10AAAAE0: "Show Fullscreen",
        0x10AAAAF0: "Show Normal",
        0x10AAAC70: "Show Primary Viewport Controls",
        0x10AAAC2C: "Show Secondary Viewport Controls",
    }
    labels = {hex(address): image.string(address) for address in expected}
    if any(labels[hex(address)] != text for address, text in expected.items()):
        raise AssertionError(f"View menu labels changed: {labels!r}")
    if image.word(0x100A8C56) != 0x10AAAAF0 or image.word(0x100A8C5D) != 0x10AAAAE0:
        raise AssertionError("fullscreen action label-selection branch changed")

    result = {
        "binary": DLL,
        "sha256": __import__("hashlib").sha256(Path(DLL).read_bytes()).hexdigest(),
        "verified_code_bytes": sum(hi - lo for _, lo, hi in RANGES),
        "ranges": result_ranges,
        "labels": labels,
        "findings": [
            "The View-menu setup chooses the fullscreen action text between Show Fullscreen and Show Normal through a state-query branch.",
            "The setup registers separate primary- and secondary-viewport control actions with distinct receiver objects and stores the action handles in adjacent object fields.",
            "Both control actions are marked checkable and their checked states are refreshed through the action-state path.",
            "The primary and secondary control receivers are distinct and pass different typed interface descriptors into a shared dispatch helper.",
        ],
        "live_observation": {
            "application": "Source Filmmaker [Beta]",
            "session_title": "particle testf.dmx",
            "view_menu_items": ["Show Fullscreen", "Show Primary Viewport Controls", "Show Secondary Viewport Controls"],
            "primary_controls_initially_checked": True,
            "primary_controls_toggle_observed": "hides viewport tool/transport strip; toggling again restored it",
            "final_state_restored": True,
        },
        "limitations": "Static bytes identify registration and receiver routing but do not prove receiver semantics or fullscreen window-state transitions. Live observation exercised only the reversible primary-controls toggle; no project editing or save operation was performed.",
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Verified SFM View-menu action setup: {result['verified_code_bytes']} installed bytes across {len(RANGES)} ranges")


if __name__ == "__main__":
    main()
