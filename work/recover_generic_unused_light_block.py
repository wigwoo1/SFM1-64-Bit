"""Recover file-backed unused-light data without assuming a zero-filled block."""
import hashlib,json,re
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll'))
base=0x1016dd10
words=[image.word(base+4*n) for n in range(80)]
assert words[:20]==words[20:40]
assert words[40:60]!=words[:20]
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
references=[]
for line in lines:
 if re.search(r'1016DD10h',line):references.append(line.strip())
report={
 'binary_sha256':hashlib.sha256(image.b).hexdigest(),
 'address':hex(base),'file_backed_words':[hex(word) for word in words],
 'first_two_80_byte_entries_equal':True,
 'third_entry_differs':True,
 'first_entry_words':[hex(word) for word in words[:20]],
 'direct_disassembly_references':references,
 'limits':'File image data only. Runtime writes/indirect aliases and hardware query slot12 maximum remain unverified. Do not repeat the first entry across arbitrary hardware limits or assume zero constants. Reported 320-byte window is an inspection window, not a proven logical block extent.'}
Path('outputs/sfm64-generic-unused-light-block-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('unused_light_file_words=80 first_two_entries_equal=1 third_entry_differs=1')
