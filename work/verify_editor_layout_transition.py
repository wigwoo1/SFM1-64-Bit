"""Byte-check the installed SFM active-editor selection transition path."""
import hashlib
import json
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pe_image import Image

TARGET = pathlib.Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
EXPECTED_SHA256 = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
im = Image(TARGET)
digest = hashlib.sha256(im.b).hexdigest()
if digest != EXPECTED_SHA256:
    raise SystemExit(f"installed ifm.dll changed: {digest}")


def data(va, length):
    offset = im.offset(va)
    return im.b[offset:offset + length]


def check(va, expected_hex, label):
    expected = bytes.fromhex(expected_hex)
    actual = data(va, len(expected))
    if actual != expected:
        raise SystemExit(f"byte mismatch for {label} at {va:#x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "bytes": actual.hex(), "length": len(actual), "label": label}


def call_target(va):
    raw = data(va, 5)
    if raw[0] != 0xE8:
        raise SystemExit(f"expected relative call at {va:#x}")
    return va + 5 + struct.unpack("<i", raw[1:])[0]


verified = []
# The active-editor setter obtains the current selector, compares it with the
# requested value, and branches through different transition/update paths.
verified.append(check(0x102CE770, "558bec515356576a046a00689c51aa106a00", "editor selector setter prologue"))
verified.append(check(0x102CE7BC, "8b7d083bdf7546", "requested-versus-current editor selector branch"))
verified.append(check(0x102CE7C3, "e8887addff8bc883fb02740983fb0474046a00eb026a01", "Motion/Graph selector mode flag routing"))
verified.append(check(0x102CE847, "57", "selector forwarded to editor transition routine"))
if call_target(0x102CE84A) != 0x102CE190:
    raise SystemExit(f"selector transition target mismatch: {call_target(0x102CE84A):#x}")
verified.append(check(0x102CE84A, "e841f9ffff", "active-editor state transition call"))

# The transition routine updates the selected editor, propagates selection to
# registered providers, and emits a selector-2-specific mode bit. Exact widget
# visibility and docking semantics are intentionally not inferred from this.
verified.append(check(0x102CE190, "558bec83ec0c53568bd9e841ddffff8bf085f60f844601000057", "editor transition routine prologue"))
verified.append(check(0x102CE262, "83ff020f94c10fb6d1528bcee88ded2f00", "selector-specific transition callback"))
verified.append(check(0x102CE2BB, "8b839c05000033f689451085c07e24", "registered editor-provider update loop"))
if call_target(0x102CE26E) != 0x105CD000:
    raise SystemExit(f"selector callback target mismatch: {call_target(0x102CE26E):#x}")

report = {
    "target": str(TARGET),
    "sha256": digest,
    "evidenceType": "static x86 byte verification from the currently installed SFM build",
    "verifiedRanges": verified,
    "controlFlow": {
        "setter": "0x102ce770",
        "transitionRoutine": "0x102ce190",
        "selectorInput": "setter compares the requested selector with the currently selected editor before choosing a transition path",
        "modeRouting": "the unchanged-selector path tests current selector values 2 and 4 and sends a boolean to a separate state setter; the transition path passes a selector-2-specific boolean to a callback",
        "providerUpdate": "transition routine iterates a provider collection and invokes a virtual callback with the selector and related state",
    },
    "limits": [
        "This static path confirms state-transition and provider-callback behavior, not the visible widgets' identities or exact docking/visibility outcomes.",
        "No runtime activation, persistence, replacement UI, or screenshot comparison has been verified.",
        "Installed addresses and x86 object offsets are not x64 ABI specifications.",
    ],
    "result": "passed",
}
out = pathlib.Path("outputs/sfm64-editor-layout-transition-audit.json")
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
print(f"verified {len(verified)} byte spans; selector transition target=0x102ce190; sha256={digest}; output={out}")
