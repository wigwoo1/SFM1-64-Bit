import struct
from audit_model_materials import decode_materials

mdl = bytearray(380)
mdl[:4] = b'IDST'
struct.pack_into('<i', mdl, 4, 49)
struct.pack_into('<7i', mdl, 204, 2, 232, 1, 360, 2, 2, 364)
struct.pack_into('<i', mdl, 232, 144)  # first name at 376
struct.pack_into('<i', mdl, 296, 82)   # second name at 378
struct.pack_into('<i', mdl, 360, 375)  # empty search directory
struct.pack_into('<4h', mdl, 364, 1,0,0,1)
mdl[376:380] = b'a\0b\0'
metadata = decode_materials(mdl)
assert metadata == dict(names=['a','b'], directories=[''], slots=2, skins=[[1,0],[0,1]])
rejected = 0
for offset, fmt, value in ((204,'<i',-1),(208,'<i',379),(232,'<i',999),
                           (212,'<i',-1),(360,'<i',999),(220,'<i',-1),
                           (224,'<i',-1),(228,'<i',379),(364,'<h',2),
                           (366,'<h',-1)):
    mutated = bytearray(mdl)
    struct.pack_into(fmt, mutated, offset, value)
    try:
        decode_materials(mutated)
    except ValueError:
        rejected += 1
    else:
        raise AssertionError((offset,value))
print(f'Material names and swapped skin families passed; malformed rejections={rejected}')
