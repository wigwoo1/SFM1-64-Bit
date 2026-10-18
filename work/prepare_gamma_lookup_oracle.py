from pathlib import Path
import json, struct
import audit_installed_gamma_conversion as audit
code = b''.join(bytes.fromhex(row['bytes']) for row in audit.rows)
fixups = []
for address in (0x10152408,0x100fc760,0x101941d0):
    needle = struct.pack('<I',address)
    assert code.count(needle) == 1
    fixups.append(code.index(needle))
Path('work/gamma_lookup_oracle_bytes.h').write_text(
 '#pragma once\nstatic const unsigned char originalGammaCode[] = {'+
 ','.join(hex(value) for value in code)+'};\nstatic const unsigned gammaFixups[] = {'+
 ','.join(map(str,fixups))+'};\n')
Path('outputs/gamma-lookup-oracle-fixture.json').write_text(json.dumps({
 'source_sha256':audit.digest,'function_address':'0x100d4ef0',
 'bytes':code.hex(),'absolute_operand_fixups':fixups,
 'scope':'Only data pointers relocated; synthetic table isolates lookup/rounding, not table generation.'},indent=2)+'\n')
print('Prepared verified 71-byte original-code fixture')
