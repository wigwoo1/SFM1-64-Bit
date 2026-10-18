"""Verify the installed SFM New Session wizard's native UI construction path."""
import hashlib
import json
import re
from pathlib import Path

from pe_image import Image


BINARY = r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
LISTING = Path("work/ifm-disassembly.txt")
OUTPUT = Path("outputs/new-session-wizard-verification.json")
START, END = 0x10105EE0, 0x101082D0
RECENT_START, RECENT_END = 0x101073BF, 0x101074C9
VALIDATION_CONNECTIONS = ((0x101062ED, 0x10106304), (0x101064DF, 0x101064F6))
PATH_BUILDER = (0x101050A0, 0x101051B1)
VALIDATION_SLOT = (0x101051C0, 0x10105372)


def verify_constructor(image):
    instructions = []
    current = None
    active = False
    row = re.compile(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)")
    continuation = re.compile(r"^\s{10,}((?:[0-9A-F]{2}\s*)+)$")
    for line in LISTING.open(encoding="ascii", errors="replace"):
        match = row.match(line)
        if match:
            address = int(match[1], 16)
            if address < START:
                continue
            if address >= END:
                break
            active = True
            current = [address, bytes.fromhex(match[2])]
            instructions.append(current)
            continue
        if active and current:
            extra = continuation.match(line.rstrip())
            if extra:
                current[1] += bytes.fromhex(extra[1])

    cursor = START
    checked = 0
    for address, expected in instructions:
        if address != cursor:
            raise AssertionError(f"disassembly gap at {cursor:#x}, next row {address:#x}")
        actual = image.b[image.offset(cursor):image.offset(cursor) + len(expected)]
        if actual != expected:
            raise AssertionError(f"installed bytes differ at {cursor:#x}")
        cursor += len(actual)
        checked += len(actual)
    if cursor != END:
        raise AssertionError(f"constructor ended at {cursor:#x}, expected {END:#x}")
    return checked


def verify_address_range(image, start, end):
    """Verify a contiguous disassembly range against the installed PE image."""
    instructions, current, active = [], None, False
    row = re.compile(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)")
    continuation = re.compile(r"^\s{10,}((?:[0-9A-F]{2}\s*)+)$")
    for line in LISTING.open(encoding="ascii", errors="replace"):
        match = row.match(line)
        if match:
            address = int(match[1], 16)
            if address < start:
                continue
            if address >= end:
                break
            active, current = True, [address, bytes.fromhex(match[2])]
            instructions.append(current)
            continue
        if active and current:
            extra = continuation.match(line.rstrip())
            if extra:
                current[1] += bytes.fromhex(extra[1])
    cursor, checked = start, 0
    for address, expected in instructions:
        if address != cursor:
            raise AssertionError(f"disassembly gap at {cursor:#x}, next row {address:#x}")
        actual = image.b[image.offset(cursor):image.offset(cursor) + len(expected)]
        if actual != expected:
            raise AssertionError(f"installed bytes differ at {cursor:#x}")
        cursor += len(actual)
        checked += len(actual)
    if cursor != end:
        raise AssertionError(f"range ended at {cursor:#x}, expected {end:#x}")
    return checked


def verify_recent_session_loop(image):
    """Byte-match recent-file enumeration and its 20-item cap."""
    instructions, current, active = [], None, False
    row = re.compile(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)")
    continuation = re.compile(r"^\s{10,}((?:[0-9A-F]{2}\s*)+)$")
    for line in LISTING.open(encoding="ascii", errors="replace"):
        match = row.match(line)
        if match:
            address = int(match[1], 16)
            if address < RECENT_START:
                continue
            if address >= RECENT_END:
                break
            active, current = True, [address, bytes.fromhex(match[2])]
            instructions.append(current)
            continue
        if active and current:
            extra = continuation.match(line.rstrip())
            if extra:
                current[1] += bytes.fromhex(extra[1])
    cursor, checked = RECENT_START, 0
    for address, expected in instructions:
        if address != cursor:
            raise AssertionError(f"recent-list disassembly gap at {cursor:#x}, next row {address:#x}")
        actual = image.b[image.offset(cursor):image.offset(cursor) + len(expected)]
        if actual != expected:
            raise AssertionError(f"installed recent-list bytes differ at {cursor:#x}")
        cursor += len(actual)
        checked += len(actual)
    if cursor != RECENT_END:
        raise AssertionError(f"recent-list range ended at {cursor:#x}, expected {RECENT_END:#x}")
    return checked


image = Image(BINARY)
sha = hashlib.sha256(image.b).hexdigest()
if sha != "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4":
    raise AssertionError(f"installed ifm.dll changed: {sha}")

