#include <windows.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include "restore_resource_loader_init.h"
#include "restore_format_lookup_init.h"
static void check(bool b){if(!b)throw std::runtime_error("resource loader mismatch");}
static unsigned modeValue,modeCalls,diagnosticCalls;
static std::uint32_t* activeOwner;
static std::vector<unsigned> events;
static void* activeDevice;
static std::uint32_t* initialResources;
static std::uint32_t* replacementResources;
static bool replaceResources;
static unsigned paddedAmount,formatIndex,capturedSample,capturedQuality; static bool formatMutation;
static unsigned firstResult,firstOutput,secondCalls;
static unsigned __stdcall createSecond(void* object,unsigned width,unsigned height,unsigned format,unsigned sample,unsigned quality,unsigned discard,std::uint32_t* output,unsigned shared){
 check(object==activeDevice&&width==640&&height==480&&format==0x71&&discard==0&&shared==0&&output==initialResources+1);
 if(secondCalls==0){check(sample==activeOwner[0x40/4]&&quality==activeOwner[0x44/4]);events.push_back(281);*output=firstOutput;++secondCalls;return firstResult;}
 check(secondCalls==1&&sample==0&&quality==0);events.push_back(282);*output=0xfeed1234;++secondCalls;return 0x80004005u;
}
static unsigned __stdcall paddedAcquire(void* object,unsigned a,unsigned b,unsigned c,std::uint32_t* output){check(object==activeDevice&&a==0&&b==0&&c==0);check(output==initialResources+2);events.push_back(18);*output=0xdead4321;return 0x80004005u;}
static void __fastcall dimensions(void* object,void*,unsigned* width,unsigned* height){check(object==activeOwner);events.push_back(3);*width=640;*height=480;}
static unsigned __fastcall formatCallback(void*,void*){events.push_back(2);if(formatMutation){activeOwner[0x40/4]=99;activeOwner[0x44/4]=123;}return formatIndex;}
static unsigned __stdcall createPadded(void* object,unsigned width,unsigned height,unsigned format,unsigned sample,unsigned quality,unsigned discard,std::uint32_t* output,unsigned shared){check(object==activeDevice);check(width==640+2*paddedAmount&&height==480+2*paddedAmount);check(format==restoreFormatInstalledWindow[formatIndex]);check(sample==capturedSample&&quality==capturedQuality);check(discard==0&&shared==0&&output==initialResources);events.push_back(28);*output=0x99887766;return 0x80004005u;}
static void __fastcall flushCallback(void* object,void*){check(object==reinterpret_cast<unsigned char*>(activeOwner)+0x648);events.push_back(1);}
static unsigned __stdcall acquireCallback(void* object,unsigned index,std::uint32_t* output){check(object==activeDevice);check(index==0);check(output==initialResources);events.push_back(38);*output=0x13579bdf;if(replaceResources)activeOwner[0x668/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(replacementResources));return 0x80004005u;}
static unsigned __stdcall finalCallback(void* object,std::uint32_t* output){check(object==activeDevice);check(output==activeOwner+0x680/4);events.push_back(40);*output=0x2468ace0;return 0x80004005u;}
static unsigned __fastcall modeCallback(void*,void*){++modeCalls;return modeValue;}
static void __cdecl diagnosticCallback(const char* text){check(std::strcmp(text,"resource fixture")==0);++diagnosticCalls;}
int main(){try{
 static_assert(sizeof(void*)==4,"original x86 loader");
 std::ifstream in("outputs/restore-resource-creation-composed-original.bin",std::ios::binary);
 std::vector<unsigned char> blob((std::istreambuf_iterator<char>(in)),{});check(blob.size()==591);
 std::ifstream installed("F:/SteamLibrary/steamapps/common/SourceFilmmaker/game/bin/shaderapidx9.dll",std::ios::binary);check(bool(installed));
 for(auto b:resourceBodies){std::vector<char> raw(b.size);installed.seekg(b.file);installed.read(raw.data(),b.size);check(installed.gcount()==b.size);check(std::memcmp(raw.data(),blob.data()+b.offset,b.size)==0);}
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));check(memory!=nullptr);std::memcpy(memory,blob.data(),blob.size());memory[4096]=0xcc;
 // Bindings reside outside executable pages. Retry path remains unbound.
 for(auto f:resourceFixups){std::uint32_t old=0;std::memcpy(&old,memory+f.operand,4);check(old==f.original);auto target=reinterpret_cast<std::uintptr_t>(memory+4608+f.binding*256);std::memcpy(memory+f.operand,&target,4);}
 for(auto c:resourceCalls){auto target=c.target==4096?reinterpret_cast<std::uintptr_t>(&flushCallback):reinterpret_cast<std::uintptr_t>(memory+c.target);auto delta=static_cast<std::uint32_t>(target-reinterpret_cast<std::uintptr_t>(memory+c.next));std::memcpy(memory+c.operand,&delta,4);}
 for(auto f:resourceFixups){std::uint32_t target=0;std::memcpy(&target,memory+f.operand,4);check(target==reinterpret_cast<std::uintptr_t>(memory+4608+f.binding*256));check(f.binding<resourceBindingCount);check(resourceBindingNames[f.binding][0]!=0);}
 for(auto c:resourceCalls){std::uint32_t delta=0;std::memcpy(&delta,memory+c.operand,4);auto target=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(memory+c.next)+delta);check(target==(c.target==4096?reinterpret_cast<std::uintptr_t>(&flushCallback):reinterpret_cast<std::uintptr_t>(memory+c.target)));}
 auto put=[&](unsigned offset,std::uintptr_t value){std::memcpy(memory+offset,&value,4);};
 put(4608,reinterpret_cast<std::uintptr_t>(memory+4672));
 const char diagnosticText[]="resource fixture";std::memcpy(memory+4864,diagnosticText,sizeof(diagnosticText));
 put(5120,reinterpret_cast<std::uintptr_t>(&diagnosticCallback));
 std::array<std::uintptr_t,35> modeTable{};modeTable[34]=reinterpret_cast<std::uintptr_t>(&modeCallback);
 auto modeTablePointer=modeTable.data();put(5888,reinterpret_cast<std::uintptr_t>(&modeTablePointer));
 put(5376,reinterpret_cast<std::uintptr_t>(memory+5440));
 std::array<std::uintptr_t,41> deviceTable{};deviceTable[38]=reinterpret_cast<std::uintptr_t>(&acquireCallback);deviceTable[40]=reinterpret_cast<std::uintptr_t>(&finalCallback);
 auto deviceTablePointer=deviceTable.data();activeDevice=&deviceTablePointer;
 deviceTable[18]=reinterpret_cast<std::uintptr_t>(&paddedAcquire);deviceTable[28]=reinterpret_cast<std::uintptr_t>(&createPadded);
 std::array<std::uintptr_t,4> ownerTable{};ownerTable[3]=reinterpret_cast<std::uintptr_t>(&dimensions);
 std::array<std::uintptr_t,3> formatTable{};formatTable[2]=reinterpret_cast<std::uintptr_t>(&formatCallback);auto formatTablePointer=formatTable.data();put(5632,reinterpret_cast<std::uintptr_t>(&formatTablePointer));
 std::memcpy(memory+6144,restoreFormatInstalledWindow.data(),sizeof(restoreFormatInstalledWindow));
 DWORD old=0;check(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=0);check(FlushInstructionCache(GetCurrentProcess(),memory,4096)!=0);
 MEMORY_BASIC_INFORMATION info{};check(VirtualQuery(memory,&info,sizeof(info))==sizeof(info));check(info.Protect==PAGE_EXECUTE_READ);
 auto routine=reinterpret_cast<void(__thiscall*)(void*)>(memory);unsigned cases=0;
 for(unsigned pattern=0;pattern<256;++pattern)for(unsigned mode=0;mode<3;++mode)for(unsigned diagnostic=0;diagnostic<2;++diagnostic){
  std::array<std::uint32_t,432> owner{};owner.fill(pattern*0x01010101u);
  std::array<std::uint32_t,8> resources{};resources.fill(0x12340000u+pattern);
  owner[0x668/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(resources.data()));owner[0x680/4]=0x76540000u+pattern;
  auto expectedOwner=owner;auto expectedResources=resources;modeValue=mode;modeCalls=diagnosticCalls=0;put(4672+0x30,diagnostic);
  routine(owner.data());check(owner==expectedOwner);check(resources==expectedResources);check(modeCalls==1);check(diagnosticCalls==diagnostic);++cases;
 }
 unsigned creationCases=0;
 std::ofstream basicRecords("outputs/native-resource-basic.bin",std::ios::binary);check(bool(basicRecords));
 for(unsigned pattern=0;pattern<256;++pattern)for(unsigned mode=0;mode<3;++mode)for(unsigned negativePadding=0;negativePadding<2;++negativePadding)for(unsigned missing=0;missing<4;++missing)for(unsigned mutation=0;mutation<2;++mutation){
  std::array<std::uint32_t,432> owner{};owner.fill(pattern*0x01010101u);
  std::array<std::uint32_t,8> resources{};resources.fill(0x12340000u+pattern);
  std::array<std::uint32_t,8> replacement{};replacement.fill(0xabcd0000u+pattern);
  owner[0x648/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(activeDevice));owner[0x668/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(resources.data()));owner[0x680/4]=(missing&2)?0:0x76540000u+pattern;
  if(missing&1)resources[0]=0;
  auto expectedOwner=owner;auto expectedResources=resources;auto expectedReplacement=replacement;
  if(missing&1){expectedResources[0]=0x13579bdf;if(mutation){expectedOwner[0x668/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(replacement.data()));expectedReplacement[2]=0;}else expectedResources[2]=0;}
  if(missing&2)expectedOwner[0x680/4]=0x2468ace0;
  std::vector<unsigned> expectedEvents;if(missing&1){expectedEvents.push_back(1);expectedEvents.push_back(38);}if(missing&2){expectedEvents.push_back(1);expectedEvents.push_back(40);}
  activeOwner=owner.data();initialResources=resources.data();replacementResources=replacement.data();replaceResources=mutation!=0;events.clear();modeValue=mode;modeCalls=diagnosticCalls=0;put(4672+0x30,0);put(5440+0x30,negativePadding?0xffffffffu:0u);
  routine(owner.data());check(owner==expectedOwner);check(resources==expectedResources);check(replacement==expectedReplacement);check(events==expectedEvents);check(modeCalls==1);check(diagnosticCalls==0);++creationCases;
  std::array<std::uint32_t,36> record{};for(unsigned i=0;i<8;++i){record[i]=resources[i];record[8+i]=replacement[i];}record[16]=owner[0x680/4];record[17]=owner[0x40/4];record[18]=owner[0x44/4];record[19]=owner[0x668/4]==reinterpret_cast<std::uintptr_t>(replacement.data());record[20]=static_cast<unsigned>(events.size());for(unsigned i=0;i<events.size();++i)record[21+i]=events[i];record[34]=modeCalls;record[35]=diagnosticCalls;basicRecords.write(reinterpret_cast<const char*>(record.data()),sizeof(record));check(bool(basicRecords));
 }
 unsigned paddedCases=0;
 std::ofstream paddedRecords("outputs/native-resource-padded.bin",std::ios::binary);check(bool(paddedRecords));
 for(unsigned pattern=0;pattern<256;++pattern)for(unsigned padding:{1u,7u})for(unsigned index:{0u,12u,24u})for(unsigned finalMissing=0;finalMissing<2;++finalMissing)for(unsigned mutation=0;mutation<2;++mutation){
  std::array<std::uint32_t,432> owner{};owner.fill(pattern*0x01010101u);std::array<std::uint32_t,8> resources{};resources.fill(0xaabb0000u+pattern);resources[0]=0;
  owner[0]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(ownerTable.data()));owner[0x648/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(activeDevice));owner[0x668/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(resources.data()));owner[0x680/4]=finalMissing?0:0x55555555;
  auto expectedOwner=owner;auto expectedResources=resources;expectedResources[0]=0x99887766;expectedResources[2]=0xdead4321;if(finalMissing)expectedOwner[0x680/4]=0x2468ace0;
  activeOwner=owner.data();initialResources=resources.data();replaceResources=false;paddedAmount=padding;formatIndex=index;formatMutation=mutation!=0;capturedSample=owner[0x40/4];capturedQuality=owner[0x44/4];if(mutation){expectedOwner[0x40/4]=99;expectedOwner[0x44/4]=123;}modeValue=0;modeCalls=diagnosticCalls=0;events.clear();put(5440+0x30,padding);
  std::vector<unsigned> expectedEvents={1,18,3,2,1,28};if(finalMissing){expectedEvents.push_back(1);expectedEvents.push_back(40);}
  routine(owner.data());check(owner==expectedOwner&&resources==expectedResources);check(events==expectedEvents&&modeCalls==1&&diagnosticCalls==0);++paddedCases;
  std::array<std::uint32_t,28> record{};for(unsigned i=0;i<8;++i)record[i]=resources[i];record[8]=owner[0x680/4];record[9]=owner[0x40/4];record[10]=owner[0x44/4];record[11]=static_cast<unsigned>(events.size());for(unsigned i=0;i<events.size();++i)record[12+i]=events[i];record[24]=modeCalls;record[25]=diagnosticCalls;
  paddedRecords.write(reinterpret_cast<const char*>(record.data()),sizeof(record));check(bool(paddedRecords));
 }
 deviceTable[28]=reinterpret_cast<std::uintptr_t>(&createSecond);unsigned retryCases=0;
 std::ofstream records("outputs/native-resource-retry.bin",std::ios::binary);check(bool(records));
 for(unsigned pattern=0;pattern<256;++pattern)for(unsigned result:{0u,1u,0x80004005u})for(unsigned output:{0u,0x11223344u})for(unsigned finalMissing=0;finalMissing<2;++finalMissing){
  std::array<std::uint32_t,432> owner{};owner.fill(pattern*0x01010101u);std::array<std::uint32_t,8> resources{};resources.fill(0xaabb0000u+pattern);resources[1]=0;
  owner[0]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(ownerTable.data()));owner[0x648/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(activeDevice));owner[0x668/4]=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(resources.data()));owner[0x680/4]=finalMissing?0:0x55555555;
  bool retry=result!=0||output==0;auto expectedOwner=owner;auto expectedResources=resources;expectedResources[1]=retry?0xfeed1234:output;if(finalMissing)expectedOwner[0x680/4]=0x2468ace0;
  activeOwner=owner.data();initialResources=resources.data();modeValue=2;modeCalls=diagnosticCalls=0;firstResult=result;firstOutput=output;secondCalls=0;events.clear();
  std::vector<unsigned> expectedEvents={3,1,281};if(retry){expectedEvents.push_back(1);expectedEvents.push_back(282);}if(finalMissing){expectedEvents.push_back(1);expectedEvents.push_back(40);}
  routine(owner.data());check(owner==expectedOwner&&resources==expectedResources);check(events==expectedEvents&&modeCalls==1&&diagnosticCalls==0&&secondCalls==(retry?2u:1u));++retryCases;
  std::array<std::uint32_t,28> record{};for(unsigned i=0;i<8;++i)record[i]=resources[i];record[8]=owner[0x680/4];record[9]=owner[0x40/4];record[10]=owner[0x44/4];record[11]=static_cast<unsigned>(events.size());for(unsigned i=0;i<events.size();++i)record[12+i]=events[i];record[24]=modeCalls;record[25]=diagnosticCalls;record[26]=secondCalls;
  records.write(reinterpret_cast<const char*>(record.data()),sizeof(record));check(bool(records));
 }
 check(VirtualFree(memory,0,MEM_RELEASE)!=0);std::cout<<"resource_existing_cases="<<cases<<" basic_creation_cases="<<creationCases<<" padded_creation_cases="<<paddedCases<<" retry_cases="<<retryCases<<" passed original_execution\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

