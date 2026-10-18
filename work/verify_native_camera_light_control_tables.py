import hashlib
import json
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, "work")
from pe_image import Image

binary = Path(r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll")
raw = binary.read_bytes()
digest = hashlib.sha256(raw).hexdigest()
expected = "8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4"
if digest != expected:
    raise AssertionError(f"unexpected installed ifm.dll hash {digest}")
image = Image(binary)


def read_control_table(start, end):
    controls = []
    for address in range(start, end, 16):
        offset = image.offset(address)
        row = raw[offset:offset + 16]
        if len(row) != 16:
            raise AssertionError(f"truncated native control record at {address:#x}")
        name_va = struct.unpack_from("<I", row)[0]
        name = image.string(name_va)
        minimum, maximum, default = struct.unpack_from("<3f", row, 4)
        if not name or not all(math.isfinite(value) for value in (minimum, maximum, default)):
            raise AssertionError(f"invalid {name!r} control record at {address:#x}")
        if minimum > maximum or not minimum <= default <= maximum:
            raise AssertionError(f"invalid range/default for {name}: {minimum}, {maximum}, {default}")
        controls.append({
            "record_va": hex(address),
            "name": name,
            "name_va": hex(name_va),
            "minimum": minimum,
            "maximum": maximum,
            "default": default,
            "record_bytes": row.hex(),
        })
    if len({control["name"] for control in controls}) != len(controls):
        raise AssertionError("duplicate control names in native table")
    return controls


# Camera creator iterates from the fieldOfView record to just before intensity.
# Light creator iterates from intensity through edgeHeight.
camera = read_control_table(0x10EFF6C0, 0x10EFF740)
light = read_control_table(0x10EFF740, 0x10EFF8D0)
if [item["name"] for item in camera] != [
    "fieldOfView", "focalDistance", "aperture", "toneMapScale", "bloomScale",
    "SSAOBias", "SSAOStrength", "SSAORadius",
]:
    raise AssertionError("camera control sequence changed")
if [item["name"] for item in light] != [
    "intensity", "ambientIntensity", "horizontalFOV", "verticalFOV", "radius",
    "shadowFilterSize", "shadowAtten", "minDistance", "maxDistance",
    "constantAttenuation", "linearAttenuation", "quadraticAttenuation",
    "shadowDepthBias", "shadowSlopeScaleDepthBias", "farZAtten", "nearEdge",
    "farEdge", "cutOn", "cutOff", "volumetricIntensity", "noiseStrength",
    "width", "edgeWidth", "height", "edgeHeight",
]:
    raise AssertionError("light control sequence changed")

receipt = {
    "dll": str(binary),
    "sha256": digest,
    "record_layout": "16 bytes: name string pointer, minimum float32, maximum float32, default float32",
    "camera_table_range": ["0x10eff6c0", "0x10eff740"],
    "light_table_range": ["0x10eff740", "0x10eff8d0"],
    "camera_controls": camera,
    "light_controls": light,
    "limitations": "Static native default/control-range data only. This does not establish widget type, units, animation-set DMX graph structure, runtime editing behavior, or renderer interpretation.",
}
out = Path("outputs/native-camera-light-control-tables.json")
out.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
print(f"verified {len(camera)} camera and {len(light)} light controls; wrote {out}")
