"""Byte-check the recovered Graph Editor key-selection undo code against SFM."""
import json
import mmap
import re
from pathlib import Path

from pe_image import Image


DLL = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
LISTING = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/graph-key-selection-undo-recovery.json")

# Controller constructor/init, snapshot virtuals and helper bodies, then the
# undo constructor/apply method and selection helpers. End addresses are exclusive.
RANGES = [
    ("graph key Clear Key Selection action provider", 0x100B2CBD, 0x100B2D2D),
    ("graph tangent-selection callback", 0x100BF8F0, 0x100BF94D),
    ("snapshot virtual wrappers", 0x10353DB0, 0x10353DEE),
    ("controller snapshot dispatch", 0x1035ADC0, 0x1035ADDB),
    ("controller constructor vtable write", 0x1035D550, 0x1035D55A),
    ("snapshot cache helper A", 0x103907E0, 0x10390815),
    ("snapshot cache helper B", 0x10390980, 0x10390A46),
    ("key selection undo constructor", 0x107225A0, 0x107226E5),
    ("Select Keys undo-command apply and snapshot replay method", 0x107227B0, 0x10722A9A),
    ("clear mask for controls in selection set", 0x10720FB0, 0x1072101D),
    ("apply candidate record selection flags", 0x10721020, 0x107210EB),
    ("selection mask OR/AND-NOT/XOR dispatcher", 0x10734A30, 0x10734A70),
    ("clear selection mask", 0x10734A80, 0x10734A9B),
    ("key selection snapshot-diff command", 0x10722AA0, 0x10722BE4),
    ("key selection typed reference resolver", 0x10743F30, 0x10743F68),
    ("key selection record context builder", 0x10743F70, 0x1074400E),
    ("key selection record decoder", 0x10744060, 0x107441F5),
    ("controller initialization thunk", 0x10A69F30, 0x10A69F46),
]


def read_ranges():
    groups = [[] for _ in RANGES]
    row = re.compile(rb"\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} ?)+)")
    continuation = re.compile(rb"\s{12}((?:[0-9A-F]{2} ?)+)\s*")
    with LISTING.open("rb") as listing, mmap.mmap(listing.fileno(), 0, access=mmap.ACCESS_READ) as mapped:
        for index, (_, start, end) in enumerate(RANGES):
            marker = f"  {start:08X}:".encode("ascii")
            position = mapped.find(marker)
            if position < 0:
                raise AssertionError(f"listing has no row at {start:#x}")
            active = None
            while position < len(mapped):
                line_end = mapped.find(b"\n", position)
                if line_end < 0:
                    line_end = len(mapped)
                line = mapped[position:line_end].rstrip(b"\r")
                match = row.match(line)
                if match:
                    va = int(match[1], 16)
                    if va >= end:
                        break
                    active = {"va": hex(va), "bytes": bytes.fromhex(match[2].decode("ascii")).hex()}
                    groups[index].append(active)
                else:
                    extra = continuation.fullmatch(line)
                    if extra and active is not None:
                        active["bytes"] += bytes.fromhex(extra[1].decode("ascii")).hex()
                position = line_end + 1
    return groups


