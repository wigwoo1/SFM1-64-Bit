#define NOMINMAX
#include <Windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <cstdio>
#include "shader_transition_native_init.h"
#include "slot267_native_init.h"
#include "aux_pixel_native_init.h"
#include "pixel_color_native_init.h"
#include "color_conversion_native_init.h"
#include "color_lookup_native_bytes.h"
#include "render_state_native_init.h"
#include "pixel_queue_refill_native_init.h"
static std::array<uint32_t,4097> replacementBlock{};static uint32_t* expectedPublished=nullptr;
static unsigned publishes=0,acquires=0,ringObject=0;
static unsigned char* queueOwner=nullptr;static unsigned publishedWords=0;
static void __fastcall publishBlock(void* ring,void*,uint32_t* block){uint32_t* end=nullptr;std::memcpy(&end,queueOwner+0x660,4);if(ring!=&ringObject||block!=expectedPublished||block[0]!=2||*end!=0)ExitProcess(27);publishedWords=static_cast<unsigned>(end-block-3);++publishes;}
static uint32_t* __fastcall acquireBlock(void*,void*,unsigned header){if(header!=1||publishes!=1)ExitProcess(28);++acquires;replacementBlock[0]=1;return replacementBlock.data();}
static void* nativeRender=nullptr;
static unsigned __fastcall suppression(void*,void*){return 0;}
static void* nativeConvert=nullptr;
static void* nativePixel=nullptr;static unsigned colorUploads=0;
static float __fastcall exposure(void*,void*,unsigned){return 2;}
static void* nativeAux=nullptr;static unsigned auxUploads=0;
static uint32_t __fastcall auxMode(void*,void*){return 1;}
static HRESULT __stdcall pixelUpload(void*,unsigned index,const float*,unsigned count){if(count!=1)ExitProcess(26);if(index==17)++auxUploads;else if(index==29)++colorUploads;else ExitProcess(26);return S_OK;}
static void* nativeSlot267=nullptr;static unsigned byteRenders=0;
static uint32_t cookie=0x12345678,inputMode=0,byteValue=99;
static int level=90;static unsigned entries=0,caps=0,vertices=0,bytes=0,aux=0,pixels=0,renders=0,conversions=0;
static unsigned mutation=0;static unsigned char* liveOwner=nullptr;
static uint32_t __fastcall entryCallback(void*,void*){++entries;return 0;}
static int __fastcall capability(void*,void*){++caps;
 if(mutation){if(caps==1){uint32_t state=0;std::memcpy(&state,liveOwner+0x3110,4);state=state?0:1;std::memcpy(liveOwner+0x3110,&state,4);}
 else{float hi=100,density=.25f;std::memcpy(liveOwner+0x2f50,&hi,4);std::memcpy(liveOwner+0x2f54,&density,4);}}
 return level;}
