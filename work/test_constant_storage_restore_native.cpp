#define NOMINMAX
#include <Windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include "constant_storage_restore_native_init.h"
#include "studio_constant_storage_restore_candidate.h"
#include "scalar_constant_native_init.h"
#include "studio_scalar_constant_submit_candidate.h"
#include "studio_device_queue_refill_candidate.h"
#include "constant_restore_defaults_native_init.h"
#include "studio_constant_restore_defaults_candidate.h"
#include "shader_binding_native_init.h"
#include "manager_unbind_prefix_native_init.h"
#include "shader_manager_restore_reset_native_init.h"
#include "native_shader_binding_state_candidate.h"
static const unsigned primaryOffsets[]={0x19ec,0x19f0,0x19f4,0x19e0,0x19e4,0x19e8};
static const unsigned stagingOffsets[]={0x2864,0x2868,0x286c,0x2858,0x285c,0x2860};
static std::vector<uint32_t> events;static unsigned allocations,frees;static uint32_t* originalOwner;
static uint32_t counts[6];static bool queued;static StudioConstantStorageOwnerCandidate* storage;
static void* __cdecl allocate(uint32_t bytes){events.insert(events.end(),{1,allocations,bytes});++allocations;void* p=std::malloc(bytes?bytes:1);if(!p)std::abort();std::memset(p,0xaa,bytes?bytes:1);return p;}
static void __cdecl release(void* p){events.insert(events.end(),{2,allocations});++frees;std::free(p);}
#ifdef _M_IX86
static void syncStorage(){
 for(unsigned id=0;id<6;++id){storage->primary[id]=reinterpret_cast<void*>(originalOwner[primaryOffsets[id]/4]);storage->staging[id]=reinterpret_cast<void*>(originalOwner[stagingOffsets[id]/4]);}
}
#endif
static HRESULT scalarDirect(unsigned opcode,uint32_t start,const uint32_t* data,uint32_t count){const unsigned width=opcode==8||opcode==11?1:4;if(start)std::abort();for(unsigned i=0;i<count*width;++i)if(data[i])std::abort();events.insert(events.end(),{3,opcode,count});return E_FAIL;}
struct Shader{};
static uint32_t shaderCounter,shaderCaptures;
static HRESULT shaderObserve(bool vertex,Shader* shader){if(shader)std::abort();events.insert(events.end(),{7,vertex?1u:2u});return E_FAIL;}
#ifdef _M_IX86
static uint32_t* __cdecl shaderGetter(){++shaderCaptures;return originalOwner+0x648/4;}
static HRESULT __stdcall shaderVertex(void*,Shader* shader){return shaderObserve(true,shader);}
static HRESULT __stdcall shaderPixel(void*,Shader* shader){return shaderObserve(false,shader);}
static std::array<unsigned char*,2> shaderCode{},shaderPrefixCode{};
static unsigned char* shaderManagerCode=nullptr;
#endif
#ifdef _M_IX86
static void* __cdecl copy(void* dst,const void* src,uint32_t size){return std::memmove(dst,src,size);}
static void __fastcall refill(void*,void*){std::abort();}
static HRESULT __stdcall vertexB(void*,uint32_t s,const uint32_t* p,uint32_t n){return scalarDirect(11,s,p,n);}
static HRESULT __stdcall vertexI(void*,uint32_t s,const uint32_t* p,uint32_t n){return scalarDirect(12,s,p,n);}
static HRESULT __stdcall pixelB(void*,uint32_t s,const uint32_t* p,uint32_t n){return scalarDirect(8,s,p,n);}
static HRESULT __stdcall pixelI(void*,uint32_t s,const uint32_t* p,uint32_t n){return scalarDirect(9,s,p,n);}
static std::array<unsigned char*,4> scalarCode{};
static bool prepareScalarBodies(std::ifstream& dll){
 for(unsigned i=0;i<4;++i){const auto& body=scalarBodies[i];std::array<unsigned char,512> bytes{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),body.size);if(!dll||std::memcmp(bytes.data(),body.bytes,body.size))return false;scalarCode[i]=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!scalarCode[i])return false;std::memcpy(scalarCode[i],body.bytes,body.size);
  for(unsigned c=0;c<body.callCount;++c){const auto& call=body.calls[c];const uintptr_t target=call[1]==0x100df030?reinterpret_cast<uintptr_t>(&copy):reinterpret_cast<uintptr_t>(&refill);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(scalarCode[i]+call[0]+5));std::memcpy(scalarCode[i]+call[0]+1,&delta,4);}DWORD previous;if(!VirtualProtect(scalarCode[i],4096,PAGE_EXECUTE_READ,&previous))return false;FlushInstructionCache(GetCurrentProcess(),scalarCode[i],4096);
 }return true;
}
static unsigned char* defaultsCode=nullptr;
static void __fastcall checkCookie(uint32_t value,void*){if(value!=0x12345678)std::abort();}
static bool prepareDefaultsBody(std::ifstream& dll){
 std::array<unsigned char,sizeof(restoreDefaultsBytes)> bytes{};dll.seekg(restoreDefaultsFileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),restoreDefaultsBytes,bytes.size()))return false;
 defaultsCode=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!defaultsCode)return false;std::memcpy(defaultsCode,bytes.data(),bytes.size());static uint32_t cookieValue=0x12345678;
 for(auto& fixup:restoreDefaultsFixups){const void* target=nullptr;if(fixup[1]==0x1017c370)target=&cookieValue;else for(auto& constant:restoreDefaultsConstants)if(constant[0]==fixup[1])target=&constant[1];if(!target)return false;const uint32_t address=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target));std::memcpy(defaultsCode+fixup[0],&address,4);}
 for(auto& call:restoreDefaultsCalls){const uintptr_t target=call[1]==0x10002da0?reinterpret_cast<uintptr_t>(&refill):reinterpret_cast<uintptr_t>(&checkCookie);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(defaultsCode+call[0]+5));std::memcpy(defaultsCode+call[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(defaultsCode,4096,PAGE_EXECUTE_READ,&old))return false;FlushInstructionCache(GetCurrentProcess(),defaultsCode,4096);return true;
}
static void __cdecl shaderForbiddenExit(){std::abort();}
static bool prepareShaderChain(std::ifstream& dll){
 static uint32_t initialized=1;static uint32_t* shaderCounterPointer=&shaderCounter;static uint32_t unused=0;
 for(unsigned i=0;i<2;++i){const auto& body=shaderBindingBodies[i];std::array<unsigned char,178> bytes{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),body.bytes,bytes.size()))return false;shaderCode[i]=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!shaderCode[i])return false;std::memcpy(shaderCode[i],body.bytes,body.size);
  for(unsigned f=0;f<body.fixupCount;++f){const auto& fixup=body.fixups[f];const void* target=fixup[1]==0x10184218||fixup[1]==0x10184220?static_cast<void*>(&initialized):fixup[1]==0x10184214||fixup[1]==0x1018421c?static_cast<void*>(&shaderCounterPointer):static_cast<void*>(&unused);const uint32_t address=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target));std::memcpy(shaderCode[i]+fixup[0],&address,4);}
  for(unsigned c=0;c<body.callCount;++c){const auto& call=body.calls[c];const uintptr_t target=call[1]==0x1000df50?reinterpret_cast<uintptr_t>(&shaderGetter):call[1]==0x10002da0?reinterpret_cast<uintptr_t>(&refill):reinterpret_cast<uintptr_t>(&shaderForbiddenExit);const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(shaderCode[i]+call[0]+5));std::memcpy(shaderCode[i]+call[0]+1,&delta,4);}
  // Initialization flag is fixed true; trap entry to its external-services path.
  shaderCode[i][31]=0x0f;shaderCode[i][32]=0x0b;
  DWORD previous;if(!VirtualProtect(shaderCode[i],4096,PAGE_EXECUTE_READ,&previous))return false;FlushInstructionCache(GetCurrentProcess(),shaderCode[i],4096);
 }

 for(unsigned i=0;i<2;++i){const auto& body=managerUnbindPrefixBodies[i];std::array<unsigned char,38> bytes{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(bytes.data()),body.size);if(!dll||std::memcmp(bytes.data(),body.bytes,body.size))return false;shaderPrefixCode[i]=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!shaderPrefixCode[i])return false;std::memcpy(shaderPrefixCode[i],bytes.data(),body.size);shaderPrefixCode[i][body.size]=0x0f;shaderPrefixCode[i][body.size+1]=0x0b;const uint32_t delta=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(shaderCode[i])-reinterpret_cast<uintptr_t>(shaderPrefixCode[i]+body.callOffset+5));std::memcpy(shaderPrefixCode[i]+body.callOffset+1,&delta,4);DWORD previous;if(!VirtualProtect(shaderPrefixCode[i],4096,PAGE_EXECUTE_READ,&previous))return false;FlushInstructionCache(GetCurrentProcess(),shaderPrefixCode[i],4096);}
 std::array<unsigned char,sizeof(managerRestoreResetBytes)> managerBytes{};dll.seekg(managerRestoreResetFileOffset);dll.read(reinterpret_cast<char*>(managerBytes.data()),managerBytes.size());if(!dll||std::memcmp(managerBytes.data(),managerRestoreResetBytes,managerBytes.size()))return false;
 shaderManagerCode=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!shaderManagerCode)return false;std::memcpy(shaderManagerCode,managerBytes.data(),managerBytes.size());DWORD previous;if(!VirtualProtect(shaderManagerCode,4096,PAGE_EXECUTE_READ,&previous))return false;FlushInstructionCache(GetCurrentProcess(),shaderManagerCode,4096);return true;
}

