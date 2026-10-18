"""Byte-verify SFM's six Primary Viewport Mode submenu registrations."""
import hashlib
import json
import re
from pathlib import Path

from pe_image import Image


DLL = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
LISTING = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/viewport-tool-mode-menu-verification.json")
RANGES = [("mode-menu registrations and cleanup", 0x100A41EB, 0x100A46B7),
          ("shared mode dispatcher", 0x100A18B0, 0x100A18EB)]
MODES = [
    (0x10AA9C8C, "Set Active Viewport Tool To Selection", 0, 0x52, 0x100A4253, 0x100A4225),
    (0x10AA9C50, "Set Active Viewport Tool To Translation", 1, 0x53, 0x100A42FB, 0x100A42D1),
    (0x10AA9C08, "Set Active Viewport Tool To Global Translation", 2, 0x54, 0x100A43AB, 0x100A4381),
    (0x10AA9BCC, "Set Active Viewport Tool To Rotation", 3, 0x55, 0x100A4458, 0x100A442E),
    (0x10AA9B84, "Set Active Viewport Tool To Global Rotation", 4, 0x56, 0x100A4508, 0x100A44DE),
    (0x10AA9B4C, "Set Active Viewport Tool To Screen", 5, 0x57, 0x100A45B4, 0x100A4586),
]


def listing_bytes():
    parsed = {}
    row = re.compile(r"\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)")
    continuation = re.compile(r"\s{12}((?:[0-9A-F]{2} ?)+)\s*")
    active = None
    with LISTING.open(encoding="ascii", errors="replace") as listing:
        for line in listing:
            match = row.match(line)
            if match:
                address = int(match[1], 16)
                parsed[address] = (bytes.fromhex(match[2]), match[3].strip())
                active = address
            else:
                extra = continuation.fullmatch(line.rstrip())
                if extra and active is not None:
                    code, instruction = parsed[active]
                    parsed[active] = (code + bytes.fromhex(extra[1]), instruction)
    return parsed


def checked_range(image, listing, name, start, end):
    cursor, data = start, bytearray()
    while cursor < end:
        if cursor not in listing:
            raise AssertionError(f"{name}: missing disassembly at {cursor:#x}")
        row, _ = listing[cursor]
        actual = image.b[image.offset(cursor):image.offset(cursor) + len(row)]
        if actual != row:
            raise AssertionError(f"{name}: installed byte mismatch at {cursor:#x}")
        data.extend(row)
        cursor += len(row)
    if cursor != end:
        raise AssertionError(f"{name}: instruction runs past range end {end:#x}")
    return {"name": name, "start": hex(start), "end_exclusive": hex(end),
            "verified_bytes": end - start}


def main():
    image = Image(DLL)
    listing = listing_bytes()
    ranges = [checked_range(image, listing, name, lo, hi) for name, lo, hi in RANGES]
    if image.string(0x10AA9C8C) != MODES[0][1]:
        raise AssertionError("Selection label changed")
    rows = []
    registration = image.b[image.offset(RANGES[0][1]):image.offset(RANGES[0][1]) + RANGES[0][2] - RANGES[0][1]]
    for label_va, label, value, action_id, push_va, value_va in MODES:
        if image.string(label_va) != label:
            raise AssertionError(f"mode label changed at {label_va:#x}")
        push_offset = image.offset(push_va)
        if image.b[push_offset] != 0x68 or image.word(push_va + 1) != label_va:
            raise AssertionError(f"mode {label!r} no longer registers the expected label")
        value_offset = image.offset(value_va)
        if image.b[value_offset] != 0xC7:
            raise AssertionError(f"mode {label!r} command field initialization changed")
        # The trailing immediate is the final 4 bytes of the verified C7 /r instruction.
        init = image.b[value_offset:value_offset + 10]
        if len(init) != 10 or int.from_bytes(init[-4:], "little") != value or init[:2] != b"\xC7\x85":
            raise AssertionError(f"mode {label!r} command value changed")
        expected_id = bytes([0x6A, action_id])
        if expected_id not in registration:
            raise AssertionError(f"mode {label!r} action registration ID changed")
        rows.append({"label": label, "label_va": hex(label_va), "command_value": value,
                     "action_id": hex(action_id), "label_push_va": hex(push_va),
                     "command_value_init_va": hex(value_va)})

    dispatcher = image.b[image.offset(0x100A18B0):image.offset(0x100A18E9)]
    if b"\x89\x87\xC8\x00\x00\x00" not in dispatcher or b"\x8B\x9F\xD8\x00\x00\x00" not in dispatcher:
        raise AssertionError("mode dispatcher fields changed")
    if b"\x8B\x8F\xCC\x00\x00\x00" not in dispatcher or b"\xFF\xD0" not in dispatcher:
        raise AssertionError("mode dispatcher iteration/callback changed")

    result = {
        "binary": str(DLL),
        "sha256": hashlib.sha256(DLL.read_bytes()).hexdigest(),
        "ranges": ranges,
        "verified_code_bytes": sum(item[2] - item[1] for item in RANGES),
        "modes": rows,
        "dispatcher_observation": (
            "The shared callback stores the supplied mode integer at object offset +0xC8, "
            "then iterates the collection pointer at +0xCC for the count at +0xD8 and invokes "
            "each item's first virtual method. This proves callback fan-out and mode values, "
            "but not each downstream viewport tool's behavior."
        ),
        "limitations": "Registration evidence only; mouse/keyboard tool behavior, checkmark updates, camera transform effects, and live mode switching still require separate traces and runtime checks.",
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Verified six Primary Viewport modes ({result['verified_code_bytes']} installed code bytes)")


if __name__ == "__main__":
    main()