static void __fastcall transitionByte(void* owner,void*,unsigned value){++bytes;byteValue=static_cast<uint8_t>(value);reinterpret_cast<void(__thiscall*)(void*,unsigned)>(nativeSlot267)(owner,value);}
static void __fastcall auxiliary(void* owner,void*,unsigned index,unsigned enable){if(index!=17||enable!=inputMode)ExitProcess(20);++aux;reinterpret_cast<void(__thiscall*)(void*,unsigned,unsigned)>(nativeAux)(owner,index,enable);}
static void __fastcall pixel(void* owner,void*){++pixels;reinterpret_cast<void(__thiscall*)(void*)>(nativePixel)(owner);}
static uint32_t __fastcall convert(void* owner,void*,unsigned r,unsigned g,unsigned b,unsigned flag){if(flag!=0x101)ExitProcess(21);++conversions;return reinterpret_cast<uint32_t(__thiscall*)(void*,unsigned,unsigned,unsigned,unsigned)>(nativeConvert)(owner,r,g,b,flag);}
static void __fastcall render(void* owner,void*,unsigned state,unsigned value,unsigned force){if(force)ExitProcess(22);reinterpret_cast<void(__thiscall*)(void*,unsigned,unsigned,unsigned)>(nativeRender)(owner,state,value,force);}
static HRESULT __stdcall renderDevice(void*,unsigned state,unsigned){if(state==28)++byteRenders;else if(state==34)++renders;else ExitProcess(22);return S_OK;}
static void __fastcall cookieCheck(uint32_t value){if(value!=cookie)ExitProcess(23);}
static void forbidden(){ExitProcess(24);}
static HRESULT __stdcall vertex(void*,unsigned index,const float*,unsigned count){if(index!=(vertices?2u:16u)||count!=1)ExitProcess(25);++vertices;return S_OK;}
int main(){
 std::ifstream dll("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\bin\\shaderapidx9.dll",std::ios::binary);
 std::array<unsigned char,1614> original{};dll.seekg(nativeShaderTransitionFileOffset);dll.read(reinterpret_cast<char*>(original.data()),original.size());if(!dll||std::memcmp(original.data(),nativeShaderTransitionBytes,original.size()))return 1;
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 2;std::memcpy(code,original.data(),original.size());
 unsigned char refillBytes[76];dll.seekg(nativePixelQueueRefillFileOffset);dll.read(reinterpret_cast<char*>(refillBytes),76);if(!dll||std::memcmp(refillBytes,nativePixelQueueRefillBytes,76))return 22;
 std::memcpy(code+7168,refillBytes,76);uintptr_t ringAddress=reinterpret_cast<uintptr_t>(&ringObject);std::memcpy(code+7168+40,&ringAddress,4);
 auto refillPatch=[&](unsigned offset,uintptr_t target){int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+7168+offset+5));std::memcpy(code+7168+offset+1,&delta,4);};
 refillPatch(44,reinterpret_cast<uintptr_t>(&publishBlock));refillPatch(53,reinterpret_cast<uintptr_t>(&acquireBlock));
 unsigned char slotBytes[49];dll.seekg(nativeSlot267FileOffset);dll.read(reinterpret_cast<char*>(slotBytes),49);if(!dll||std::memcmp(slotBytes,nativeSlot267Bytes,49))return 10;
 std::memcpy(code+2048,slotBytes,49);nativeSlot267=code+2048;
 unsigned char renderBytes[204];dll.seekg(nativeRenderStateFileOffset);dll.read(reinterpret_cast<char*>(renderBytes),204);if(!dll||std::memcmp(renderBytes,nativeRenderStateBytes,204))return 16;
 std::memcpy(code+6656,renderBytes,204);nativeRender=code+6656;int32_t refillDelta=7168-(6656+103);std::memcpy(code+6656+99,&refillDelta,4);
 int32_t slotDelta=static_cast<int32_t>(reinterpret_cast<uintptr_t>(&render)-reinterpret_cast<uintptr_t>(code+2048+32+5));std::memcpy(code+2048+33,&slotDelta,4);
 std::array<uintptr_t,35> globalTable{};globalTable[34]=reinterpret_cast<uintptr_t>(&entryCallback);
 std::array<unsigned char,0x5b0> globalObject{};uintptr_t globalVtable=reinterpret_cast<uintptr_t>(globalTable.data());std::memcpy(globalObject.data(),&globalVtable,4);globalObject[0x5aa]=1;uintptr_t globalPointer=reinterpret_cast<uintptr_t>(globalObject.data());
 std::array<uintptr_t,18> capTable{};capTable[17]=reinterpret_cast<uintptr_t>(&capability);uintptr_t capObject=reinterpret_cast<uintptr_t>(capTable.data()),capPointer=reinterpret_cast<uintptr_t>(&capObject);
 std::array<uintptr_t,268> secondaryTable{};secondaryTable[267]=reinterpret_cast<uintptr_t>(&transitionByte);secondaryTable[2]=reinterpret_cast<uintptr_t>(&auxMode);
 secondaryTable[28]=reinterpret_cast<uintptr_t>(&exposure);
 std::array<uintptr_t,110> deviceTable{};deviceTable[94]=reinterpret_cast<uintptr_t>(&vertex);deviceTable[109]=reinterpret_cast<uintptr_t>(&pixelUpload);uintptr_t deviceObject=reinterpret_cast<uintptr_t>(deviceTable.data());
 deviceTable[57]=reinterpret_cast<uintptr_t>(&renderDevice);std::array<uintptr_t,37> primaryTable{};primaryTable[36]=reinterpret_cast<uintptr_t>(&suppression);
 std::array<uintptr_t,6> jump{};for(unsigned i=0;i<6;++i)jump[i]=reinterpret_cast<uintptr_t>(code+nativeShaderTransitionJumpOffsets[i]);
 float one=1,normalize=1.f/255.f;uint32_t sentinel=0xff7fffff;
 std::array<float,256> table{};std::ifstream tableFile("outputs/native-full-generator-color-table.bin",std::ios::binary);tableFile.read(reinterpret_cast<char*>(table.data()),1024);if(!tableFile)return 13;
 unsigned char conversionBytes[384],lookupBytes[71];dll.seekg(nativeColorConversionFileOffset);dll.read(reinterpret_cast<char*>(conversionBytes),384);if(!dll||std::memcmp(conversionBytes,nativeColorConversionBytes,384))return 14;
 dll.seekg(colorLookupFileOffset);dll.read(reinterpret_cast<char*>(lookupBytes),71);if(!dll||std::memcmp(lookupBytes,colorLookupBytes,71))return 15;
 std::memcpy(code+5120,conversionBytes,384);std::memcpy(code+6144,lookupBytes,71);nativeConvert=code+5120;
 float scale=255,threshold=.95f;
 auto patch=[&](unsigned offset,const void* pointer){uintptr_t target=reinterpret_cast<uintptr_t>(pointer);std::memcpy(code+offset,&target,4);};
 for(auto& f:nativeColorConversionFixups){const void* pointer=f[1]==0x1016d130?static_cast<void*>(&globalPointer):f[1]==0x100fc764?static_cast<void*>(&normalize):f[1]==0x100fa5f8?static_cast<void*>(&one):static_cast<void*>(&scale);patch(5120+f[0],pointer);}
 for(auto offset:nativeColorConversionCalls){int32_t delta=6144-(5120+offset+5);std::memcpy(code+5120+offset+1,&delta,4);}
 patch(6144+0x1a,&threshold);patch(6144+0x2a,&scale);patch(6144+0x3f,table.data());
 double exponent=2.200000047683716;unsigned char pixelBytes[712];dll.seekg(nativePixelColorFileOffset);dll.read(reinterpret_cast<char*>(pixelBytes),712);if(!dll||std::memcmp(pixelBytes,nativePixelColorBytes,712))return 12;
 std::memcpy(code+4096,pixelBytes,712);nativePixel=code+4096;
 for(auto& f:nativePixelColorFixups){uintptr_t pointer=f[1]==0x1017c370?reinterpret_cast<uintptr_t>(&cookie):f[1]==0x1016d130?reinterpret_cast<uintptr_t>(&globalPointer):f[1]==0x100fa5f8?reinterpret_cast<uintptr_t>(&one):reinterpret_cast<uintptr_t>(&exponent);std::memcpy(code+4096+f[0],&pointer,4);}
 for(auto& c:nativePixelColorCalls){uintptr_t target=c[1]==0x10002da0?reinterpret_cast<uintptr_t>(code+7168):c[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&cookieCheck):c[1]==0x100df030?reinterpret_cast<uintptr_t>(&std::memcpy):reinterpret_cast<uintptr_t>(&forbidden);int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+4096+c[0]+5));std::memcpy(code+4096+c[0]+1,&delta,4);}
 unsigned char auxBytes[518];dll.seekg(nativeAuxPixelFileOffset);dll.read(reinterpret_cast<char*>(auxBytes),518);if(!dll||std::memcmp(auxBytes,nativeAuxPixelBytes,518))return 11;
 std::memcpy(code+2560,auxBytes,518);nativeAux=code+2560;
 for(auto& f:nativeAuxPixelFixups){uintptr_t pointer=f[1]==0x1017c370?reinterpret_cast<uintptr_t>(&cookie):f[1]==0x100fa5f8?reinterpret_cast<uintptr_t>(&one):reinterpret_cast<uintptr_t>(&sentinel);std::memcpy(code+2560+f[0],&pointer,4);}
 for(auto& c:nativeAuxPixelCalls){uintptr_t target=c[1]==0x10002da0?reinterpret_cast<uintptr_t>(code+7168):c[1]==0x100df5ad?reinterpret_cast<uintptr_t>(&cookieCheck):c[1]==0x100df030?reinterpret_cast<uintptr_t>(&std::memcpy):reinterpret_cast<uintptr_t>(&forbidden);int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+2560+c[0]+5));std::memcpy(code+2560+c[0]+1,&delta,4);}
 for(auto& f:nativeShaderTransitionFixups){uintptr_t target=0;switch(f[1]){
 case 0x1017c370:target=reinterpret_cast<uintptr_t>(&cookie);break;case 0x1016d130:target=reinterpret_cast<uintptr_t>(&globalPointer);break;
 case 0x1019092c:target=reinterpret_cast<uintptr_t>(&capPointer);break;case 0x100fa5f8:target=reinterpret_cast<uintptr_t>(&one);break;
 case 0x100fc7dc:target=reinterpret_cast<uintptr_t>(&sentinel);break;case 0x100fc764:target=reinterpret_cast<uintptr_t>(&normalize);break;case 0x1001bee0:target=reinterpret_cast<uintptr_t>(jump.data());break;default:return 3;}
 std::memcpy(code+f[0],&target,4);}
 for(auto& c:nativeShaderTransitionCalls){uintptr_t target=0;switch(c[1]){
 case 0x100df5ad:target=reinterpret_cast<uintptr_t>(&cookieCheck);break;case 0x10002da0:target=reinterpret_cast<uintptr_t>(code+7168);break;
 case 0x100144a0:target=reinterpret_cast<uintptr_t>(&auxiliary);break;case 0x100167b0:target=reinterpret_cast<uintptr_t>(&pixel);break;
 case 0x1000fc90:target=reinterpret_cast<uintptr_t>(&convert);break;case 0x1000e330:target=reinterpret_cast<uintptr_t>(&render);break;default:return 4;}
 int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+c[0]+5));std::memcpy(code+c[0]+1,&delta,4);}
 DWORD old;if(!VirtualProtect(code,8192,PAGE_EXECUTE_READ,&old))return 5;FlushInstructionCache(GetCurrentProcess(),code,8192);
 std::array<unsigned char,0x3700> owner{};std::array<uint32_t,80> primary{},secondary{};
 std::array<uint32_t,120> pixelCurrent{},pixelComparison{};
 liveOwner=owner.data();
 queueOwner=owner.data();
 auto word=[&](unsigned offset,uintptr_t value){std::memcpy(owner.data()+offset,&value,4);};
 auto scalar=[&](unsigned offset,float value){std::memcpy(owner.data()+offset,&value,4);};
 word(0x2d0,reinterpret_cast<uintptr_t>(secondaryTable.data()));word(0x648,reinterpret_cast<uintptr_t>(&deviceObject));word(0x19e0,reinterpret_cast<uintptr_t>(primary.data()));word(0x2858,reinterpret_cast<uintptr_t>(secondary.data()));word(0x3610,17);
 word(0x19ec,reinterpret_cast<uintptr_t>(pixelCurrent.data()));word(0x2864,reinterpret_cast<uintptr_t>(pixelComparison.data()));
 word(0,reinterpret_cast<uintptr_t>(primaryTable.data()));
 scalar(0x2f4c,2);scalar(0x2f50,6);scalar(0x2f54,.5f);scalar(0x10bc,1);scalar(0x10c0,2);scalar(0x10c4,3);scalar(0x18a0,4);
 owner[0x310c]=64;owner[0x310d]=96;owner[0x310e]=192;
 auto native=reinterpret_cast<void(__thiscall*)(void*,unsigned,unsigned,unsigned,unsigned)>(code);unsigned cases=0;
 std::ofstream oracle("outputs/native-shader-transition-grid.bin",std::ios::binary|std::ios::trunc);if(!oracle)return 8;
 for(inputMode=0;inputMode<7;++inputMode)for(unsigned state=0;state<3;++state)for(unsigned modern=0;modern<2;++modern)for(unsigned flag=0;flag<2;++flag)for(mutation=0;mutation<2;++mutation){
  word(0x3110,state);word(0x187c,0);primary.fill(0xcccccccc);secondary.fill(0);level=modern?91:90;
  word(0x1880,0);word(0x1884,0);word(0x1888,0);owner[0x1890]=0;
  owner[0x1891]=255;byteRenders=0;
  word(0x1d08,0xcccccccc);word(0x1d20,0xcccccccc);
  pixelCurrent.fill(0);pixelComparison.fill(0xcccccccc);auxUploads=colorUploads=0;
  scalar(0x2f50,6);scalar(0x2f54,.5f);
  entries=caps=vertices=bytes=aux=pixels=renders=conversions=0;byteValue=99;
  native(owner.data()+0x2d0,inputMode,flag?0x101:0x100,0x101,0);
  const bool early=inputMode==6||state==2,skip=early||(modern&&!flag);const unsigned selected=skip?2u:modern?1u:0u;
  const unsigned liveState=mutation&&!early?(state?0u:1u):state;
  const unsigned expectedEntries=1+conversions+unsigned(inputMode&&selected!=0&&inputMode!=2&&inputMode!=5);
  if(entries!=expectedEntries||caps!=(early?0u:skip?1u:2u)||vertices!=(skip?0u:2u)||bytes!=1||aux!=1||byteValue!=unsigned(inputMode&&selected==0&&liveState!=0)||pixels!=unsigned(inputMode&&selected!=0)||renders!=unsigned(inputMode&&selected==0))return 6;
  if(!skip&&(std::memcmp(primary.data()+64,secondary.data()+64,16)||std::memcmp(primary.data()+8,secondary.data()+8,16)))return 7;
  std::array<uint32_t,32> record{};std::memcpy(record.data(),primary.data()+64,16);std::memcpy(record.data()+4,primary.data()+8,16);std::memcpy(record.data()+8,owner.data()+0x1880,12);
  record[11]=owner[0x1890];std::memcpy(&record[12],owner.data()+0x187c,4);record[13]=byteValue;record[14]=pixels;record[15]=renders;record[16]=vertices;record[17]=conversions;
  record[18]=owner[0x1891];record[19]=byteRenders;
  std::memcpy(record.data()+20,pixelCurrent.data()+68,16);record[24]=auxUploads;
  std::memcpy(record.data()+25,pixelCurrent.data()+116,16);record[29]=colorUploads;
  std::memcpy(&record[30],owner.data()+0x1d08,4);std::memcpy(&record[31],owner.data()+0x1d20,4);
  oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));if(!oracle)return 9;
  ++cases;
 }
 std::ofstream queueOracle("outputs/native-shader-transition-queue.bin",std::ios::binary|std::ios::trunc);if(!queueOracle)return 17;
 std::ofstream queueGrid("outputs/native-shader-transition-queue-grid.bin",std::ios::binary|std::ios::trunc);if(!queueGrid)return 20;
 std::ofstream rolloverGrid("outputs/native-shader-transition-rollover-grid.bin",std::ios::binary|std::ios::trunc);if(!rolloverGrid)return 23;
 std::ofstream splitGrid("outputs/native-shader-transition-split-grid.bin",std::ios::binary|std::ios::trunc);if(!splitGrid)return 26;
 std::array<uint32_t,64> packet{};unsigned queueCases=0;
 for(inputMode=0;inputMode<7;++inputMode)for(unsigned state=0;state<3;++state)for(unsigned modern=0;modern<2;++modern)for(unsigned flag=0;flag<2;++flag)for(mutation=0;mutation<2;++mutation){
  level=modern?91:90;word(0x3110,state);word(0x187c,0);
  primary.fill(0xcccccccc);secondary.fill(0);pixelCurrent.fill(0);pixelComparison.fill(0xcccccccc);
  word(0x1880,0);word(0x1884,0);word(0x1888,0);owner[0x1890]=0;owner[0x1891]=255;
  word(0x1d08,0xcccccccc);word(0x1d20,0xcccccccc);scalar(0x2f50,6);scalar(0x2f54,.5f);
  entries=caps=vertices=bytes=aux=pixels=renders=conversions=byteRenders=auxUploads=colorUploads=0;
  packet.fill(0xcccccccc);word(0x658,1);word(0x660,reinterpret_cast<uintptr_t>(packet.data()));word(0x664,64);
  native(owner.data()+0x2d0,inputMode,flag?0x101:0x100,0x101,0);
  uintptr_t cursor=0;uint32_t remaining=0;std::memcpy(&cursor,owner.data()+0x660,4);std::memcpy(&remaining,owner.data()+0x664,4);
  const bool skip=inputMode==6||state==2||(modern&&!flag);const unsigned selected=skip?2u:modern?1u:0u;
  const unsigned used=(skip?0u:14u)+10u+(inputMode?(selected?7u:3u):0u);
  if(cursor!=reinterpret_cast<uintptr_t>(packet.data()+used)||remaining!=64-used||vertices||renders||byteRenders||auxUploads||colorUploads)return 18;
  queueGrid.write(reinterpret_cast<const char*>(packet.data()),32*4);if(!queueGrid)return 21;
  if(inputMode==4&&state==1&&flag==1&&mutation==0){queueOracle.write(reinterpret_cast<const char*>(packet.data()),32*4);if(!queueOracle)return 19;}
  const auto baseline=packet;
  for(unsigned capacity=0;capacity<32;++capacity){
  word(0x3110,state);word(0x187c,0);primary.fill(0xcccccccc);secondary.fill(0);pixelCurrent.fill(0);pixelComparison.fill(0xcccccccc);
  word(0x1880,0);word(0x1884,0);word(0x1888,0);owner[0x1890]=0;owner[0x1891]=255;
  word(0x1d08,0xcccccccc);word(0x1d20,0xcccccccc);scalar(0x2f50,6);scalar(0x2f54,.5f);
  entries=caps=vertices=bytes=aux=pixels=renders=conversions=byteRenders=auxUploads=colorUploads=publishes=acquires=0;
  packet.fill(0xcccccccc);packet[0]=1;expectedPublished=packet.data();replacementBlock.fill(0xcccccccc);
  publishedWords=0;word(0x65c,reinterpret_cast<uintptr_t>(packet.data()));word(0x660,reinterpret_cast<uintptr_t>(packet.data()+3));word(0x664,capacity);
  native(owner.data()+0x2d0,inputMode,flag?0x101:0x100,0x101,0);
  std::memcpy(&cursor,owner.data()+0x660,4);std::memcpy(&remaining,owner.data()+0x664,4);
  const bool refill=capacity<used;const unsigned prefix=refill?publishedWords:used,suffix=used-prefix;
  const auto expectedCursor=refill?replacementBlock.data()+1+suffix:packet.data()+3+used;
  if(publishes!=unsigned(refill)||acquires!=unsigned(refill)||cursor!=reinterpret_cast<uintptr_t>(expectedCursor)||remaining!=(refill?4095-suffix:capacity-used)||vertices||renders||byteRenders||auxUploads||colorUploads)return 24;
  if(std::memcmp(packet.data()+3,baseline.data(),prefix*4)||std::memcmp(replacementBlock.data()+1,baseline.data()+prefix,suffix*4))return 29;
  splitGrid.write(reinterpret_cast<const char*>(&prefix),4);if(!splitGrid)return 30;
  if(capacity==2){rolloverGrid.write(reinterpret_cast<const char*>(replacementBlock.data()+1),32*4);if(!rolloverGrid)return 25;}
  }
  ++queueCases;
 }
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_shader_transition_routing_cases=%u controlled_helpers=1 full_helper_equivalence=unverified native_composed_queue_cases=%u\n",cases,queueCases);
 std::printf("native_transition_capacity_boundary_cases=%u\n",queueCases*32);
}