static HRESULT __stdcall pixel(void*,uint32_t start,const uint32_t* data,uint32_t count){if(start)std::abort();for(unsigned i=0;i<count*4;++i)if(data[i])std::abort();events.insert(events.end(),{3,7,count});return E_FAIL;}
static HRESULT __stdcall vertex(void*,uint32_t,const uint32_t*,uint32_t count){events.insert(events.end(),{3,10,count});return E_FAIL;}
#endif
struct Allocation{void free(void* p){release(p);}uint32_t count(unsigned id){return counts[id];}void* allocate(uint32_t n){return ::allocate(n);}};
struct Capability{uint32_t count(unsigned id){return counts[id];}};
struct ScalarBackend{
 StudioConstantStorageOwnerCandidate& owner;StudioDeviceQueueStateCandidate& queue;
 unsigned id(unsigned slot){return slot==43?1:slot==44?2:slot==41?4:5;}
 uint32_t* primary(unsigned slot){return static_cast<uint32_t*>(owner.primary[id(slot)]);}uint32_t* secondary(unsigned slot){return static_cast<uint32_t*>(owner.staging[id(slot)]);}
 bool queueEnabled(){return queued;}auto& queueState(){return queue;}
 void refill(StudioDeviceQueueStateCandidate&){std::abort();}
 void direct(unsigned slot,uint32_t start,const uint32_t* source,uint32_t count){scalarDirect(slot==98?11:slot==96?12:slot==113?8:9,start,source,count);}
};
struct Initialize{
 StudioConstantStorageOwnerCandidate& owner;uint32_t*& cursor;uint32_t& remaining;Capability caps;
 Capability* capability(){return &caps;}uint32_t* staging(unsigned id){return static_cast<uint32_t*>(owner.staging[id]);}uint32_t* cache(unsigned id){return static_cast<uint32_t*>(owner.primary[id]);}
 void copy(uint32_t* d,uint32_t* s,uint32_t n){std::memmove(d,s,n);}
 void submitFloat(bool vertex,uint32_t start,uint32_t count,uint32_t* data){if(queued){const unsigned words=count*4+3;if(remaining<words)std::abort();remaining-=words;cursor[0]=vertex?10:7;cursor[1]=start;cursor[2]=count;std::memmove(cursor+3,data,count*16);cursor+=words;}else events.insert(events.end(),{3,vertex?10u:7u,count});}
 void submitSubobject(unsigned slot,uint32_t start,uint32_t* data,uint32_t count,uint32_t force){StudioDeviceQueueStateCandidate queue{nullptr,cursor,remaining};ScalarBackend backend{owner,queue};studioScalarConstantSubmitCandidate(studioScalarConstantSubmissionsCandidate[slot-41],start,data,count,force,backend);cursor=queue.cursor;remaining=queue.remaining;}
};
struct Tail{
 Initialize& initialize;
 uint32_t* primaryCache(){return initialize.cache(3);}uint32_t* secondaryCache(){return initialize.staging(3);}
 void submitVertex(unsigned index,const std::array<uint32_t,4>& words){initialize.submitFloat(true,index,1,const_cast<uint32_t*>(words.data()));}
 void restoreVertexDefaults(uint32_t bits){studioConstantRestoreDefaultsCandidate(bits,*this);}
 struct Device{HRESULT SetVertexShader(Shader* p){return shaderObserve(true,p);}HRESULT SetPixelShader(Shader* p){return shaderObserve(false,p);}};
 struct Lifecycle{
  Initialize& initialize;Device object;
  void recordBind(bool){++shaderCounter;}Device* commandState(){++shaderCaptures;return &object;}
  bool queueEnabled(Device*){return queued;}Device* device(Device* p){return p;}
  void queueShader(Device*,bool vertex,Shader* p){if(p||initialize.remaining<2)std::abort();initialize.remaining-=2;*initialize.cursor++=vertex?6:5;*initialize.cursor++=0;}
 };
 void unbindShaders(){NativeShaderBindingStateCandidateT<Shader> vertex{},pixel{};Lifecycle lifecycle{initialize};nativeShaderRestoreUnbindCandidate(vertex,pixel,lifecycle);if(!vertex.valid||!pixel.valid||vertex.shader||pixel.shader)std::abort();}
};
int main(){
 const unsigned previousMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
#ifdef _M_IX86
 std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);std::array<unsigned char,sizeof(nativeConstantStorageRestoreBytes)> bytes{};dll.seekg(nativeConstantStorageRestoreOffset);dll.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!dll||std::memcmp(bytes.data(),nativeConstantStorageRestoreBytes,bytes.size()))return 1;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,bytes.data(),bytes.size());std::array<uint32_t,336> capability{};uint32_t* capabilityPointer=capability.data();uint32_t constant=0x40000000;if(!prepareShaderChain(dll))return 12;void* managerTable[12]{};managerTable[9]=shaderPrefixCode[0];managerTable[10]=shaderPrefixCode[1];managerTable[11]=shaderManagerCode;std::array<uint32_t,41> managerObject{};managerObject[0]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(managerTable));uint32_t* managerPointer=managerObject.data();
 for(auto& f:nativeConstantStorageRestoreFixups){const uintptr_t target=f[1]==0x1016d130?reinterpret_cast<uintptr_t>(&capabilityPointer):f[1]==0x100fa5fc?reinterpret_cast<uintptr_t>(&constant):f[1]==0x1016e20c?reinterpret_cast<uintptr_t>(&managerPointer):0;if(!target)return 3;const uint32_t address=static_cast<uint32_t>(target);std::memcpy(code+f[0],&address,4);}
 for(auto& c:nativeConstantStorageRestoreCalls){const uintptr_t target=c[1]==0x10003990?reinterpret_cast<uintptr_t>(&allocate):c[1]==0x100039b0?reinterpret_cast<uintptr_t>(&release):c[1]==0x100df030?reinterpret_cast<uintptr_t>(&copy):c[1]==0x10002da0?reinterpret_cast<uintptr_t>(&refill):0;if(!target)return 4;const uint32_t delta=static_cast<uint32_t>(target-reinterpret_cast<uintptr_t>(code+c[0]+5));std::memcpy(code+c[0]+1,&delta,4);}DWORD old;if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return 5;FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*)>(code);std::ofstream oracle("outputs/native-constant-storage-restore.bin",std::ios::binary|std::ios::trunc);void* subTable[155]{};if(!prepareScalarBodies(dll))return 10;subTable[41]=scalarCode[0];subTable[42]=scalarCode[1];subTable[43]=scalarCode[2];subTable[44]=scalarCode[3];if(!prepareDefaultsBody(dll))return 11;subTable[154]=defaultsCode;void* deviceTable[114]{};deviceTable[92]=reinterpret_cast<void*>(&shaderVertex);deviceTable[107]=reinterpret_cast<void*>(&shaderPixel);deviceTable[98]=reinterpret_cast<void*>(&vertexB);deviceTable[96]=reinterpret_cast<void*>(&vertexI);deviceTable[113]=reinterpret_cast<void*>(&pixelB);deviceTable[111]=reinterpret_cast<void*>(&pixelI);deviceTable[94]=reinterpret_cast<void*>(&vertex);deviceTable[109]=reinterpret_cast<void*>(&pixel);void** deviceObject=deviceTable;
