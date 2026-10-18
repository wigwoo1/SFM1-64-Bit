"""Verify SFM's default-layout controller dispatch and virtual resource template."""
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
    off = im.offset(va)
    return im.b[off:off + length]


def check(va, expected_hex, label):
    expected = bytes.fromhex(expected_hex)
    actual = data(va, len(expected))
    if actual != expected:
        raise SystemExit(f"byte mismatch for {label} at {va:#x}: {actual.hex()} != {expected.hex()}")
    return {"va": f"0x{va:08x}", "bytes": actual.hex(), "label": label}


def word(va):
    return struct.unpack("<I", data(va, 4))[0]


def import_symbol(iat_va):
    pe = im.u32(0x3C)
    optional = pe + 24
    import_rva, import_size = struct.unpack_from("<II", im.b, optional + 104)

    def rva_offset(rva):
        for section_rva, size, raw in im.sections:
            if section_rva <= rva < section_rva + size:
                return raw + rva - section_rva
        raise ValueError(f"RVA has no file backing: {rva:#x}")

    cursor = rva_offset(import_rva)
    for _ in range(import_size // 20):
        original_thunk, _timestamp, _chain, name_rva, first_thunk = struct.unpack_from("<IIIII", im.b, cursor)
        if not any((original_thunk, name_rva, first_thunk)):
            break
        name_off = rva_offset(name_rva)
        dll = im.b[name_off:im.b.index(b"\0", name_off)].decode("ascii")
        thunk_off = rva_offset(original_thunk or first_thunk)
        index = 0
        while True:
            import_rva_or_ordinal = struct.unpack_from("<I", im.b, thunk_off + index * 4)[0]
            if not import_rva_or_ordinal:
                break
            if im.base + first_thunk + index * 4 == iat_va:
                if import_rva_or_ordinal & 0x80000000:
                    return f"{dll}!ordinal#{import_rva_or_ordinal & 0xffff}"
                import_name_off = rva_offset(import_rva_or_ordinal)
                name_start = import_name_off + 2  # IMAGE_IMPORT_BY_NAME hint
                symbol = im.b[name_start:im.b.index(b"\0", name_start)].decode("ascii")
                return f"{dll}!{symbol}"
            index += 1
        cursor += 20
    raise ValueError(f"IAT address not found: {iat_va:#x}")


controller_vtable = 0x10AA9984
default_layout_handler = word(controller_vtable + 0x5C)
if default_layout_handler != 0x1009A3B0:
    raise SystemExit(f"default-layout controller slot mismatch: {default_layout_handler:#x}")
layout_service_target = word(controller_vtable + 0x7C)
if layout_service_target != 0x1069EF90:
    raise SystemExit(f"layout-service virtual target mismatch: {layout_service_target:#x}")
load_layout_target = word(controller_vtable + 0xA8)
if load_layout_target != 0x1069F380:
    raise SystemExit(f"layout-file callback mismatch: {load_layout_target:#x}")
fallback_target = word(controller_vtable + 0x78)
if fallback_target != 0x100A62A0:
    raise SystemExit(f"default-layout fallback mismatch: {fallback_target:#x}")

expected_imports = {
    0x10A7B8FC: "QtCore4.dll!?fromAscii_helper@QString@@CAPAUData@1@PBDH@Z",
    0x10A7B378: "QtCore4.dll!?exists@QFile@@QBE_NXZ",
    0x10A7B37C: "QtCore4.dll!??0QFile@@QAE@ABVQString@@@Z",
    0x10A7B824: "QtCore4.dll!??0QSettings@@QAE@ABVQString@@W4Format@0@PAVQObject@@@Z",
    0x10A7B9A4: "QtCore4.dll!?value@QSettings@@QBE?AVQVariant@@ABVQString@@ABV2@@Z",
    0x10A7B94C: "QtCore4.dll!?toInt@QVariant@@QBEHPA_N@Z",
    0x10A7B328: "QtCore4.dll!?toString@QVariant@@QBE?AVQString@@XZ",
    0x10A7B394: "QtCore4.dll!?fileName@QFile@@QBE?AVQString@@XZ",
}
actual_imports = {hex(va): import_symbol(va) for va in expected_imports}
for va, expected in expected_imports.items():
    if import_symbol(va) != expected:
        raise SystemExit(f"unexpected import at {va:#x}: {import_symbol(va)}")

verified = [
    check(0x1009A3B0, "8b018b51108b407c52ffd0c3", "OnShowDefaultLayout forwarding thunk"),
    check(0x100A2B30, "558bec6aff68189baa10ff15fcb8a7108b4d08890183c4088bc15dc20400", "SFM layout group-name getter"),
    check(0x1069EF90, "558bec83ec20535633db895de8c645ec20", "default-layout resource resolver prologue"),
    check(0x1069EFC2, "6aff68a49eba10ff15fcb8a710", "default-layout virtual resource-template use"),
    check(0x1069EFF2, "ff1598b9a710", "layout group substituted into QString template"),
    check(0x1069F00C, "ff1590b9a710", "incremented layout index substituted into QString template"),
    check(0x1069F05E, "ff157cb3a710", "QFile constructed for resolved layout path"),
    check(0x1069F067, "ff1578b3a710", "layout path existence check"),
    check(0x1069F086, "8b87a80000008bceffd0", "existing layout file passed to reader callback"),
    check(0x1069F0C1, "8b168b42788bceffd0", "fallback invoked after layout load failure"),
    check(0x1069F38C, "6a006a01894df4508d4decff1524b8a710", "QSettings INI-format reader construction"),
    check(0x1069F3AE, "6aff688c9fa910ffd7", "Version key lookup"),
    check(0x1069F418, "6aff683074a910ffd7", "Name key lookup"),
    check(0x1069F472, "6aff68c49eba10ffd7", "Index key lookup"),
    check(0x1069F4C3, "837df801", "layout format version must equal 1"),
    check(0x1069F4D2, "83ff09", "layout index upper bound is 9"),
]
template_va = 0x10BA9EA4
template = im.string(template_va)
if template != "tools:layouts/%1/layout%2.ini":
    raise SystemExit(f"unexpected layout template: {template!r}")
group_va = 0x10AA9B18
group = im.string(group_va)
if group != "sfm":
    raise SystemExit(f"unexpected layout group: {group!r}")
layout_keys = {"Version": 0x10A99F8C, "Name": 0x10A97430, "Index": 0x10BA9EC4}
for key, va in layout_keys.items():
    if im.string(va) != key:
        raise SystemExit(f"unexpected layout INI key at {va:#x}: {im.string(va)!r}")

report = {
    "target": str(TARGET),
    "sha256": digest,
    "evidenceType": "static x86 dispatch and string-reference verification",
    "controllerAction": {
        "qtSignature": "OnShowDefaultLayout()",
        "metaMethodId": 59,
        "generatedVirtualByteOffset": "0x5c",
        "handler": f"0x{default_layout_handler:08x}",
        "handlerForwardsToControllerByteOffset": "0x7c",
        "resolver": f"0x{layout_service_target:08x}",
        "fileReaderCallback": f"0x{load_layout_target:08x}",
        "fallbackCallback": f"0x{fallback_target:08x}",
    },
    "virtualFilesystemTemplate": {
        "stringVa": f"0x{template_va:08x}",
        "format": template,
        "groupArgument": group,
        "indexArgument": "controller member at byte offset 0x10 plus one",
        "resolvedPattern": "tools:layouts/sfm/layout<controller[0x10]+1>.ini",
        "unresolved": ["the runtime value stored at controller byte offset 0x10", "the installed VFileSystem mount backing the tools: alias"],
    },
    "layoutIniReader": {
        "qSettingsFormat": "QSettings IniFormat (enum value 1)",
        "keysRead": layout_keys,
        "validation": ["Version must be 1", "Index must be <= 9"],
        "callbacksAfterValidation": ["controller vtable byte offset 0x80", "0x90", "0x64"],
        "qtImports": actual_imports,
    },
    "verifiedRanges": verified,
    "interpretation": "The default-layout action builds tools:layouts/sfm/layoutN.ini, checks whether that file exists, and passes it to a QSettings IniFormat reader. The reader queries Version, Name, and Index, accepts Version=1 and Index values at most 9, then calls layout-controller callbacks; failure returns to a separate fallback callback. The specific runtime index, filesystem mount, contents of the selected INI, and resulting visible panel geometry remain unverified.",
    "result": "passed",
}
out = pathlib.Path("outputs/sfm64-default-layout-resource-audit.json")
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
print(f"verified default layout handler 0x{default_layout_handler:08x} -> resource helper 0x{layout_service_target:08x}; template={template}; output={out}")