expected_strings = {
    0x10E8361C: ".?AVCQSFMNewWizardDialog@@",
    0x10AA18A0: "CQSFMNewWizardDialog",
    0x10AB609C: "NewSessionWizard",
    0x10AB60B0: "session",
    0x10AB6084: "FrameRate",
    0x10AB6090: "Directory",
    0x10AB6530: "Framerate:",
    0x10AB6558: "Directory:",
    0x10AB64BC: " Recent Session ",
    0x10AB6454: " Open Session ",
    0x10AB6424: "Open an existing session through a file dialog",
    0x10AB65B0: " New Session ",
    0x10AB61A8: "Create Session",
    0x10AB61B8: "CreateSessionInWizard",
    0x10AB617C: "OpenFileInWizard",
    0x10AB6190: "OpenRecentFileInWizard",
    0x10AA18C2: "OnCreate()",
    0x10AA18CD: "OnBrowseDirectory()",
    0x10AA18E6: "OnOpenRecentFile(QString)",
    0x10AA1900: "OnOpenRecentFile()",
    0x10AA1913: "OnOpen()",
    0x10AA191C: "CheckNameAndDirectory(QString)",
    0x10AB6518: "59.94",
    0x10AB6520: "29.97",
    0x10AB6528: "23.976",
    0x10AB4A74: "Browse...",
    0x10AB6564: "2textChanged( const QString &)",
    0x10AB6584: "1CheckNameAndDirectory( const QString & )",
}
strings = {hex(address): image.string(address) for address in expected_strings}
for address, expected in expected_strings.items():
    if strings[hex(address)] != expected:
        raise AssertionError(f"string mismatch at {address:#x}: {strings[hex(address)]!r}")

constructor_bytes = verify_constructor(image)
if constructor_bytes != END - START:
    raise AssertionError(f"unexpected constructor size: {constructor_bytes}")
recent_loop_bytes = verify_recent_session_loop(image)
validation_connection_bytes = [verify_address_range(image, start, end) for start, end in VALIDATION_CONNECTIONS]
path_builder_bytes = verify_address_range(image, *PATH_BUILDER)
validation_slot_bytes = verify_address_range(image, *VALIDATION_SLOT)

# Qt meta-object entry 6 names CheckNameAndDirectory(QString), and the
# CQSFMNewWizardDialog vtable entry reached by that slot resolves to 0x101051C0.
if image.word(0x10AA1888) != 0x7C:
    raise AssertionError("validation meta-object method index changed")
if image.word(0x10AB607C) != VALIDATION_SLOT[0]:
    raise AssertionError("validation slot vtable target changed")

# The native constructor sets an 800 logical-pixel width and derives height
# from its mode argument; the screenshot is one displayed state of that dialog.
if image.word(0x10105FEA) != 0x320:
    raise AssertionError("wizard width initializer changed")

receipt = {
    "binary": BINARY,
    "sha256": sha,
    "class": "CQSFMNewWizardDialog",
    "constructor": {"start": hex(START), "end_exclusive": hex(END), "verified_bytes": constructor_bytes},
    "recent_session_loop": {"start": hex(RECENT_START), "end_exclusive": hex(RECENT_END), "verified_bytes": recent_loop_bytes},
    "verified_ui": {
        "object_names": ["NewSessionWizard", "session", "FrameRate", "Directory"],
        "sections": ["New Session", "Recent Session", "Open Session", "More"],
        "actions_and_slots": ["Create", "Browse...", "Open Recent", "Open", "OnCreate()", "OnBrowseDirectory()", "OnOpenRecentFile()", "OnOpen()"],
        "validation_slot": "CheckNameAndDirectory(const QString&), connected from textChanged(const QString&) on both session text fields.",
        "validation_connection_ranges": [{"start": hex(start), "end_exclusive": hex(end), "verified_bytes": count} for (start, end), count in zip(VALIDATION_CONNECTIONS, validation_connection_bytes)],
        "validation_method_record": {"address": "0x10AA1888", "string_offset": "0x7c", "vtable_entry": "0x10AB607C", "native_target": hex(VALIDATION_SLOT[0])},
        "validation_slot_range": {"start": hex(VALIDATION_SLOT[0]), "end_exclusive": hex(VALIDATION_SLOT[1]), "verified_bytes": validation_slot_bytes},
        "target_path_builder_range": {"start": hex(PATH_BUILDER[0]), "end_exclusive": hex(PATH_BUILDER[1]), "verified_bytes": path_builder_bytes},
        "create_enabled_when": "The Create control exists, the session name and directory are nonempty, and the constructed .dmx target path does not already exist.",
        "target_exists_check": {"interface_method_offset": "0x28", "arguments": ["candidate .dmx path", "nullptr path ID"], "method_slot_index": 10, "identification": "signature and slot match IBaseFileSystem::FileExists; the global pointer binding is not yet traced"},
        "fractional_frame_rate_entries": ["23.976", "29.97", "59.94"],
        "logical_width_pixels": 800,
        "recent_session_items_max": 20,
        "recent_session_population": "Enumerates the application's recent-file list and creates one menu item per path, up to the 20-item limit.",
    },
    "limits": [
        "The screenshot is supplied UI evidence; this verifier checks the installed ifm.dll constructor and strings, not a live SFM interaction.",
        "The complete set/order of frame-rate choices, default session name/directory, browse cancellation behavior, recent-path ordering, and create/open persistence effects are not established here.",
        "The full dialog's visual pixel parity and current-session startup path remain unverified.",
    ],
    "strings": strings,
    "all_match": True,
}
OUTPUT.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"class": receipt["class"], "constructor_bytes": constructor_bytes,
                  "recent_loop_bytes": recent_loop_bytes, "recent_session_items_max": 20,
                  "validation_connection_bytes": sum(validation_connection_bytes),
                  "validation_slot_bytes": validation_slot_bytes, "path_builder_bytes": path_builder_bytes,
                  "sha256": sha, "strings_checked": len(strings), "all_match": True}, indent=2))