def main():
    image = Image(DLL)
    groups = read_ranges()
    verified_bytes = 0
    routines = []
    for (name, start, end), spans in zip(RANGES, groups):
        cursor = start
        for span in spans:
            va = int(span["va"], 16)
            raw = bytes.fromhex(span["bytes"])
            if va != cursor:
                raise AssertionError(f"disassembly gap at {cursor:#x}, next span {va:#x}")
            offset = image.offset(va)
            if image.b[offset : offset + len(raw)] != raw:
                raise AssertionError(f"installed ifm.dll mismatch at {va:#x}")
            cursor += len(raw)
            verified_bytes += len(raw)
        if cursor != end:
            raise AssertionError(f"range ended at {cursor:#x}, expected {end:#x}")
        routines.append({"name": name, "start": hex(start), "end_exclusive": hex(end), "spans": spans})

    controller = 0x10F3D770
    controller_vtable = 0x10B37094
    if image.word(0x10E9A51C) != controller:
        raise AssertionError("controller global pointer changed")
    slots = {"snapshot_a": image.word(controller_vtable + 0x140),
             "snapshot_b": image.word(controller_vtable + 0x144),
             "snapshot_dispatch": image.word(controller_vtable + 0x0C8),
             "resolve_record_reference": image.word(controller_vtable + 0x048)}
    expected_slots = {"snapshot_a": 0x10353DB0, "snapshot_b": 0x10353DD0,
                      "snapshot_dispatch": 0x1035ADC0,
                      "resolve_record_reference": 0x10356C00}
    if slots != expected_slots:
        raise AssertionError(f"controller vtable slots changed: {slots!r}")

    result = {
        "binary": DLL,
        "verified_code_bytes": verified_bytes,
        "controller_global": hex(controller),
        "controller_vtable": hex(controller_vtable),
        "verified_controller_vtable_slots": {key: hex(value) for key, value in slots.items()},
        "routines": routines,
        "findings": [
            "The initialized controller object is stored at 0x10F3D770 and uses vtable 0x10B37094; startup code calls its constructor at 0x1035D550.",
            "Controller vtable slots +0x140 and +0x144 lead to distinct wrappers. Both use the state subobject at this+0xC118 and share a count at subobject+0x54, returning cached buffers at +0x5C and +0x60 when populated.",
            "When the cache is empty, the wrappers request typed state through controller vtable slot +0xC8. The exact type descriptor and consumer semantics remain unidentified.",
            "The undo constructor captures both returned state buffers, copies supplied records in 16-byte units, and can retain two optional auxiliary arrays.",
            "The replay record decoder resolves word 3 through controller slot +0x48 and checks the result against the expected graph-target type descriptor 0x10EFFC4C. It carries words 0-2 as category/family, within-family index, and key ordinal/identity selectors, and validates their derived addresses against the resolved target.",
            "The adjacent typed-reference resolver at 0x10743F30 independently confirms the +0x48 lookup and 0x10EFFC4C descriptor check for a context locator's reference field; it returns null when resolution or type validation fails.",
            "Selection record consumer 0x1073C770 calls the decoder and applies validated selection-state changes. This consumer path does not establish where the undo snapshots are produced.",
            "A related context builder stores an owner pointer plus category, channel, and key selectors into a 16-byte temporary locator; this is a separate transient layout and is not treated as proof that those words map one-to-one to the serialized replay record.",
            "Method 0x107227B0 is both used by the graph Select Keys consumer and implements undo-command application: it conditionally constructs/submits a CUndoGraphEditorSelectKeys record, brackets state changes with controller batch calls, and has a snapshot-replay branch. Its other branches prepare selection state and process candidate records; they are not all snapshot replay.",
            "Selection preparation helper 0x10720FB0 walks a control set, resolves each handle through controller slot +0x48, validates descriptor 0x10EFFC4C, and invokes 0x10736090 followed by 0x10734A80 on valid controls. The latter clears a selection mask through the +0x6C setter.",
            "Candidate helper 0x10721020 decodes a 16-byte candidate, tests packed per-key flag bit 0, and when absent invokes 0x1073C770 with value 1 before setting bit 0. Its next arguments independently set/clear packed flag bits 1 and 2. The semantic names of these three flags remain unknown.",
            "Mask helper 0x10734A30 applies mode 0 OR, mode 1 AND-NOT, or mode 2 XOR to the mask at object+0x68 and publishes through object+0x6C. Mode 3 reaches no table entry and leaves the value unchanged. This proves the helper's bitwise operations, not which mouse/keyboard gesture selects each mode.",
            "In 0x107227B0's candidate-list path, selection mode 0 clears masks for the prior control set before applying candidates; mode 1 applies candidates without clearing the prior set; mode 2 clears selection bits on each valid candidate and XOR-updates its owner mask; mode 3 does the latter only when the candidate's selected bit was already set. The separate graph mouse-dispatch verifier maps the mode field to modifier combinations.",
            "The verified TimeStrip action provider adds the QAction labeled Clear Key Selection only when its key-selection object pointer is present. This proves the action's dynamic menu condition and placement through QWidget::addAction, but not the action's signal callback or its mutation behavior.",
            "The separate callback at 0x100BF8F0 uses the label Select Tangents and calls the snapshot-diff command at 0x10722AA0. It is not the Clear Key Selection callback. The snapshot-diff command allocates the key-selection undo object, submits it through controller slot +0x13C, and processes before/after records; the exact callback wiring is still unresolved.",
        ],
        "limitations": "The record fields are structurally classified from the decoder dataflow, but exact engine type names, the producer that emits serialized records, full snapshot semantics, flag-bit names, mode-to-gesture mapping, Clear Key Selection signal callback, and live UI behavior remain unknown. This is static binary validation, not a live interaction test.",
    }
    OUTPUT.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Verified Graph Editor key-selection command/helper path: {verified_bytes} installed ifm.dll bytes")


if __name__ == "__main__":
    main()
