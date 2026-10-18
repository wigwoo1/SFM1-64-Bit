from pathlib import Path
p=Path('work/named_element_reference_candidate.h')
s=p.read_text().replace('// conversion helper behavior remains explicit, not an invented coercion policy.','// Type is read again on mismatch; stable mismatches return the default.\n// Scratch helper copies a raw32 handle, with no coercion or parsing.')
s=s.replace('a.canConvertHandle(attribute)','a.isElementHandleType(attribute)').replace('a.convertIntoSharedHandle(attribute)','a.copyIntoSharedHandle(attribute)')
p.write_text(s)
p=Path('work/test_named_element_reference.cpp')
s=p.read_text().replace('canConvertHandle','isElementHandleType').replace('convertIntoSharedHandle','copyIntoSharedHandle').replace('conversion ordering','type reread/scratch copy ordering')
p.write_text(s)
