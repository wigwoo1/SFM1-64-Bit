from pathlib import Path
from pe_image import Image
import struct,math,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll')
def f(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def double(address):return struct.unpack_from('<d',image.b,image.offset(address))[0]
def bits(x):return struct.unpack('<Q',struct.pack('<d',x))[0]
def frombits(x):return struct.unpack('<d',struct.pack('<Q',x&0xffffffffffffffff))[0]
scale=struct.unpack('<f',bytes.fromhex('3baa b846'.replace(' ','')))[0]
def candidate(x):
    # Normal-range path 1045D541..1045D5D2; nearest-even SSE conversion assumed.
    n=round(f(x*scale))
    table0=double(0x105EF0C8+((n<<3)&0x78))
    table1=double(0x105EEFC8+((n>>1)&0xf8))
    table2=double(0x105EEEC8+((n>>6)&0xf8))
    residual=double(0x105EF148)*x-float(n)
    correction=(double(0x105EF150)*table0)*residual
    exponent=frombits(bits(table1*table2)+((n>>14)<<52))
    return f((table0+correction)*exponent)
maximum=0;different=0;examples=[]
for index in range(1,10002):
    x=f(-10*index/10002)
    value=candidate(x);reference=f(math.exp(x));error=abs(value-reference)
    maximum=max(maximum,error);different+=value!=reference
    if index in (1,1000,10000):examples.append(dict(input=x,candidate=value,exp_reference=reference))
assert maximum<2e-7
optional=image.u32(60)+24;export_rva=image.u32(optional+96);exports=[]
if export_rva:
    directory=image.offset(image.base+export_rva)
    function_table=image.u32(directory+28);name_table=image.u32(directory+32);ordinal_table=image.u32(directory+36)
    for index in range(image.u32(directory+24)):
        name=image.string(image.base+image.word(image.base+name_table+index*4))
        ordinal=image.u16(image.offset(image.base+ordinal_table+index*2))
        target=image.base+image.word(image.base+function_table+ordinal*4)
        if target in (0x1045D509,0x1045D660):exports.append(dict(name=name,target=hex(target)))
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),helper='0x1045d509',matching_named_exports=exports,normal_path_samples=10001,max_absolute_error=maximum,different_float32_results=different,scale=scale,range_reduction_constant=double(0x105EF148),examples=examples,interpretation='Recovered normal-range arithmetic is numerically consistent with expf.',limitations='Offline emulation, nearest-even rounding assumption, sampled negative normal range only; exceptional paths and live native function not verified. Not a proof of symbol name or bit-exact replacement.')
Path('outputs/sfm64-client-exp-helper-audit.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('exp comparison samples=10001 max_absolute_error=',maximum,'different=',different)