#else
 std::ifstream oracle("outputs/native-constant-storage-restore.bin",std::ios::binary);
#endif
 unsigned cases=0;for(bool queueMode:{false,true})for(bool present:{false,true})for(bool zeroScalars:{false,true}){queued=queueMode;counts[0]=4;counts[3]=256;for(unsigned id:{1u,2u,4u,5u})counts[id]=zeroScalars?0:2;StudioConstantStorageOwnerCandidate owner;storage=&owner;events.clear();allocations=frees=0;shaderCounter=shaderCaptures=0;for(unsigned id=0;id<6;++id)if(present){owner.primary[id]=std::malloc(16);owner.staging[id]=std::malloc(16);if(!owner.primary[id]||!owner.staging[id])return 6;}std::array<uint32_t,4096> block{};uint32_t* cursor=block.data()+1;uint32_t remaining=4095;
#ifdef _M_IX86
  std::array<uint32_t,3000> original{};originalOwner=original.data();for(unsigned id=0;id<6;++id){original[primaryOffsets[id]/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(owner.primary[id]));original[stagingOffsets[id]/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(owner.staging[id]));capability[(0x528+id*4)/4]=counts[id];}original[0x2d0/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(subTable));original[0x648/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&deviceObject));original[0x658/4]=queued?1:0;original[0x660/4]=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(cursor));original[0x664/4]=remaining;managerObject[0x9c/4]=managerObject[0xa0/4]=0;native(original.data());if(managerObject[0x9c/4]||managerObject[0xa0/4])return 13;syncStorage();cursor=reinterpret_cast<uint32_t*>(original[0x660/4]);remaining=original[0x664/4];
