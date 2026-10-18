from pathlib import Path
import re
# Reuse the checked graph builder, adding parent and commit nodes before its audit.
source=Path('work/prepare_transition_composed_native.py').read_text()
insertion="""
parent=json.loads(Path('outputs/sfm64-restore-blend-helper-recovery.json').read_text());assert parent['sha256']==sha
nodes[int(parent['begin'],16)]=parent
commit=json.loads(Path('outputs/sfm64-restore-commit-native-preparation.json').read_text());assert commit['sha256']==sha
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\\s*([0-9A-F]{8}):\\s+((?:[0-9A-F]{2}\\s+)+)(.*)',line)
 if m:
  if pending:rows.append(pending)
  pending={'address':hex(int(m[1],16)),'bytes':bytes.fromhex(m[2]).hex(),'instruction':m[3].strip()}
 elif pending and re.fullmatch(r'\\s*(?:[0-9A-F]{2}\\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
if pending:rows.append(pending)
commit['instructions']=[row for row in rows if int(commit['begin'],16)<=int(row['address'],16)<int(commit['end'],16)]
nodes[int(commit['begin'],16)]=commit
assert len(nodes)==24
"""
assert source.count('assert len(nodes)==22')==1
source=source.replace('assert len(nodes)==22',insertion)
source=source.replace("0x1016e170:'packed_handler_table'","0x1016e170:'packed_handler_table',0x1016dd0c:'suppression_receiver_cell',0x1016e20c:'shader_receiver_cell'")
source=source.replace('transition-composed-original.bin','restore-transition-composed-original.bin').replace('sfm64-transition-composed-native-preparation.json','sfm64-restore-transition-composed-native-preparation.json')
source=source.replace("'entry_placement':placements[0x1002f2e0]","'entry_placement':placements[0x1002f410],'dispatcher_placement':placements[0x1002f2e0]")
source=source.replace('composed_transition_bodies','composed_restore_transition_bodies')
exec(compile(source,'checked_restore_transition_graph','exec'))
