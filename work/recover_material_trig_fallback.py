"""Independent numerical constants and checked installed fallback path evidence.

Reads pinned binaries only; creates private evidence under outputs. Never uses
an engine loader, copies executable bytes into source, or controls live SFM.
"""
from decimal import Decimal,localcontext,ROUND_HALF_EVEN
from pathlib import Path
import json,re,struct
from verify_ifm_draw_packet import check_ranges
from verify_material_numeric_value import ROOT,LIB,PRIVATE,PIN,require,sha

OUT=ROOT/'outputs/material-trig-fallback-20261026'

def main():
    OUT.mkdir(exist_ok=True)
    require(sha(LIB)==sha(PRIVATE)==PIN,'Pinned material library')
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('float_sine_dispatch',0x10081314,0x10081463),
        ('float_cosine_dispatch',0x10081786,0x100818dd),
        ('double_sine_dispatch_and_x87',0x1008b380,0x1008b4a9),
        ('double_cosine_dispatch_and_x87',0x1008ba10,0x1008bb39),
        ('double_sine_sse2_and_large_fallback',0x10090f78,0x10091127),
        ('double_cosine_sse2_and_large_fallback',0x10091148,0x100912d9),
        ('x87_input_classification',0x10084bf5,0x10084c38),
        ('x87_finite_return_control_restore',0x10084c5b,0x10084c85),
        ('cpu_feature_flag_initializer',0x10083c10,0x10083c20)])
    with localcontext() as ctx:
        ctx.prec=110
        def atan(x):
            term=total=x;n=1
            while True:
                term*=-x*x;n+=2;new=total+term/n
                if new==total:return total
                total=new
        pi=16*atan(Decimal(1)/5)-4*atan(Decimal(1)/239)
        def sine(x):
            term=total=x;n=1
            while True:
                term*=-x*x/((n+1)*(n+2));n+=2;new=total+term
                if new==total:return total
                total=new
        quarter=[]
        for k in range(17):
            value=sine(pi*k/32) if k else Decimal(0)
            if k==16:value=Decimal(1)
            power=Decimal(0)
            if value:
                power=Decimal(1)
                while power>value:power/=2
                if power*2-value<value-power:power*=2
            high=float(value)
            low=struct.unpack('<f',struct.pack('<f',float(value-Decimal.from_float(high))))[0]
            quarter.append((high,float(low),float(value-power),float(power)))
        period=pi*Decimal(2)**62
        significand=int(period.to_integral_value(rounding=ROUND_HALF_EVEN))
        require(significand==14488038916154245685,'Independent period significand')
        period_text=str(period)
    header=(ROOT/'work/material_trig_fallback_candidate.h').read_text()
    text=header.split('constexpr MaterialTrigDoubleCellCandidate quarter[] = {',1)[1].split('};',1)[0]
    values=[float.fromhex(x) for x in re.findall(r'-?0x[\da-f.]+p[+-]\d+',text)]
    require(values==[x for row in quarter for x in row],'Independently generated owned table cells')
    def cell(k):
        k%=64;j=k if k<=16 else 32-k if k<=32 else k-32 if k<=48 else 64-k
        return tuple(-v if k>32 and v else v for v in quarter[j])
    checked=0
    for base in [0x100f2ba0,0x100f3440]:
        for k in range(64):
            s=cell(k);c=cell(k+16);expected=(c[2],s[0],s[1],c[3])
            require(struct.pack('<4d',*expected)==image.b[image.offset(base+32*k):image.offset(base+32*k)+32],'Generated installed double table')
            checked+=4
    period_bits=struct.pack('<QH',significand,0x403e)
    require(image.b[image.offset(0x100eda1a):image.offset(0x100eda1a)+10]==period_bits,'Installed pi*2^62 period')
    require(image.b[image.offset(0x100eda18):image.offset(0x100eda18)+2]==struct.pack('<H',0x027f),'Original nearest/53-bit control')
    asm=(ROOT/'work/material_trig_x87_candidate.inc').read_text()
    literal=re.search(r'REAL10\s+([\d.E+]+)',asm)[1]
    require(int(Decimal(literal).to_integral_value(rounding=ROUND_HALF_EVEN))==significand,'MASM period decimal literal rounding')
    coefficients=[-1/6,-.5,1/120,1/24,-1/5040,-1/720,1/362880,1/40320]
    for base in [0x100f33a0,0x100f3c40]:
        expected=struct.pack('<8d',*coefficients)
        require(image.b[image.offset(base):image.offset(base)+len(expected)]==expected,'Double polynomial coefficients')
        for offset,value in [(64,float.fromhex('0x1.45f306dc9c883p+3')),(72,float.fromhex('0x1.8p+52')),
                             (80,float.fromhex('0x1.0b4611a6p-38')),(88,float.fromhex('0x1.0b4611a6p-38')),
                             (96,float.fromhex('0x1.921fb544p-4')),(104,float.fromhex('0x1.3198a2e037073p-73'))]:
            require(image.b[image.offset(base+offset):image.offset(base+offset)+8]==struct.pack('<d',value),'Double reduction constants')
    # For all binary32 values entering this fallback, the observed double high
    # word comparison changes from SSE2 to x87 exactly at abs(x)=90112.
    boundary=[]
    for bits in [0x46000000,0x47afffff,0x47b00000,0x7f7fffff]:
        value=struct.unpack('<f',struct.pack('<I',bits))[0]
        high=struct.unpack('<4H',struct.pack('<d',value))[3]&0x7fff
        native_sse=((high-0x3030)&0xffff)<=0x10c5
        require(native_sse==(bits<0x47b00000),'SSE2 fallback boundary')
        boundary.append(dict(bits=hex(bits),value=value,native_sse2_moderate=native_sse))
    receipt=dict(status='PASS',library_sha256=PIN,decimal_precision=110,
        generated_installed_table_doubles=checked,period_decimal=period_text,
        period_extended_bytes=period_bits.hex(),x87_control_word='0x027f',
        dispatch_boundaries=boundary,static_ranges=ranges,
        limits=['CPU flag0/1 are controlled fixture states; live selection unverified',
                'x87 results tested on current CPU; no all-CPU or FP status/exception equivalence',
                'No production/editor integration or original writes'])
    (OUT/'recovery.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:receipt[k] for k in ['status','generated_installed_table_doubles','period_extended_bytes','dispatch_boundaries']}))
    require(sha(LIB)==sha(PRIVATE)==PIN,'Original library unchanged')

if __name__=='__main__':main()
