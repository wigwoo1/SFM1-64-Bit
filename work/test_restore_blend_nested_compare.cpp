#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "studio_restore_blend_commit_candidate.h"
static std::array<unsigned char,512> receiver;
static std::array<uint32_t,256> block,events;
static bool mutateFixture;static StudioRestoreBlendStateCandidate* liveState;
static unsigned used,getters,countReads,specialReads,gates,cursor,remaining;static int fixtureSamplerCount;static unsigned special,suppression;static bool queued;
static void event(unsigned op,unsigned a,unsigned b,unsigned c=0){if(used+4>events.size())std::abort();events[used++]=op;events[used++]=a;events[used++]=b;events[used++]=c;}
struct Services{
 void render(unsigned key,unsigned value){++getters;if(queued){block[cursor++]=1;block[cursor++]=key;block[cursor++]=value;remaining-=3;}else{event(1,key,value);if(mutateFixture&&key==27)liveState->f100=99;}}
 void sampler(unsigned stage,unsigned type,unsigned value){++getters;if(queued){block[cursor++]=17;block[cursor++]=stage;block[cursor++]=type;block[cursor++]=value;remaining-=4;}else{event(17,stage,type,value);if(mutateFixture&&stage==0&&type==11){fixtureSamplerCount=0;liveState->samplers[0].second=1;}}}
 int samplerCount(){++countReads;return fixtureSamplerCount;}unsigned specialSamplerEnabled(){++specialReads;return special;}unsigned suppressed(){++gates;if(mutateFixture){liveState->kind=9;liveState->descriptor=0x03112233;}return suppression;}
 void transition(int kind,unsigned offset,unsigned n){event(201,static_cast<unsigned>(kind),offset,n);liveState->commit.dirty134=liveState->commit.dirty135=liveState->commit.dirty13c=liveState->commit.enabled13d=liveState->commit.dirty13e=liveState->commit.enabled13f=1;liveState->commit.value138=123;}void commit(){studioRestoreBlendCommitCandidate(*liveState,*this);}
 void vertexShader(int value){event(200,9,static_cast<unsigned>(value));}void pixelShader(int value){event(200,10,static_cast<unsigned>(value));}
};
static void store(unsigned offset,uint32_t value){std::memcpy(receiver.data()+offset,&value,4);}
int main(){std::ifstream oracle("outputs/native-restore-blend-nested.bin",std::ios::binary);unsigned cases=0;
 for(bool change:{false,true})for(bool queue:{false,true})for(int n:{-1,0,1,2})for(unsigned specValue:{0u,1u,256u})for(unsigned suppressValue:{0u,1u,256u}){
 mutateFixture=change;receiver.fill(0x55);const int16_t kind=-2;std::memcpy(receiver.data()+4,&kind,2);store(8,0x02123456);receiver[0x145]=17;receiver[0x146]=0;receiver[0x148]=19;receiver[0x149]=1;block.fill(99);events.fill(0);used=getters=countReads=specialReads=gates=cursor=0;remaining=256;fixtureSamplerCount=n;special=specValue;suppression=suppressValue;queued=queue;
 StudioRestoreBlendStateCandidate state;state.kind=-2;state.cachedKind=state.cachedHandle=0x5555;state.descriptor=0x02123456;state.samplers={{17,0,0x55},{19,1,0x55}};state.commit.value138=state.commit.cached140=0x55555555;state.commit.enabled13d=state.commit.enabled13f=0x55;liveState=&state;Services services;studioRestoreBlendCandidate(state,services);
 std::memcpy(receiver.data()+4,&state.kind,2);store(8,state.descriptor);receiver[0x146]=state.samplers[0].second;store(0x138,state.commit.value138);store(0x140,state.commit.cached140);receiver[0x13d]=state.commit.enabled13d;receiver[0x13f]=state.commit.enabled13f;const unsigned offsets[]{0x100,0x104,0x108,0x110,0x114,0x118,0x11c,0x120,0x124,0x12c,0x130};const uint32_t values[]{state.f100,state.f104,state.f108,state.f110,state.f114,state.f118,state.f11c,state.f120,state.f124,state.f12c,state.f130};for(unsigned i=0;i<11;++i)store(offsets[i],values[i]);receiver[0xfc]=state.ffc;receiver[0x10c]=state.f10c;receiver[0x128]=state.f128;receiver[0x144]=state.f144;receiver[0x134]=state.commit.dirty134;receiver[0x135]=state.commit.dirty135;receiver[0x13c]=state.commit.dirty13c;receiver[0x13e]=state.commit.dirty13e;std::memcpy(receiver.data()+0x54,&state.cachedKind,2);std::memcpy(receiver.data()+0x56,&state.cachedHandle,2);
 const unsigned stages=n>0?static_cast<unsigned>(n):0u;if(getters!=(static_cast<uint8_t>(suppression)?13u:17u)+stages*(static_cast<uint8_t>(special)?2u:1u)||countReads!=1||specialReads!=stages||gates!=1)return 1;
 std::array<uint32_t,647> record{},native{};std::memcpy(record.data(),receiver.data(),receiver.size());std::memcpy(record.data()+128,block.data(),sizeof(block));std::memcpy(record.data()+384,events.data(),sizeof(events));record[640]=used;record[641]=getters;record[642]=countReads;record[643]=specialReads;record[644]=gates;record[645]=cursor;record[646]=remaining;oracle.read(reinterpret_cast<char*>(native.data()),sizeof(native));if(!oracle)return 2;if(record!=native){for(unsigned i=0;i<record.size();++i)if(record[i]!=native[i]){std::printf("case=%u word=%u candidate=%08x native=%08x\n",cases,i,record[i],native[i]);break;}return 3;}++cases;
 }if(oracle.peek()!=std::char_traits<char>::eof())return 4;std::printf("restore_blend_nested_x64_comparison_cases=%u passed\n",cases);return 0;}


