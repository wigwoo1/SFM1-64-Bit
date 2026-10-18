from pathlib import Path
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
for begin,end in [(0x100e01aa,0x100e0f00),(0x100e1154,0x100e1d00)]:
 for r in rows:
  a=int(r['address'],16)
  if begin<=a<end and (r['instruction'].startswith(('ret','call','jmp'))):print(r['address'],r['instruction'])
