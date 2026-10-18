import io
import struct
import argparse
import json
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser()
parser.add_argument('input')
parser.add_argument('rgba')
parser.add_argument('output')
args = parser.parse_args()
data = Path(args.input).read_bytes()
width, height = struct.unpack_from('<2H', data, 16)
format_id = struct.unpack_from('<I', data, 52)[0]
block_size = 8 if format_id == 13 else 16
size = lambda w,h: ((w+3)//4)*((h+3)//4)*block_size
offset = struct.unpack_from('<I', data, 12)[0] + ((data[61]+3)//4)*((data[62]+3)//4)*8
for level in range(data[56]-1, 2, -1):
    offset += size(max(1,width>>level), max(1,height>>level))
width, height = max(1,width>>2), max(1,height>>2)
payload = data[offset:offset+size(width,height)]
dds = bytearray(128)
dds[:4] = b'DDS '
struct.pack_into('<7I', dds, 4, 124, 0x81007, height, width, len(payload), 0, 0)
struct.pack_into('<2I', dds, 76, 32, 4)
dds[84:88] = b'DXT1' if format_id == 13 else b'DXT5'
struct.pack_into('<I', dds, 108, 0x1000)
reference = Image.open(io.BytesIO(dds+payload)).convert('RGBA').tobytes()
actual = Path(args.rgba).read_bytes()
assert len(actual) == len(reference)
differences = [abs(a-b) for a,b in zip(actual,reference)]
report = dict(width=width,height=height,differing_components=sum(v!=0 for v in differences),
              maximum_component_difference=max(differences), reference='Pillow DDS decoder')
Path(args.output+'.json').write_text(json.dumps(report,indent=2))
Image.frombytes('RGBA',(width,height),actual).save(args.output+'.png')
print(report)
