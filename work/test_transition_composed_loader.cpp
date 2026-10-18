#define NOMINMAX
#include <Windows.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <array>
#include "transition_composed_native_init.h"
#include "studio_transition_backend_handlers_candidate.h"
static void* cacheCell;static void* capabilityCell;static void* backendCell;
struct Command{void* device;unsigned pad[3];unsigned mode,pad14;unsigned* cursor;unsigned remaining;};
static Command command;static unsigned getters;
static std::vector<unsigned> trace;
static unsigned* directWords;static unsigned directUsed;static StudioTransitionState72Candidate* liveState;
static HRESULT __stdcall renderDirect(void*,unsigned key,unsigned value){directWords[directUsed++]=1;directWords[directUsed++]=key;directWords[directUsed++]=value;if(key==194)liveState->value40=99;return E_FAIL;}
static HRESULT __stdcall samplerDirect(void*,unsigned stage,unsigned key,unsigned value){directWords[directUsed++]=17;directWords[directUsed++]=stage;directWords[directUsed++]=key;directWords[directUsed++]=value;return E_FAIL;}
static int __fastcall count(void*,void*){trace.push_back(1);return 2;}
static unsigned __fastcall cap41(void*,void*){trace.push_back(41);return 1;}
static int __fastcall cap17(void*,void*){trace.push_back(17);return 92;}
static unsigned __fastcall cap25(void*,void*){trace.push_back(25);return 1;}
static unsigned __fastcall cap49(void*,void*){trace.push_back(49);return 1;}
static void __fastcall depth(void*,void*,StudioTransitionState72Candidate* state){trace.insert(trace.end(),{261,(state->flags44>>2)&3u});}
static void __fastcall stage(void*,void*,StudioTransitionState72Candidate*,int index){trace.insert(trace.end(),{262,static_cast<unsigned>(index)});}
static void __fastcall flag44(void*,void*,unsigned value){trace.insert(trace.end(),{263,value});}
static void __fastcall flag45(void*,void*,unsigned value){trace.insert(trace.end(),{272,value});}
static void __fastcall backend271(void*,void*,unsigned value){trace.insert(trace.end(),{271,value});}
static void __fastcall combined(void*,void*,unsigned value,unsigned a,unsigned b,unsigned c){trace.insert(trace.end(),{264,value,a,b,c});}
static void* __cdecl getter(){++getters;return &command;}
static void __fastcall refill(void*,void*){std::abort();}
int main(){
 static_assert(sizeof(void*)==4,"Original execution loader requires x86");
 std::ifstream file("outputs/transition-composed-original.bin",std::ios::binary);std::vector<unsigned char> saved(transitionComposedSize);file.read(reinterpret_cast<char*>(saved.data()),saved.size());if(!file||file.peek()!=std::char_traits<char>::eof())return 1;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 for(const auto& body:transitionComposedBodies){std::vector<unsigned char> actual(body[1]);dll.clear();dll.seekg(body[2]);dll.read(reinterpret_cast<char*>(actual.data()),actual.size());if(!dll||std::memcmp(actual.data(),saved.data()+body[0],actual.size()))return 2;}
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;std::memcpy(code,saved.data(),saved.size());
 void* ordinary[13]{};void* packed[2]{};for(unsigned i=0;i<13;++i)ordinary[i]=code+transitionComposedOrdinary[i];for(unsigned i=0;i<2;++i)packed[i]=code+transitionComposedPacked[i];
 void* bindings[]{&cacheCell,&capabilityCell,&backendCell,ordinary,packed};
 for(const auto& call:transitionComposedCalls){auto* target=call[2]==0?code+call[1]:call[2]==1?reinterpret_cast<unsigned char*>(&getter):reinterpret_cast<unsigned char*>(&refill);const auto displacement=static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&displacement,4);unsigned read;std::memcpy(&read,code+call[0]+1,4);if(reinterpret_cast<uintptr_t>(code+call[0]+5)+read!=reinterpret_cast<uintptr_t>(target))return 4;}
 for(const auto& fix:transitionComposedFixups){const auto address=reinterpret_cast<uintptr_t>(bindings[fix[1]]);std::memcpy(code+fix[0],&address,4);unsigned read;std::memcpy(&read,code+fix[0],4);if(read!=address)return 5;}
 if(ordinary[9]!=ordinary[11])return 6;
 DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 7;FlushInstructionCache(GetCurrentProcess(),code,4096);
 MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(code,&info,sizeof(info))||info.Protect!=PAGE_EXECUTE_READ)return 8;
 using Dispatcher=void(__thiscall*)(void*,unsigned,unsigned,int);
 auto dispatch=reinterpret_cast<Dispatcher>(code+transitionComposedEntry);
 dispatch(nullptr,0,0,-1);if(getters)return 9;
 std::array<StudioTransitionState72Candidate,3> states{};auto& state=states[1];state.flags44=2;state.values[2]=17;state.values[3]=23;state.values[15]=0x02000000;
 std::array<unsigned,128> cache{};cacheCell=cache.data();std::array<unsigned,32> queue{};queue.fill(0xcccccccc);
 command={};command.mode=1;command.cursor=queue.data();command.remaining=32;
 const unsigned char operations[]{0xff,0xff,1,2,4,0x8f,0xff};std::array<unsigned,64> owner{};
 owner[0x58/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(states.data()));owner[0xb8/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(operations));
 dispatch(owner.data(),1,2,4);
 const std::array<unsigned,13> expected{1,14,1,1,168,17,1,8,23,17,15,11,1};
 if(getters!=4||command.cursor!=queue.data()+13||command.remaining!=19||cache[0x140/4]!=17)return 10;
 for(unsigned i=0;i<32;++i)if(queue[i]!=(i<expected.size()?expected[i]:0xcccccccc))return 11;
 for(unsigned i:{0u,2u}){const StudioTransitionState72Candidate empty{};if(std::memcmp(&states[i],&empty,sizeof(empty)))return 12;}
 void* capabilityTable[50]{};capabilityTable[1]=reinterpret_cast<void*>(&count);capabilityTable[41]=reinterpret_cast<void*>(&cap41);capabilityTable[17]=reinterpret_cast<void*>(&cap17);capabilityTable[25]=reinterpret_cast<void*>(&cap25);capabilityTable[49]=reinterpret_cast<void*>(&cap49);void* capabilityReceiver=capabilityTable;capabilityCell=&capabilityReceiver;
 void* backendTable[273]{};backendTable[261]=reinterpret_cast<void*>(&depth);backendTable[262]=reinterpret_cast<void*>(&stage);backendTable[263]=reinterpret_cast<void*>(&flag44);backendTable[272]=reinterpret_cast<void*>(&flag45);backendTable[271]=reinterpret_cast<void*>(&backend271);backendTable[264]=reinterpret_cast<void*>(&combined);void* backendReceiver=backendTable;backendCell=&backendReceiver;
 state={};state.flags44=255;state.flags45=13;state.values[0]=3;state.values[1]=1;state.values[2]=17;state.values[3]=23;state.values[4]=state.values[7]=state.values[10]=7;state.values[5]=state.values[8]=state.values[11]=13;state.values[6]=state.values[9]=17;state.value40=17;state.values[15]=0x06000000;
 cache.fill(0);std::array<unsigned,128> allQueue{};allQueue.fill(0xcccccccc);command.cursor=allQueue.data();command.remaining=128;getters=0;trace.clear();
 const unsigned char allOperations[]{0,1,2,3,4,5,6,7,8,9,10,11,12,0x8f,0x9f};owner[0xb8/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(allOperations));dispatch(owner.data(),1,0,15);
 const std::vector<unsigned> allExpected{1,7,1,1,23,3,1,14,1,1,168,17,1,15,1,1,25,7,1,24,13,1,8,23,1,194,0,1,27,1,1,19,7,1,20,13,1,171,17,1,206,1,1,207,7,1,208,13,1,209,17,17,15,11,1,17,15,8,0x34544547};
 const std::vector<unsigned> expectedTrace{261,3,41,17,271,1,25,264,17,1,1,1,263,1,264,17,1,1,1,1,262,0,262,1,264,17,1,1,1,272,1,49};
 if(trace!=expectedTrace||getters!=19||command.cursor!=allQueue.data()+allExpected.size()||command.remaining!=128-allExpected.size())return 13;
 for(unsigned i=0;i<128;++i)if(allQueue[i]!=(i<allExpected.size()?allExpected[i]:0xcccccccc))return 14;
 if(cache[0x11c/4]!=1||cache[0x120/4]!=3||cache[0x124/4]!=3||cache[0x140/4]!=17)return 15;
 std::array<unsigned,340> record{};std::memcpy(record.data(),&state,72);std::memcpy(record.data()+18,cache.data(),512);std::memcpy(record.data()+146,allQueue.data(),512);for(unsigned i=0;i<trace.size();++i)record[274+i]=trace[i];record[338]=getters;record[339]=static_cast<unsigned>(trace.size());std::ofstream oracle("outputs/native-transition-composed.bin",std::ios::binary|std::ios::trunc);oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!oracle)return 16;
 allQueue.fill(0xcccccccc);command.cursor=allQueue.data();command.remaining=128;getters=0;trace.clear();dispatch(owner.data(),1,0,15);
 const std::vector<unsigned> repeatExpected{1,14,1,1,168,17,1,8,23,1,194,0,17,15,11,1,17,15,8,0x34544547};
 const std::vector<unsigned> repeatTrace(expectedTrace.begin()+2,expectedTrace.end());
 if(trace!=repeatTrace||getters!=6||command.cursor!=allQueue.data()+20||command.remaining!=108)return 17;
 for(unsigned i=0;i<128;++i)if(allQueue[i]!=(i<repeatExpected.size()?repeatExpected[i]:0xcccccccc))return 18;
 record.fill(0);std::memcpy(record.data(),&state,72);std::memcpy(record.data()+18,cache.data(),512);std::memcpy(record.data()+146,allQueue.data(),512);for(unsigned i=0;i<trace.size();++i)record[274+i]=trace[i];record[338]=getters;record[339]=static_cast<unsigned>(trace.size());oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!oracle)return 19;
 cache.fill(0);state.value40=17;trace.clear();getters=0;std::array<unsigned,128> directOutput{};directOutput.fill(0xcccccccc);directWords=directOutput.data();directUsed=0;liveState=&state;
 void* deviceTable[70]{};deviceTable[57]=reinterpret_cast<void*>(&renderDirect);deviceTable[69]=reinterpret_cast<void*>(&samplerDirect);void* deviceReceiver=deviceTable;command.device=&deviceReceiver;command.mode=0;allQueue.fill(0xcccccccc);command.cursor=allQueue.data();command.remaining=128;dispatch(owner.data(),1,0,15);
 if(directUsed!=59||getters!=19||state.value40!=99||command.cursor!=allQueue.data()||command.remaining!=128)return 20;for(auto word:allQueue)if(word!=0xcccccccc)return 21;for(unsigned i=0;i<59;++i)if(directOutput[i]!=allExpected[i])return 22;
 record.fill(0);std::memcpy(record.data(),&state,72);std::memcpy(record.data()+18,cache.data(),512);std::memcpy(record.data()+146,directOutput.data(),512);for(unsigned i=0;i<trace.size();++i)record[274+i]=trace[i];record[338]=getters;record[339]=static_cast<unsigned>(trace.size());oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!oracle)return 23;
 VirtualFree(code,0,MEM_RELEASE);std::puts("transition_composed_loader=passed queued_repeat_and_direct=passed");
}

