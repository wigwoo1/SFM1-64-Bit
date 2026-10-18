#define NOMINMAX
#include <Windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <cstdio>
#include "presentation_builder_native_init.h"
#include "multisample_selector_native_init.h"
#include "studio_multisample_selector_candidate.h"
static unsigned step=0,sets=0,gets=0;static std::array<int,3> conditions{};static int width=0,height=0;static void* expectedOwner;static void* expectedDescriptor;static unsigned char display[24]{};
static void checkAt(bool value,int line){if(!value){std::printf("native_check_failed_line=%d\n",line);std::fflush(stdout);ExitProcess(30);}}
#define check(value) checkAt((value),__LINE__)
static void __cdecl trap(){ExitProcess(31);}
static unsigned expectedWindowed=1;
static bool forceNoVsync=false;static unsigned commandCalls=0,convarInitCalls=0,convarCacheFlags=0;static std::array<unsigned,13> convarObject{};static std::array<uintptr_t,2> convarReference{};
static void* commandObjectPointer=nullptr;
static void* __cdecl commandService(){return commandObjectPointer;}
static void checkPendingPresentation(){auto* words=static_cast<unsigned*>(expectedOwner);check(words[0x3c/4]==0&&words[0x64/4]==0&&words[0x38/4]==22);}
static int __fastcall commandQuery(void*,void*,const char* name){check(!std::strcmp(name,"-forcenovsync"));checkPendingPresentation();++commandCalls;return forceNoVsync?1:0;}
static void __fastcall convarInit(void* object,void*,const char* name){check(object==convarReference.data()&&!std::strcmp(name,"mat_triplebuffered"));checkPendingPresentation();++convarInitCalls;convarReference[1]=reinterpret_cast<uintptr_t>(convarObject.data());}
static unsigned depthChoice=75,depthCalls=0,formatCalls=0;
static unsigned quality=0,sampleCalls=0,sampleSeen=0;static HRESULT sampleResult=0;static std::array<unsigned,129> adapterInfo{};
static void* __fastcall adapterDetails(void*,void*,unsigned adapter){check(adapter==3);return adapterInfo.data();}
static HRESULT __stdcall sampleSupport(void*,unsigned adapter,unsigned type,unsigned format,BOOL windowed,unsigned sample,unsigned* output){check(adapter==3&&type==1&&format==22&&windowed==1);++sampleCalls;sampleSeen=sample;*output=quality;return sampleResult;}
static void* __cdecl clearBlock(void* target,int value,unsigned size){check(target==static_cast<unsigned char*>(expectedOwner)+0x30&&value==0&&size==56);return std::memset(target,value,size);}
static unsigned __cdecl colorChoice(unsigned adapter,unsigned type,unsigned prior,unsigned requested,unsigned windowed){check(adapter==3&&type==1&&prior==22&&requested==22&&windowed==expectedWindowed);return 22;}
static unsigned __cdecl depthFormat(unsigned adapter,unsigned prior,unsigned color,unsigned preferred){check(adapter==3&&prior==22&&color==22&&(preferred==75||preferred==77));++depthCalls;return depthChoice;}
static unsigned __cdecl translateFormat(unsigned value){check(value==22);++formatCalls;return 22;}
static void __fastcall displayQuery(void*,void*,void* output,unsigned adapter){check(++step==1&&adapter==3);check(*static_cast<unsigned*>(output)==1);std::memcpy(output,display,24);}
static void __fastcall dimensions(void* owner,void*,void* input,void* descriptor,int* w,int* h){check(++step==2&&owner==expectedOwner&&descriptor==expectedDescriptor);check(!std::memcmp(input,display,24));*w=width;*h=height;}
static uintptr_t materialTable[8]{};static uintptr_t materialObject;
static void* __cdecl getMaterial(){++gets;return &materialObject;}
static void __fastcall setCondition(void*,void*,const char* name,int value){const char* names[]={"WIN32WIDE","WIN32HIDEF","WIN32LODEF"};check(sets<3&&!std::strcmp(name,names[sets]));if(sets==0)check(value==static_cast<int>((static_cast<float>(width)/static_cast<float>(height))>=1.5f));else check(value==static_cast<int>(sets==1?(height>=720):(height<720)));conditions[sets++]=value;}
int main(int argc,char** argv){
 const bool captured=argc>1&&!std::strcmp(argv[1],"--captured");
 std::ifstream dll(argc>1?argv[1]:R"(D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);unsigned char installed[sizeof(nativePresentationBuilderBytes)]{};if(captured){std::memcpy(installed,nativePresentationBuilderBytes,sizeof(installed));}else{dll.seekg(nativePresentationBuilderOffset);dll.read(reinterpret_cast<char*>(installed),sizeof(installed));check(dll&& !std::memcmp(installed,nativePresentationBuilderBytes,sizeof(installed)));}
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code!=nullptr);std::memcpy(code,installed,sizeof(installed));
 unsigned char selectorInstalled[sizeof(selectorBytes)]{};if(captured){std::memcpy(selectorInstalled,selectorBytes,sizeof(selectorInstalled));}else{dll.seekg(selectorOffset);dll.read(reinterpret_cast<char*>(selectorInstalled),sizeof(selectorInstalled));check(dll&&!std::memcmp(selectorInstalled,selectorBytes,sizeof(selectorInstalled)));}auto* selectorCode=code+2048;std::memcpy(selectorCode,selectorInstalled,sizeof(selectorInstalled));uintptr_t selectorTable=reinterpret_cast<uintptr_t>(selectorCode+168);std::memcpy(selectorCode+21,&selectorTable,4);for(unsigned i=0;i<15;++i){uintptr_t target=reinterpret_cast<uintptr_t>(selectorCode+25+9*i);std::memcpy(selectorCode+168+4*i,&target,4);}
 uintptr_t managerTable[14]{};managerTable[13]=reinterpret_cast<uintptr_t>(&displayQuery);uintptr_t managerObject=reinterpret_cast<uintptr_t>(managerTable);void* manager=&managerObject;
 materialTable[7]=reinterpret_cast<uintptr_t>(&setCondition);materialObject=reinterpret_cast<uintptr_t>(materialTable);uintptr_t materialImport=reinterpret_cast<uintptr_t>(&getMaterial),trapImport=reinterpret_cast<uintptr_t>(&trap);float aspect=1.5f;unsigned char dummy[1024]{};
 uintptr_t commandTable[11]{};commandTable[10]=reinterpret_cast<uintptr_t>(&commandQuery);uintptr_t commandObject=reinterpret_cast<uintptr_t>(commandTable);commandObjectPointer=&commandObject;trapImport=reinterpret_cast<uintptr_t>(&commandService);
 uintptr_t apiTable[12]{};apiTable[11]=reinterpret_cast<uintptr_t>(&sampleSupport);uintptr_t apiObject=reinterpret_cast<uintptr_t>(apiTable);std::array<uintptr_t,13> apiHolder{};apiHolder[12]=reinterpret_cast<uintptr_t>(&apiObject);void* apiService=apiHolder.data();
 for(auto& fix:nativePresentationBuilderFixups){const void* target=dummy;switch(fix[1]){case 0x10183b94:target=&convarCacheFlags;break;case 0x10183b8c:target=convarReference.data();break;case 0x10183b90:target=&convarReference[1];break;case 0x100fdd58:target="-forcenovsync";break;case 0x100fdd44:target="mat_triplebuffered";break;case 0x1016dfb8:target=&apiService;break;case 0x10183b54:target=&manager;break;case 0x100fdd8c:target=&aspect;break;case 0x100fa274:target=&materialImport;break;case 0x100fa234:target=&trapImport;break;case 0x100fdd80:target="WIN32WIDE";break;case 0x100fdd74:target="WIN32HIDEF";break;case 0x100fdd68:target="WIN32LODEF";break;}auto address=reinterpret_cast<uintptr_t>(target);std::memcpy(code+fix[0],&address,4);}
 for(auto& call:nativePresentationBuilderCalls){void* callback=reinterpret_cast<void*>(&trap);switch(call[1]){case 0x100d6ba0:callback=reinterpret_cast<void*>(&convarInit);break;case 0x100297d0:callback=selectorCode;break;case 0x10028a60:callback=reinterpret_cast<void*>(&adapterDetails);break;case 0x100298c0:callback=reinterpret_cast<void*>(&dimensions);break;case 0x100df3a0:callback=reinterpret_cast<void*>(&clearBlock);break;case 0x10002990:callback=reinterpret_cast<void*>(&colorChoice);break;case 0x100026f0:callback=reinterpret_cast<void*>(&depthFormat);break;case 0x10036f30:callback=reinterpret_cast<void*>(&translateFormat);break;}auto target=reinterpret_cast<uintptr_t>(callback);int32_t delta=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)!=0);FlushInstructionCache(GetCurrentProcess(),code,4096);auto native=reinterpret_cast<void(__thiscall*)(void*,HWND,unsigned,void*,bool)>(code);
 std::ofstream oracle("outputs/native-presentation-first-pass.bin",std::ios::binary|std::ios::trunc);check(static_cast<bool>(oracle));std::printf("native_source_mode=%s current_installation_verified=%u\n",captured?"previously_verified_capture":"installed",captured?0u:1u);unsigned cases=0;for(int w:{640,1080,1920})for(int h:{480,719,720,1080})for(unsigned pattern:{0u,0x55u,0xffu}){
 std::array<unsigned char,720> owner{};owner.fill(static_cast<unsigned char>(pattern));auto before=owner;std::array<unsigned,14> descriptor{};expectedOwner=owner.data();expectedDescriptor=descriptor.data();width=w;height=h;step=sets=gets=0;
 native(owner.data(),reinterpret_cast<HWND>(0x1234),3,descriptor.data(),true);check(owner==before&&step==2&&sets==3&&gets==3);std::array<unsigned,6> record{step,sets,gets,static_cast<unsigned>(conditions[0]),static_cast<unsigned>(conditions[1]),static_cast<unsigned>(conditions[2])};oracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));check(static_cast<bool>(oracle));++cases;
 }
 std::ofstream secondOracle("outputs/native-presentation-windowed.bin",std::ios::binary|std::ios::trunc);check(static_cast<bool>(secondOracle));unsigned secondCases=0;
 for(unsigned flags:{1u,3u,11u,5u,65u,129u})for(unsigned depth:{73u,75u,79u,22u}){
 std::array<unsigned,180> owner{};owner.fill(0x55555555);owner[0x68/4]=22;std::array<unsigned,14> descriptor{};descriptor[2]=800;descriptor[3]=600;descriptor[4]=22;descriptor[13]=flags;descriptor[11]=640;descriptor[12]=480;
 unsigned mode[6]={1,1280,720,0,0,0};std::memcpy(display,mode,24);expectedOwner=owner.data();expectedDescriptor=descriptor.data();width=1280;height=720;step=sets=gets=depthCalls=formatCalls=0;depthChoice=depth;
 native(owner.data(),reinterpret_cast<HWND>(0x1234),3,descriptor.data(),false);check(step==2&&sets==3&&gets==3&&depthCalls==1&&formatCalls==1);
 auto* parameters=owner.data()+12;check(parameters[7]==0x1234&&parameters[8]==1&&parameters[9]==1&&parameters[10]==depth&&parameters[11]==0&&parameters[12]==0&&parameters[13]==0x80000000u&&parameters[4]==0&&parameters[5]==0);
 secondOracle.write(reinterpret_cast<const char*>(parameters),56);secondOracle.write(reinterpret_cast<const char*>(owner.data()+0xac/4),8);check(static_cast<bool>(secondOracle));++secondCases;
 }
 std::ofstream sampleOracle("outputs/native-presentation-multisample.bin",std::ios::binary|std::ios::trunc);check(static_cast<bool>(sampleOracle));unsigned sampleCases=0;std::ofstream combinedOracle("outputs/native-presentation-combined.bin",std::ios::binary|std::ios::trunc);check(static_cast<bool>(combinedOracle));
 for(unsigned requested:{1u,2u,4u,16u,17u})for(unsigned vendor:{0x10deu,0x8086u})for(unsigned result:{0u,1u,0x80000000u,0xffffffffu})for(unsigned q:{0u,5u,16u}){
 std::array<unsigned,180> owner{};owner[0x68/4]=22;std::array<unsigned,14> descriptor{};descriptor[2]=800;descriptor[3]=600;descriptor[4]=22;descriptor[8]=requested;descriptor[13]=1;
 expectedOwner=owner.data();expectedDescriptor=descriptor.data();step=sets=gets=depthCalls=formatCalls=0;depthChoice=75;adapterInfo[128]=vendor;sampleResult=static_cast<HRESULT>(result);quality=q;sampleCalls=sampleSeen=0;
 native(owner.data(),reinterpret_cast<HWND>(0x1234),3,descriptor.data(),false);const bool special=requested==16&&vendor==0x10de;const unsigned expectedType=special?4u:studioMultisampleSelectorCandidate(static_cast<int32_t>(requested));check(sampleCalls==1&&sampleSeen==expectedType);auto* parameters=owner.data()+12;check(parameters[4]==(sampleResult>=0?expectedType:0u)&&parameters[5]==(sampleResult>=0&&special&&q==16?15u:0u));
 std::array<unsigned,4> record{parameters[4],parameters[5],sampleCalls,sampleSeen};sampleOracle.write(reinterpret_cast<const char*>(record.data()),sizeof(record));check(static_cast<bool>(sampleOracle));combinedOracle.write(reinterpret_cast<const char*>(parameters),56);combinedOracle.write(reinterpret_cast<const char*>(owner.data()+0xac/4),8);check(static_cast<bool>(combinedOracle));++sampleCases;
 }
 std::ofstream fullscreenOracle("outputs/native-presentation-fullscreen.bin",std::ios::binary|std::ios::trunc);check(static_cast<bool>(fullscreenOracle));unsigned fullscreenCases=0;expectedWindowed=0;
 for(unsigned flags:{0u,4u})for(int requestedWidth:{0,800,-1})for(int denominator:{0,1001}){
 std::array<unsigned,180> owner{};owner[0x68/4]=22;std::array<unsigned,14> descriptor{};descriptor[2]=static_cast<unsigned>(requestedWidth);descriptor[3]=600;descriptor[4]=22;descriptor[5]=60000;descriptor[6]=static_cast<unsigned>(denominator);descriptor[13]=flags;
 expectedOwner=owner.data();expectedDescriptor=descriptor.data();step=sets=gets=depthCalls=formatCalls=0;depthChoice=75;
 native(owner.data(),reinterpret_cast<HWND>(0x1234),3,descriptor.data(),false);auto* parameters=owner.data()+12;check(parameters[0]==(requestedWidth?static_cast<unsigned>(requestedWidth):1280u)&&parameters[1]==(requestedWidth?600u:720u)&&parameters[8]==0&&parameters[12]==(denominator?59u:0u)&&parameters[13]==0x80000000u&&parameters[3]==1);
 fullscreenOracle.write(reinterpret_cast<const char*>(parameters),56);fullscreenOracle.write(reinterpret_cast<const char*>(owner.data()+0xac/4),8);check(static_cast<bool>(fullscreenOracle));++fullscreenCases;
 }
 std::printf("native_fullscreen_presentation_cases=%u gpu_executed=0\n",fullscreenCases);
 std::ofstream vsyncOracle("outputs/native-presentation-vsync.bin",std::ios::binary|std::ios::trunc);check(static_cast<bool>(vsyncOracle));unsigned vsyncCases=0;
 for(unsigned flags:{16u,20u})for(bool force:{false,true})for(int triple:{-1,0,1})for(unsigned cached:{0u,1u}){
 std::array<unsigned,180> owner{};owner[0x68/4]=22;std::array<unsigned,14> descriptor{};descriptor[2]=800;descriptor[3]=600;descriptor[4]=22;descriptor[5]=60000;descriptor[6]=1001;descriptor[13]=flags;
 expectedOwner=owner.data();expectedDescriptor=descriptor.data();step=sets=gets=depthCalls=formatCalls=0;depthChoice=75;forceNoVsync=force;commandCalls=convarInitCalls=0;convarCacheFlags=cached;convarReference[1]=cached?reinterpret_cast<uintptr_t>(convarObject.data()):0;convarObject[12]=static_cast<unsigned>(triple);
 native(owner.data(),reinterpret_cast<HWND>(0x1234),3,descriptor.data(),false);auto* parameters=owner.data()+12;check(parameters[13]==(force?0x80000000u:1u)&&parameters[3]==(force?1u:(triple?2u:1u))&&commandCalls==1&&convarInitCalls==(!force&&!cached?1u:0u));
 vsyncOracle.write(reinterpret_cast<const char*>(parameters),56);vsyncOracle.write(reinterpret_cast<const char*>(owner.data()+0xac/4),8);std::array<unsigned,3> extra{commandCalls,convarInitCalls,convarCacheFlags};vsyncOracle.write(reinterpret_cast<const char*>(extra.data()),sizeof(extra));check(static_cast<bool>(vsyncOracle));++vsyncCases;
 }
 std::printf("native_fullscreen_vsync_cases=%u gpu_executed=0\n",vsyncCases);
 VirtualFree(code,0,MEM_RELEASE);std::printf("native_presentation_first_pass_cases=%u windowed_second_pass_cases=%u multisample_cases=%u gpu_executed=0\n",cases,secondCases,sampleCases);
}