#else
  Allocation allocation;Initialize initialize{owner,cursor,remaining};Tail tail{initialize};if(!studioConstantStorageRestoreCheckedCandidate(owner,allocation,initialize,tail))return 15;
#endif
  if(shaderCounter!=2||shaderCaptures!=2)return 14;std::vector<uint32_t> record{shaderCounter,shaderCaptures,allocations,frees,remaining,static_cast<uint32_t>(cursor-block.data()),static_cast<uint32_t>(events.size())};record.insert(record.end(),events.begin(),events.end());for(unsigned id=0;id<6;++id){const unsigned words=counts[id]*(id==1||id==4?1:4);const auto* primary=static_cast<uint32_t*>(owner.primary[id]);const auto* stage=static_cast<uint32_t*>(owner.staging[id]);record.insert(record.end(),primary,primary+words);record.insert(record.end(),stage,stage+words);}record.insert(record.end(),block.begin(),block.end());
#ifdef _M_IX86
  oracle.write(reinterpret_cast<const char*>(record.data()),record.size()*4);
#else
  std::vector<uint32_t> expected(record.size());oracle.read(reinterpret_cast<char*>(expected.data()),expected.size()*4);if(record!=expected){std::printf("mismatch queued=%u old=%u zero=%u\n",queueMode,present,zeroScalars);return 7;}
#endif
  if(!oracle)return 8;for(unsigned id=0;id<6;++id){std::free(owner.primary[id]);std::free(owner.staging[id]);}++cases;
 }
#ifdef _M_IX86
 VirtualFree(code,0,MEM_RELEASE);for(auto* body:scalarCode)VirtualFree(body,0,MEM_RELEASE);VirtualFree(defaultsCode,0,MEM_RELEASE);for(auto* body:shaderCode)VirtualFree(body,0,MEM_RELEASE);for(auto* body:shaderPrefixCode)VirtualFree(body,0,MEM_RELEASE);VirtualFree(shaderManagerCode,0,MEM_RELEASE);
#else
 if(oracle.peek()!=std::char_traits<char>::eof())return 9;
#endif
 _mm_setcsr(previousMxcsr);std::printf("constant_storage_restore_native_cases=%u passed (original scalars, defaults and shader-unbind chain; external services controlled)\n",cases);
}


