"""Derive mathematical decimal scaling constants; verify private reference data.

No DLL bytes are used to calculate constants. The installed table stores a
rounded high64 field and separate low16 field; its consumer undoes the high64
increment when low16 >= 0x8000 before multiplication.
"""
import json,tempfile
from datetime import datetime,timezone
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

def power80(k):
    n,d=(10**k,1) if k>=0 else (1,10**(-k))
    e=n.bit_length()-d.bit_length()
    if (n<(d<<e) if e>=0 else (n<<(-e))<d):e-=1
    shift=79-e
    nn,dd=(n<<shift,d) if shift>=0 else (n,d<<(-shift))
    q,r=divmod(nn,dd)
    q+=int(2*r>dd or (2*r==dd and (q&1)))
    if q==(1<<80):q>>=1;e+=1
    return q,e+16383

def main():
    out=ROOT/'outputs/material-value-construction-20261025'
    run=type(out)(tempfile.mkdtemp(prefix='powers-verification-',dir=out))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Library identity')
    im,evidence=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('table_selection_and_high64_adjustment',0x10087ece,0x10087f41)])
    rows=[];tables=[]
    for sign,base in [(1,0x10105128),(-1,0x10105288)]:
        table=[]
        for index in range(29):
            exponent=sign*(8**(index//7))*(index%7+1)
            mantissa,bias=power80(exponent)
            offset=im.offset(base+12*index);raw=im.b[offset:offset+12]
            low=int.from_bytes(raw[:2],'little');high=int.from_bytes(raw[2:10],'little')
            installed_bias=int.from_bytes(raw[10:12],'little')
            # The original code decrements DWORD +2 only, without borrowing.
            adjusted_high=high
            if low>=0x8000:
                adjusted_high=(high&0xffffffff00000000)|(((high&0xffffffff)-1)&0xffffffff)
            require(((adjusted_high<<16)|low)==mantissa and installed_bias==bias,'Effective exact power '+str(exponent))
            require(high==(mantissa>>16)+int((mantissa&65535)>=32768),'Stored high64 convention')
            table.append(mantissa.to_bytes(10,'little')+bias.to_bytes(2,'little'))
            rows.append(dict(decimal_exponent=exponent,address=hex(base+12*index),high64_adjusted=low>=32768))
        tables.append(table)
    # Generate independently calculated *effective* constants, not raw DLL data.
    lines=['#pragma once','#include <array>','#include <cstdint>',
        '// Generated from exact rational powers of ten by verify_material_decimal_powers.py.',
        '// Entries are effective normalized 80-bit constants; do NOT apply the',
        '// original storage-specific high64 correction to these values again.',
        '// Index: 7 entries per base-8 exponent group, then one for 4096.']
    for name,table in zip(['Positive','Negative'],tables):
        lines.append('inline constexpr std::array<std::array<uint8_t,12>,29> materialDecimal'+name+'PowersCandidate{{')
        lines += ['    {{'+','.join('0x%02x'%b for b in row)+'}},' for row in table]
        lines.append('}};')
    target=ROOT/'work/material_decimal_powers_candidate.h'
    target.write_text('\n'.join(lines)+'\n')
    require(sha(LIB)==sha(PRIVATE)==PIN,'Libraries unchanged')
    receipt=dict(status='VERIFIED mathematical scaling constants only',time_utc=datetime.now(timezone.utc).isoformat(),
        constants=len(rows),library_sha256=PIN,generated_header=str(target),generated_header_sha256=sha(target),
        static_evidence=evidence,entries=rows,
        limitations=['Multiplication/scaling loop not yet reconstructed','Not a constructor acceptance result','Not an editor integration'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(receipt=str(run/'results.json'),constants=len(rows),generated_header=str(target))))
if __name__=='__main__':main()
