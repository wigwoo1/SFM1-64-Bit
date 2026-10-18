#include "float_array_write_sequence.h"
#include "float_array_write_fixture.h"
#include "float_array_owner_notification.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream oracle(argv[1],std::ios::binary);
 unsigned caseCount=20160;
#ifdef TEST_OWNER_GATES
 caseCount*=6;
#endif
 for(unsigned seed=0;seed<caseCount;++seed){auto f=floatArrayWriteFixture(seed%20160);std::vector<uint32_t> storage(f.old.begin(),f.old.end());std::array<uint32_t,32> actual{},expected{};unsigned cursor=0;uint16_t flags=17;
#ifdef TEST_OWNER_GATES
 unsigned mode=seed/20160;uint8_t ownerFlags=0;
 if(mode>=4||(mode&1))flags|=0x100;
 if(mode>=4||(mode&2))ownerFlags|=4;
#endif
 storage.resize(size_t(f.count));
 auto backing=[&]{auto values=f.old;for(size_t i=0;i<storage.size();++i)values[i]=storage[i];return values;};
 auto emit=[&](uint32_t c,uint32_t arg){actual[cursor++]=c;actual[cursor++]=arg;auto values=backing();actual[cursor++]=floatArrayWriteHash(values.data());actual[cursor++]=flags;};
 auto before=[&](int a,int b){
#ifdef TEST_OWNER_GATES
 recoveredFloatArrayOwnerNotificationCandidate(flags,ownerFlags,a,b,[&](int first,int last){emit(3,uint32_t(first)|(uint32_t(last)<<8));if(mode==4)flags&=0xfeffu;if(mode==5)ownerFlags&=0xfbu;});
#else
 emit(3,uint32_t(a)|(uint32_t(b)<<8));
#endif
 };
 auto after=[&](int a,int b,int c){
#ifdef TEST_OWNER_GATES
 (void)c;recoveredFloatArrayOwnerNotificationCandidate(flags,ownerFlags,a,b,[&](int first,int last){emit(4,uint32_t(first)|(uint32_t(last)<<8));});
#else
 emit(4,uint32_t(a)|(uint32_t(b)<<8)|(uint32_t(c)<<16));
#endif
 };
 if(f.sourceType==3)writeRecoveredFloatArrayCandidate(storage,f.index,f.incoming,[&]{emit(1,0);if(f.allowed)flags|=0x4080;return f.allowed;},[&]{emit(2,0);},before,after,[&]{emit(5,0);});
 auto values=backing();for(unsigned i=0;i<4;++i)actual[24+i]=values[i];actual[28]=flags;actual[30]=cursor;
 if(!oracle.read(reinterpret_cast<char*>(expected.data()),sizeof(expected)))throw std::runtime_error("short oracle");if(actual!=expected)throw std::runtime_error("native mismatch seed="+std::to_string(seed));}
 if(oracle.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64 float-array write: "<<caseCount<<" native payload/event comparisons passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
