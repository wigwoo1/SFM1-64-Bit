import json
from pathlib import Path
from pe_image import Image

image = Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
address = 0x1008be70
expected = bytes.fromhex('558bec8b411885c074128b4d088d490085c9740a8b401c4985c075f433c05dc20400')
offset = image.offset(address)
assert image.b[offset:offset+len(expected)] == expected
receipt = dict(address=hex(address), complete_routine_bytes=expected.hex(),
    verified_bytes=len(expected), findings=[
        'Reads first child at receiver18, next sibling at child1c.',
        'Returns child when ordinal reaches zero; otherwise decrements ordinal after following sibling.',
        'Missing child or exhausted chain returns null. Negative ordinal has no explicit rejection and traverses a finite chain to null.'
    ], limitations='Complete routine bytes verified; portable node ownership and original Qt index creation remain separate. Original runtime parity unverified.')
Path('outputs/control-child-accessor-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified complete child accessor:',len(expected),'bytes')
