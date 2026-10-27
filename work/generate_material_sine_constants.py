"""Generate mathematical lookup data from decimal Machin pi and Taylor series.
No original files, executable bytes, or external packages are used.
"""
from decimal import Decimal,localcontext
from pathlib import Path
import struct,sys

def constants():
    with localcontext() as context:
        context.prec=240
        def atan(x):
            total=term=x;n=1
            while True:
                term*=-(x*x);piece=term/Decimal(2*n+1);nextValue=total+piece
                if nextValue==total:return total
                total=nextValue;n+=1
        pi=16*atan(Decimal(1)/5)-4*atan(Decimal(1)/239)
        def trig(x,cosine=False):
            total=term=Decimal(1) if cosine else x;n=1
            while True:
                term*=-(x*x)/Decimal((2*n-1)*(2*n) if cosine else (2*n)*(2*n+1));nextValue=total+term
                if nextValue==total:return total
                total=nextValue;n+=1
        table=[]
        for i in range(64):
            quadrant,step=divmod(i,16);x=pi*step/32
            sn,cs=trig(x),trig(x,True)
            if quadrant==1:sn,cs=cs,-sn
            elif quadrant==2:sn,cs=-sn,-cs
            elif quadrant==3:sn,cs=-cs,sn
            if step==0:
                sn=[Decimal(0),Decimal(1),Decimal(0),Decimal(-1)][quadrant]
                cs=[Decimal(1),Decimal(0),Decimal(-1),Decimal(0)][quadrant]
            d=Decimal(0)
            if cs:
                d=Decimal(1)
                while d>abs(cs):d/=2
                if abs(cs)-d>2*d-abs(cs):d*=2
                if cs<0:d=-d
            head=float(sn);tail=struct.unpack('<f',struct.pack('<f',float(sn-Decimal.from_float(head))))[0]
            table.append([float(cs-d),head,float(tail),float(d)])
        def truncate(value,bits):
            raw=struct.unpack('<Q',struct.pack('<d',float(value)))[0]&~((1<<bits)-1)
            return struct.unpack('<d',struct.pack('<Q',raw))[0]
        high=truncate(pi/32,22);middle=truncate(pi/32-Decimal.from_float(high),21)
        return table,[high,middle,float(pi/32-Decimal.from_float(high)-Decimal.from_float(middle)),float(32/pi)]

def render():
    table,split=constants();rows=['#pragma once','// Independently generated mathematical data; see generate_material_sine_constants.py.','inline constexpr double materialSineTableCandidate[64][4]={']
    rows+=['    {'+','.join(x.hex() for x in row)+'},' for row in table]
    rows+=['};','inline constexpr double materialSinePiSplitCandidate[4]={'+','.join(x.hex() for x in split)+'};','']
    return '\n'.join(rows)

if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('output-header required')
    Path(sys.argv[1]).write_text(render(),encoding='ascii')
