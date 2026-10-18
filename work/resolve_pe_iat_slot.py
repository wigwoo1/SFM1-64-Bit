"""Resolve an imported PE32 address (IAT slot VA) to DLL and symbol name."""
import argparse
import pathlib
import struct


def resolve(path: pathlib.Path, target_va: int):
    data = path.read_bytes()
    u16 = lambda off: struct.unpack_from("<H", data, off)[0]
    u32 = lambda off: struct.unpack_from("<I", data, off)[0]
    pe = u32(0x3C)
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("not a PE image")
    opt = pe + 24
    if u16(opt) != 0x10B:
        raise ValueError("expected PE32")
    image_base = u32(opt + 28)
    section_count = u16(pe + 6)
    optional_size = u16(pe + 20)
    sections = []
    for index in range(section_count):
        off = opt + optional_size + 40 * index
        sections.append((u32(off + 12), max(u32(off + 8), u32(off + 16)), u32(off + 20)))

    def offset(rva):
        for start, size, raw in sections:
            if start <= rva < start + size:
                return raw + rva - start
        raise ValueError(f"unmapped RVA {rva:#x}")

    directory = opt + 96
    import_rva = u32(directory + 8)
    descriptor = offset(import_rva)
    while any(data[descriptor:descriptor + 20]):
        original = u32(descriptor)
        name_rva = u32(descriptor + 12)
        first = u32(descriptor + 16)
        name_off = offset(name_rva)
        dll = data[name_off:data.index(b"\0", name_off)].decode("ascii", "replace")
        names = offset(original) if original else offset(first)
        slot = offset(first)
        index = 0
        while u32(names + 4 * index):
            thunk = u32(names + 4 * index)
            current_va = image_base + first + 4 * index
            if current_va == target_va:
                if thunk & 0x80000000:
                    symbol = f"ordinal {thunk & 0xffff}"
                else:
                    name = offset(thunk)
                    symbol = data[name + 2:data.index(b"\0", name + 2)].decode("ascii", "replace")
                return {"iat_va": hex(current_va), "dll": dll, "symbol": symbol}
            index += 1
        descriptor += 20
    return {"iat_va": hex(target_va), "error": "not found in import table"}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=pathlib.Path)
    parser.add_argument("iat_va", type=lambda value: int(value, 0))
    args = parser.parse_args()
    print(resolve(args.image, args.iat_va))
