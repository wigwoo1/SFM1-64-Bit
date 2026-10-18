from pathlib import Path
p=Path('work/recover_owner_reset_cleanup.py')
s=p.read_text().replace('0x100fa244}','0x100fa244,0x100fa1f4}')
s=s.replace("'limits':'Static verified only. Gate identities, service interfaces, timer/FP semantics, query ownership and other resource helpers remain unbound; no GPU execution.'", "'limits':'Static verified only. Import identities and owning-thread gate identified; service interfaces, timer lifecycle/FP semantics, query ownership and other resource helpers remain unbound; no GPU execution.'")
p.write_text(s)
