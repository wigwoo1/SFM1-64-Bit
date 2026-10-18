import hashlib
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
image = Image(binary)
digest = hashlib.sha256(binary.read_bytes()).hexdigest()


def disassembly_bytes(start, end):
    cursor = start
    active = False
    for line in Path("work/ifm-disassembly.txt").open(errors="replace"):
        match = re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)", line)
        if match:
            address = int(match.group(1), 16)
            if address < start:
                continue
            if address >= end:
                break
            active = True
            if address != cursor:
                raise AssertionError(f"non-contiguous disassembly at {address:#x}, expected {cursor:#x}")
            raw = bytes.fromhex(match.group(2))
            if address + len(raw) > end:
                raise AssertionError(f"range ends inside instruction at {address:#x}")
            offset = image.offset(address)
            if image.b[offset:offset + len(raw)] != raw:
                raise AssertionError(f"installed DLL bytes differ from disassembly at {address:#x}")
            cursor += len(raw)
        elif active and cursor < end and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$", line):
            raw = bytes.fromhex(line.strip())
            if cursor + len(raw) > end:
                raise AssertionError(f"range ends inside continued instruction at {cursor:#x}")
            offset = image.offset(cursor)
            if image.b[offset:offset + len(raw)] != raw:
                raise AssertionError(f"installed DLL bytes differ from continuation at {cursor:#x}")
            cursor += len(raw)
    if cursor != end:
        raise AssertionError(f"range ended at {cursor:#x}, expected {end:#x}")
    return {"start": hex(start), "end_exclusive": hex(end), "bytes": end - start, "verified": True}


ranges = {
    "waveform_setting_default": (0x1017A2D7, 0x1017A2DE),
    "waveform_setting_getter": (0x10168E00, 0x10168E07),
    "waveform_setting_toggle": (0x1016AB30, 0x1016AB51),
    "dynamic_menu_action_label": (0x1017157C, 0x1017159D),
    "timeline_sound_waveform_painter_gate": (0x10167B70, 0x10167C1B),
    "painter_selection_flag_branch": (0x10167D1E, 0x10167D22),
    "sound_wave_object_boolean_helper": (0x104BD2E0, 0x104BD2E7),
}
verified = {name: disassembly_bytes(*span) for name, span in ranges.items()}
report = {
    "binary": str(binary),
    "binary_sha256": digest,
    "all_instruction_bytes_verified": True,
    "verified_spans": verified,
    "recovered_behavior": {
        "state_default": "true",
        "when_state_true_menu_action": "Hide Waveform on Select",
        "when_state_false_menu_action": "Show Waveform on Select",
        "toggle": "inverts state, notifies, and rebuilds the settings menu",
        "paint_path": "CQTimelineClipRect::PaintSoundWaveForm resolves a sound-wave object, calls its virtual slot at vtable offset 0x10, then checks the object's byte at offset 0x30 against the helper result and waveform-on-select mode before drawing",
        "helper_104bd2e0": "returns the byte at object offset 0xB0 as a boolean; the caller uses it as an additional waveform gate",
    },
    "limitations": "Verified native state, toggle, dynamic label and static painter gates only. The meaning of the object byte at offset 0xB0, complete per-clip visibility predicate, sample extraction/drawing implementation, renderer integration and live visual parity remain unresolved.",
}
Path("outputs/timeline-waveform-visibility-recovery.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps(report, indent=2))
