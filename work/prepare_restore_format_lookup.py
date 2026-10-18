from pathlib import Path
import hashlib,json
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
offset=im.offset(0x10036f30);raw=im.b[offset:offset+15]
assert raw==bytes.fromhex('558bec8b45088b0485d4e516105dc3')
table=[im.word(0x1016e5d4+i*4) for i in range(64)]
prefix=f'static constexpr unsigned restoreFormatOffset={offset};\nstatic constexpr unsigned restoreFormatTableOffset={im.offset(0x1016e5d4)};\n'
Path('work/restore_format_lookup_init.h').write_text(prefix+'static constexpr std::array<std::uint32_t,64> restoreFormatInstalledWindow={{'+','.join(hex(v)+'u' for v in table)+'}};\n')
Path('outputs/sfm64-restore-format-installed-window.json').write_text(json.dumps({'sha256':sha,'address':'0x1016e5d4','sampled_dwords':table,'limits':'64 file-backed DWORD window, not proven table extent. Runtime initialization/mutation and enumeration semantics remain unverified.'},indent=2)+'\n')
Path('outputs/sfm64-restore-format-lookup-recovery.json').write_text(json.dumps({'sha256':sha,'address':'0x10036f30','bytes':raw.hex(),'table':'0x1016e5d4','operand_offset':9,'limits':'Direct indexed DWORD lookup; native bounds unchecked. Synthetic64entry table fixture does not establish actual table extent or format semantics.'},indent=2)+'\n')
print('restore_format_lookup_bytes=15 verified')
