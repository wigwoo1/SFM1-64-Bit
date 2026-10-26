"""Private numerical evidence; independently generate, then check CRT constants.

No loader, application control, binary modification or extracted code arrays.
"""
from decimal import Decimal, localcontext
from pathlib import Path
import json, math, re, struct
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT, LIB, PRIVATE, PIN, require, sha

OUT = ROOT/'outputs/material-trig-fast-20261026'

def main():
    OUT.mkdir(exist_ok=True)
    require(sha(LIB)==sha(PRIVATE)==PIN, 'Pinned library')
    image, ranges = check_ranges(LIB, 'materialsystem-disassembly.txt', [
        ('sine_complete_fast_and_dispatch',0x10081314,0x10081463),
        ('cosine_complete_fast_and_dispatch',0x10081786,0x100818dd)])
    def double(address):
        return struct.unpack_from('<d',image.b,image.offset(address))[0]
    def binary32(x): return struct.unpack('<f',struct.pack('<f',x))[0]
    def u32(x): return struct.unpack('<I',struct.pack('<f',x))[0]
    with localcontext() as ctx:
        ctx.prec=100
        def atan(x):
            term=total=x; n=1
            while True:
                term *= -x*x; n+=2; new=total+term/n
                if new==total:return total
                total=new
        pi=16*atan(Decimal(1)/5)-4*atan(Decimal(1)/239)
        def sine(x):
            term=total=x; n=1
            while True:
                term *= -x*x/((n+1)*(n+2)); n+=2; new=total+term
                if new==total:return total
                total=new
        quarter=[float(sine(pi*k/32)) for k in range(17)]
    header=(ROOT/'work/material_trig_fast_candidate.h').read_text()
    table_text=header.split('constexpr double quarter[] = {',1)[1].split('};',1)[0]
    literals=re.findall(r'0x[0-9a-f.]+p[+-]\d+',table_text)
    require([float.fromhex(x) for x in literals]==quarter,'Owned quarter independently regenerated')
    def table(k):
        k%=64
        if k<=16:return quarter[k]
        if k<=32:return quarter[32-k]
        if k<=48:return -quarter[k-32]
        return -quarter[64-k]
    for base in [0x100e0b60,0x100e1b80]:
        for k in range(64):
            for offset,phase in [(0,k),(8,k+16)]:
                require(struct.pack('<d',double(base+16*k+offset))==struct.pack('<d',table(phase)),'Generated installed table bits')
    hi=float.fromhex('0x1.921fb54444000p-4'); tail=float.fromhex('0x1.2e7b9676733afp-44')
    for base in [0x100e0f70,0x100e1f90]:
        require(math.ldexp(double(base),-896)==tail and math.ldexp(double(base+8),-896)==hi,'Reduction constants')
        require([double(base+8*k) for k in range(2,7)]==[1.,-1/6,1/120,-.5,1/24],'Polynomial constants')
    require(double(0x100e0fa8)==1-2**-30,'Tiny sine multiplier')
    degrees=struct.unpack('<f',struct.pack('<I',0x4658f683))[0]
    radians=binary32(degrees*0.017453292519943295)
    n=round(binary32(radians*float.fromhex('0x1.45f306p+3')))
    r=radians-n*hi; correction=n*tail; corrected=r+correction
    squared=r*r; fourth=squared*squared
    cp=(squared*(-.5)+1)+fourth*(1/24)
    sp=(squared*(-1/6)+1)+fourth*(1/120)
    k=(n+16)&63; term1=table(k)*cp; term2=(corrected*table(k+16))*sp
    value=term1+term2
    require(u32(value)==0xbf66d095 and u32(math.cos(radians))==0xbf66d094,'Explained cosine boundary')
    receipt=dict(status='PASS',library_sha256=PIN,independent_table_entries_checked=256,
        precision_decimal_digits=100,static_ranges=ranges,
        failing_case_trace=dict(degrees=degrees,radians=radians,n=n,table_index=k,
            remainder=r,correction=correction,corrected=corrected,cos_polynomial=cp,
            sin_polynomial=sp,first_term=term1,second_term=term2,result_binary64=value,
            polynomial_binary32=hex(u32(value)),host_cosine_binary32=hex(u32(math.cos(radians)))),
        scope='Fast/tiny/nonfinite numerical path only; large-angle legacy fallback not reconstructed')
    (OUT/'recovery.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['status','independent_table_entries_checked','failing_case_trace','scope']}))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original unchanged')

if __name__=='__main__':main()
