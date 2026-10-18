#include "buffer_owned_constructor_candidate.h"
#include "buffer_write_check_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{
 unsigned char bytes[260],error=9,flags=0;void* storage=bytes;
 unsigned cap=0,grow=0,state=0xa7,get=9,put=9,field=9,offset=9,max=9;bool read=false,write=false,terminated=false,overflowOk=false;unsigned overflowCalls=0;
 void setStorage(void* p){storage=p;}void setCapacity(unsigned v){cap=v;}void setGrowSize(unsigned v){grow=v;}
 void* allocate(unsigned v){assert(v==cap && !storage);return bytes;}
 void setErrorByte(unsigned char v){error=v;}unsigned state2c(){return state;}void setState2c(unsigned v){state=v;}
 void setGetPosition(unsigned v){get=v;}void setPutPosition(unsigned v){put=v;}void setField18(unsigned v){field=v;}
 void setBufferOffset(unsigned v){offset=v;}void setFlagsByte(unsigned char v){flags=v;}void setMaxGet(unsigned v){max=v;}
 unsigned char errorByte(){return error;}unsigned char flagsByte(){return flags;}unsigned putPosition(){return put;}unsigned bufferOffset(){return offset;}unsigned capacity(){return cap;}unsigned maxGet(){return max;}
 bool writeOverflow(unsigned n){assert(n==1);++overflowCalls;if(overflowOk){put=offset=3;cap=260;}return overflowOk;}
 void writeByte(unsigned i,unsigned char v){assert(i<260);bytes[i]=v;}
 void terminateBuffer(){assert(max==0xffffffff && !read && !write && !error && !get && !put && !field && !offset);terminated=true;terminateBufferCandidate(*this);}
 void installReadCallback(){read=true;}void installWriteCallback(){assert(read);write=true;}
};
int main(){for(unsigned flags=0;flags<256;++flags)for(unsigned cap:{0u,260u}){
 Adapter a;constructOwnedBufferCandidate(17,cap,flags|0x1200,a);
 assert(a.cap==cap && a.grow==17 && a.flags==flags && a.state==0xa4 && !a.max && a.read && a.write);
 assert(a.terminated==(cap!=0 && !(flags&8)));assert(a.storage==(cap?a.bytes:nullptr));
 }
 Adapter b;b.error=0;b.flags=0;b.put=1;b.offset=0;b.cap=0;b.max=0;terminateBufferCandidate(b);assert(b.max==1 && b.error==0 && b.overflowCalls==1);
 Adapter c;c.error=0;c.flags=0;c.put=1;c.offset=0;c.cap=0;c.max=0;c.overflowOk=true;terminateBufferCandidate(c);assert(c.max==3 && c.bytes[0]==0 && c.overflowCalls==1);
 Adapter d;d.error=1;d.flags=0;d.put=2;d.max=0;terminateBufferCandidate(d);assert(d.max==2 && d.error==1 && !d.overflowCalls);
 std::cout<<"x64 owned buffer constructor: all flags, zero/nonzero capacities, retained state bits and callback ordering passed\n";
}
