import struct
from pathlib import Path
palette=[0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234,1,0x3e800000]
data=Path('work/native-vector-write-v1.bin').read_bytes()
differences=[]
for seed in range(1024):
    old=[];incoming=[];state=seed
    for i in range(4):
        state=(state*1664525+1013904223)&0xffffffff;old.append(palette[state%10])
        state=(state*1664525+1013904223)&0xffffffff;incoming.append(palette[state%10])
    if seed%4==0:incoming=old.copy()
    if seed%16==1:old=[0,0x80000000,0,0];incoming=[0x80000000,0,0x80000000,0]
    result=struct.unpack_from('<32I',data,seed*128)
    if result[29]&0x2000:
        for i in range(3):
            if incoming[i]!=result[24+i]:differences.append((seed,i,hex(incoming[i]),hex(result[24+i])))
print('copy_payload_differences=',len(differences))
print(differences[:8])
