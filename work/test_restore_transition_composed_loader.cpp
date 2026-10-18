#define NOMINMAX
#include <Windows.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <array>
#include "restore_transition_composed_native_init.h"
#include "studio_transition_backend_handlers_candidate.h"
static void* cacheCell;static void* capabilityCell;static void* backendCell;static void* suppressionCell;static void* shaderCell;
struct Command{void* device;unsigned pad[3];unsigned mode,pad14;unsigned* cursor;unsigned remaining;};
static Command command;static unsigned getters;
static unsigned gateValue,gateReads,stageReads,shaderCalls;
static unsigned depthCalls,samplerStages,specialValue,descriptorMutation;
static unsigned* directOutput;static unsigned directUsed;static unsigned* liveReceiver;
static HRESULT __stdcall renderDirect(void*,unsigned key,unsigned value){directOutput[directUsed++]=1;directOutput[directUsed++]=key;directOutput[directUsed++]=value;if(key==27)liveReceiver[0x100/4]=99;return E_FAIL;}
static HRESULT __stdcall samplerDirect(void*,unsigned stage,unsigned key,unsigned value){directOutput[directUsed++]=17;directOutput[directUsed++]=stage;directOutput[directUsed++]=key;directOutput[directUsed++]=value;if(stage==0&&key==11){samplerStages=0;reinterpret_cast<unsigned char*>(liveReceiver)[0x148]=23;}return E_FAIL;}
static unsigned __fastcall noSpecial(void*,void*){return specialValue;}
static void __fastcall depthBackend(void*,void*,StudioTransitionState72Candidate* state){if(state->flags44!=255)std::abort();++depthCalls;}
static int __fastcall samplerCount(void*,void*){++stageReads;return static_cast<int>(samplerStages);}
static unsigned __fastcall suppressed(void*,void*){++gateReads;if(descriptorMutation)liveReceiver[8/4]=0;return gateValue;}
static void __fastcall shader(void*,void*,int value){if(value!=-1)std::abort();++shaderCalls;}
static void* __cdecl getter(){++getters;return &command;}
static void __fastcall refill(void*,void*){std::abort();}
int main(){
 static_assert(sizeof(void*)==4,"Original execution loader requires x86");
 std::ifstream file("outputs/restore-transition-composed-original.bin",std::ios::binary);std::vector<unsigned char> saved(restoreTransitionComposedSize);file.read(reinterpret_cast<char*>(saved.data()),saved.size());if(!file||file.peek()!=std::char_traits<char>::eof())return 1;
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 for(const auto& body:restoreTransitionComposedBodies){std::vector<unsigned char> actual(body[1]);dll.clear();dll.seekg(body[2]);dll.read(reinterpret_cast<char*>(actual.data()),actual.size());if(!dll||std::memcmp(actual.data(),saved.data()+body[0],actual.size()))return 2;}
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 3;std::memcpy(code,saved.data(),saved.size());
 void* ordinary[13]{};void* packed[2]{};for(unsigned i=0;i<13;++i)ordinary[i]=code+restoreTransitionComposedOrdinary[i];for(unsigned i=0;i<2;++i)packed[i]=code+restoreTransitionComposedPacked[i];
 void* bindings[]{&cacheCell,&capabilityCell,&backendCell,ordinary,packed,&suppressionCell,&shaderCell};
 for(const auto& call:restoreTransitionComposedCalls){auto* target=call[2]==0?code+call[1]:call[2]==1?reinterpret_cast<unsigned char*>(&getter):reinterpret_cast<unsigned char*>(&refill);const auto displacement=static_cast<unsigned>(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&displacement,4);unsigned read;std::memcpy(&read,code+call[0]+1,4);if(reinterpret_cast<uintptr_t>(code+call[0]+5)+read!=reinterpret_cast<uintptr_t>(target))return 4;}
 for(const auto& fix:restoreTransitionComposedFixups){const auto address=reinterpret_cast<uintptr_t>(bindings[fix[1]]);std::memcpy(code+fix[0],&address,4);unsigned read;std::memcpy(&read,code+fix[0],4);if(read!=address)return 5;}
 if(ordinary[9]!=ordinary[11])return 6;
 DWORD old;if(!VirtualProtect(code,8192,PAGE_EXECUTE_READ,&old))return 7;FlushInstructionCache(GetCurrentProcess(),code,8192);
 MEMORY_BASIC_INFORMATION info{};if(!VirtualQuery(code,&info,sizeof(info))||info.Protect!=PAGE_EXECUTE_READ)return 8;
 void* backendTable[266]{};backendTable[265]=reinterpret_cast<void*>(&samplerCount);backendTable[261]=reinterpret_cast<void*>(&depthBackend);void* backendReceiver=backendTable;backendCell=&backendReceiver;
 void* gateTable[37]{};gateTable[36]=reinterpret_cast<void*>(&suppressed);void* gateReceiver=gateTable;suppressionCell=&gateReceiver;
 void* shaderTable[11]{};shaderTable[9]=shaderTable[10]=reinterpret_cast<void*>(&shader);void* shaderReceiver=shaderTable;shaderCell=&shaderReceiver;
 const std::vector<unsigned> expected{1,27,0,1,19,2,1,20,1,1,171,1,1,206,0,1,207,2,1,208,1,1,209,1,1,7,1,1,23,4,1,15,0,1,25,7,1,24,0};
 std::ofstream oracle("outputs/native-restore-transition-composed.bin",std::ios::binary|std::ios::trunc);
 void* capTable[50]{};capTable[49]=reinterpret_cast<void*>(&noSpecial);void* capReceiver=capTable;capabilityCell=&capReceiver;
 void* deviceTable[70]{};deviceTable[57]=reinterpret_cast<void*>(&renderDirect);deviceTable[69]=reinterpret_cast<void*>(&samplerDirect);void* deviceReceiver=deviceTable;
 unsigned cases=0;for(unsigned change:{0u,1u})for(unsigned mode:{0u,1u})for(unsigned special:{0u,1u,256u})for(unsigned stages:{0u,1u,2u})for(unsigned nonempty:{0u,1u})for(unsigned gate:{0u,1u,256u}){
  std::array<unsigned,128> receiver{};std::array<unsigned,128> queue{};queue.fill(0xcccccccc);cacheCell=receiver.data();auto* bytes=reinterpret_cast<unsigned char*>(receiver.data());bytes[0x54]=9;bytes[0x56]=12;bytes[0x134]=bytes[0x135]=bytes[0x13c]=bytes[0x13e]=bytes[0x144]=1;
  StudioTransitionState72Candidate model{};model.flags44=255;model.flags45=1;model.values[0]=3;model.values[1]=1;model.values[4]=model.values[7]=model.values[10]=7;model.values[5]=model.values[8]=model.values[11]=13;model.values[6]=model.values[9]=17;const unsigned char operations[]{255,0,3,6,7};
  receiver[0x58/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(&model));receiver[0xb8/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(operations));receiver[8/4]=nonempty?0x04000001:0;
  descriptorMutation=change;specialValue=special;samplerStages=stages;bytes[0x145]=17;bytes[0x148]=19;bytes[0x149]=1;std::array<unsigned,128> untouchedQueue{};untouchedQueue.fill(0xabababab);liveReceiver=receiver.data();directOutput=queue.data();directUsed=0;command={};command.device=&deviceReceiver;command.mode=mode;command.cursor=mode?queue.data():untouchedQueue.data();command.remaining=128;getters=gateReads=stageReads=shaderCalls=depthCalls=0;gateValue=gate;
  reinterpret_cast<void(__thiscall*)(void*)>(code+restoreTransitionComposedEntry)(receiver.data());
  const bool transitions=nonempty&&gate!=1&&!change;auto expectedCase=expected;if(mode==0)expectedCase[5]=99;for(unsigned stage=0;stage<stages;++stage){expectedCase.insert(expectedCase.end(),{17,stage,11,stage==0?17u:mode==0?23u:19u});if(special==1)expectedCase.insert(expectedCase.end(),{17,stage,8,stage==0?0x31544547u:0x34544547u});}if(transitions){const std::vector<unsigned> tail{1,23,3,1,15,1,1,24,13,1,27,1,1,19,7,1,20,13,1,171,17,1,206,1,1,207,7,1,208,13,1,209,17};expectedCase.insert(expectedCase.end(),tail.begin(),tail.end());}
  if(getters!=stages*(special==1?2u:1u)+(transitions?24u:13u)||depthCalls!=unsigned(transitions)||stageReads!=1||gateReads!=1||shaderCalls!=2||command.cursor!=(mode?queue.data()+expectedCase.size():untouchedQueue.data())||command.remaining!=(mode?128-expectedCase.size():128))return 9;
  for(unsigned i=0;i<128;++i)if(queue[i]!=(i<expectedCase.size()?expectedCase[i]:0xcccccccc))return 10;
  if(bytes[0x54]!=(gate==1?9:0)||bytes[0x55]!=0||bytes[0x56]!=255||bytes[0x57]!=255||bytes[0x134]||bytes[0x135]||bytes[0x13c]||bytes[0x13e]||bytes[0x144])return 11;
  if(mode==0){if(directUsed!=expectedCase.size())return 15;for(auto word:untouchedQueue)if(word!=0xabababab)return 16;}
  if(receiver[0x100/4]!=(transitions?7u:mode==0?99u:2u)||receiver[0x104/4]!=(transitions?13u:1u)||receiver[0x108/4]!=(transitions?17u:1u)||receiver[0x110/4]!=(transitions?7u:2u)||receiver[0x114/4]!=(transitions?13u:1u)||receiver[0x118/4]!=(transitions?17u:1u)||receiver[0x11c/4]!=1||receiver[0x120/4]!=(transitions?3u:4u)||receiver[0x12c/4]!=7)return 12;
  if(receiver[0x58/4]!=reinterpret_cast<uintptr_t>(&model)||receiver[0xb8/4]!=reinterpret_cast<uintptr_t>(operations))return 13;receiver[0x58/4]=receiver[0xb8/4]=0;
  std::array<unsigned,280> record{};std::memcpy(record.data(),receiver.data(),512);std::memcpy(record.data()+128,queue.data(),512);std::memcpy(record.data()+256,&model,72);record[274]=getters;record[275]=gateReads;record[276]=stageReads;record[277]=shaderCalls;record[278]=depthCalls;record[279]=command.remaining;oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!oracle)return 14;++cases;
 }
 VirtualFree(code,0,MEM_RELEASE);std::printf("restore_transition_composed_parent_cases=%u passed actual_handler_operations\n",cases);
}






