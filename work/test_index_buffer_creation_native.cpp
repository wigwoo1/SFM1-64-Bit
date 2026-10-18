#define NOMINMAX
#include <Windows.h>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "command_buffer_creation_native_init.h"
#include "profile_counter_native_init.h"
static void check(bool value){if(!value)std::abort();}
static bool mutateMetadata,rebindDevice,mutateRegistration;static std::array<void*,4> deviceChoices;static std::array<unsigned,2> createDevices;static unsigned cleanupDevice;static unsigned deviceId(void* device){for(unsigned i=0;i<4;++i)if(device==deviceChoices[i])return i;std::abort();}
static void(*registeredCallback)();static unsigned callbackExecutions;static unsigned lazyCalls,registrations;static unsigned* publishedCounter;
static unsigned flushes,creates,cleanups,logs,lazyFlag=1;
static std::array<unsigned,1664> profile;
static unsigned& counter=profile[0x10c8/4];
static unsigned threadCalls,compareCalls,fallback=99,threadValue=42;
static bool fullProfile;
static unsigned*(__thiscall* profileFactory)(void*,const char*,unsigned);
static unsigned __stdcall threadId(){++threadCalls;return threadValue;}
static int __cdecl compareName(const char* a,const char* b){++compareCalls;return std::strcmp(a,b);}
static std::array<char,64> nameStorage;static unsigned allocations;
static void* __cdecl allocateName(unsigned bytes){check(bytes==25&&++allocations==1);return nameStorage.data();}
static unsigned countValue,dynamicValue,managedValue,firstResult,secondResult,outputMask;
static std::array<unsigned,8>* receiver;
static void* expectedDevice;
static void* expectedCommand;
static void __fastcall flush(void* command,void*){check(command==expectedCommand);++flushes;if(rebindDevice){check(flushes<4);expectedDevice=deviceChoices[flushes];reinterpret_cast<void**>(command)[0]=expectedDevice;}}

static unsigned* __fastcall createCounter(void* service,void*,const char* name,unsigned type){check(service==profile.data()&&name!=nullptr&&type==2&&lazyFlag==1&&++lazyCalls==1);auto result=profileFactory(service,name,type);check(result==(threadValue==42&&!fullProfile?&counter:&fallback));return result;}
static int __cdecl registerCounter(void(*callback)()){check(callback==registeredCallback&&lazyFlag==1&&publishedCounter==(threadValue==42&&!fullProfile?&counter:&fallback)&&++registrations==1);unsigned before=counter;callback();++callbackExecutions;check(counter==before&&lazyFlag==1&&publishedCounter==(threadValue==42&&!fullProfile?&counter:&fallback));if(mutateRegistration)(*receiver)[1]=9;return 0;}
static void __cdecl logMessage(const char*){++logs;}
static HRESULT __stdcall create(void* device,unsigned bytes,unsigned usage,unsigned format,unsigned pool,void** output,void* shared){
 check(device==expectedDevice&&bytes==((countValue+1u)&~1u)*2u&&usage==(8u|(dynamicValue?0x200u:0u)|(managedValue?0x10u:0u))&&format==0x65&&pool==(dynamicValue?0u:1u)&&output==reinterpret_cast<void**>(receiver->data())&&shared==nullptr);
 check(*output==reinterpret_cast<void*>(uintptr_t{creates==0?0u:(outputMask&1u?0x1235u:0u)}));check(creates<2);createDevices[creates]=deviceId(device);++creates;if(outputMask&(1u<<(creates-1))) *output=reinterpret_cast<void*>(uintptr_t{0x1234u+creates});if(mutateMetadata){(*receiver)[1]=7;reinterpret_cast<unsigned char*>(receiver->data())[0xc]^=4u;}return static_cast<HRESULT>(creates==1?firstResult:secondResult);
}
static HRESULT __stdcall cleanup(void* device){check(device==expectedDevice);cleanupDevice=deviceId(device);++cleanups;return E_FAIL;}
int main(){
 static_assert(sizeof(void*)==4,"original x86 oracle");const auto& body=bufferCreationBodies[1];std::ifstream dll(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll)",std::ios::binary);
 std::array<unsigned char,321> raw{};dll.seekg(body.fileOffset);dll.read(reinterpret_cast<char*>(raw.data()),raw.size());check(bool(dll)&&!std::memcmp(raw.data(),body.bytes,raw.size()));
 auto code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code!=nullptr);std::memcpy(code,raw.data(),raw.size());unsigned char exitByte=0;dll.seekg(1019664);dll.read(reinterpret_cast<char*>(&exitByte),1);check(bool(dll)&&exitByte==0xc3);code[512]=exitByte;registeredCallback=reinterpret_cast<void(*)()>(code+512);
 auto logging=&logMessage;auto lazy=&createCounter;void* counterService=profile.data();const char label[]="controlled buffer oracle";
 for(unsigned i=0;i<body.fixupCount;++i){auto fix=body.fixups[i];uintptr_t replacement=0;switch(fix[1]){case 0x1017fe94:replacement=reinterpret_cast<uintptr_t>(&lazyFlag);break;case 0x1017fe90:replacement=reinterpret_cast<uintptr_t>(&publishedCounter);break;case 0x100fa244:replacement=reinterpret_cast<uintptr_t>(&logging);break;case 0x100fa224:replacement=reinterpret_cast<uintptr_t>(&counterService);break;case 0x100fa258:replacement=reinterpret_cast<uintptr_t>(&lazy);break;case 0x100f9b10:replacement=reinterpret_cast<uintptr_t>(registeredCallback);break;case 0x100fb554:case 0x100fb534:replacement=reinterpret_cast<uintptr_t>(label);break;default:std::abort();}unsigned value=static_cast<unsigned>(replacement);std::memcpy(code+fix[0],&value,4);}
 for(unsigned i=0;i<body.callCount;++i){auto call=body.calls[i];uintptr_t target=call[1]==0x10002e50?reinterpret_cast<uintptr_t>(&flush):reinterpret_cast<uintptr_t>(&registerCounter);unsigned delta=static_cast<unsigned>(target-reinterpret_cast<uintptr_t>(code+call[0]+5));std::memcpy(code+call[0]+1,&delta,4);}
 auto threadFunction=&threadId;const auto& factory=profileCounterBodies[0];std::ifstream tier0(R"(F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tier0.dll)",std::ios::binary);std::array<unsigned char,250> factoryRaw{};tier0.seekg(factory.fileOffset);tier0.read(reinterpret_cast<char*>(factoryRaw.data()),factory.size);check(bool(tier0)&&factory.size==factoryRaw.size()&&!std::memcmp(factoryRaw.data(),factory.bytes,factory.size));auto* factoryCode=code+1024;std::memcpy(factoryCode,factoryRaw.data(),factory.size);
 for(unsigned i=0;i<factory.fixupCount;++i){auto fix=factory.fixups[i];check(fix[1]==0x1002f040||fix[1]==0x1007a380);unsigned address=static_cast<unsigned>(fix[1]==0x1002f040?reinterpret_cast<uintptr_t>(&threadFunction):reinterpret_cast<uintptr_t>(&fallback));std::memcpy(factoryCode+fix[0],&address,4);}
 for(unsigned i=0;i<factory.callCount;++i){auto call=factory.calls[i];check(call[1]==0x1001b1b1||call[1]==0x1001e482);uintptr_t address=call[1]==0x1001b1b1?reinterpret_cast<uintptr_t>(&compareName):reinterpret_cast<uintptr_t>(&allocateName);unsigned delta=static_cast<unsigned>(address-reinterpret_cast<uintptr_t>(factoryCode+call[0]+5));std::memcpy(factoryCode+call[0]+1,&delta,4);}
 profileFactory=reinterpret_cast<unsigned*(__thiscall*)(void*,const char*,unsigned)>(factoryCode);
 DWORD old;check(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old)!=0&&FlushInstructionCache(GetCurrentProcess(),code,4096)!=0);
 auto callback=reinterpret_cast<void*(__thiscall*)(void*,void*,unsigned,unsigned,unsigned)>(code);
 std::array<void*,28> deviceTable{};deviceTable[27]=reinterpret_cast<void*>(&create);deviceTable[5]=reinterpret_cast<void*>(&cleanup);auto table=deviceTable.data();std::array<void**,4> deviceObjects;deviceObjects.fill(table);for(unsigned i=0;i<4;++i)deviceChoices[i]=&deviceObjects[i];expectedDevice=deviceChoices[0];std::array<void*,8> command{};command[0]=expectedDevice;expectedCommand=command.data();
 std::ofstream records("outputs/native-index-buffer-creation.bin",std::ios::binary|std::ios::trunc);unsigned cases=0;
 for(unsigned count:{0u,1u,3u,0xffffffffu})for(unsigned dynamic:{0u,1u})for(unsigned managed:{0u,1u})for(unsigned result:{0u,1u,0x80004005u,0x8876017cu,0x8007000eu})for(unsigned retryResult:{0u,1u,0x80004005u,0x8876017cu,0x8007000eu})for(unsigned writes=0;writes<4;++writes)for(bool mutation:{false,true})for(bool rebind:{false,true})for(unsigned initialized:{0u,1u})for(bool wrongThread:{false,true})for(unsigned profileCount:{1u,255u,0u})for(bool registrationMutation:{false,true}){mutateRegistration=registrationMutation;bool full=profileCount==255;fullProfile=full;threadValue=wrongThread?43u:42u;profile.fill(0xabababab);profile[0x19c8/4]=profileCount;profile[0x19cc/4]=42;reinterpret_cast<unsigned char*>(profile.data())[0x14c8]=2;profile[0x15c8/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(label));counter=7;auto expectedProfile=profile;threadCalls=compareCalls=allocations=0;nameStorage.fill('!');fallback=99;lazyFlag=initialized;lazyCalls=registrations=callbackExecutions=0;publishedCounter=initialized?&counter:nullptr;rebindDevice=rebind;expectedDevice=deviceChoices[0];command[0]=expectedDevice;createDevices.fill(0);cleanupDevice=0;mutateMetadata=mutation;secondResult=retryResult;outputMask=writes;
 countValue=count;dynamicValue=dynamic;managedValue=managed;firstResult=result;flushes=creates=cleanups=logs=0;counter=7;std::array<unsigned,8> object{};object.fill(0xabababab);receiver=&object;auto expected=object;expected[0]=outputMask&1u?0x1235u:0u;expected[1]=(count+1u)&~1u;expected[2]=0;reinterpret_cast<unsigned char*>(expected.data())[0xc]=static_cast<unsigned char>((0xabu&0xfau)|2u|(dynamic?4u:0u));expected[4]=0xffffffffu;expected[5]=expected[6]=0;
 bool retry=result==0x8876017cu||result==0x8007000eu;if(retry&&(outputMask&2u))expected[0]=0x1236u;if(mutateMetadata){expected[1]=7;if(!retry)reinterpret_cast<unsigned char*>(expected.data())[0xc]^=4u;}auto returned=callback(object.data(),command.data(),count,managed,dynamic);if(registrationMutation&&!initialized&&!(reinterpret_cast<unsigned char*>(expected.data())[0xc]&4u))expected[1]=9;
 check(returned==object.data()&&object==expected&&creates==(retry?2u:1u)&&flushes==(retry?3u:1u)&&cleanups==unsigned(retry)&&logs==unsigned(result!=0)&&(publishedCounter?*publishedCounter:counter)==((!initialized&&(wrongThread||full)&&!(reinterpret_cast<unsigned char*>(expected.data())[0xc]&4u))?99u:(!initialized&&profileCount==0&&!wrongThread&&!(reinterpret_cast<unsigned char*>(expected.data())[0xc]&4u)?0u:7u))+((reinterpret_cast<unsigned char*>(expected.data())[0xc]&4u)?0u:expected[1]*2u)&&lazyFlag==((reinterpret_cast<unsigned char*>(expected.data())[0xc]&4u)?initialized:1u)&&lazyCalls==unsigned(!initialized&&!(reinterpret_cast<unsigned char*>(expected.data())[0xc]&4u))&&registrations==lazyCalls);
 check(createDevices[0]==(rebindDevice?1u:0u)&&createDevices[1]==(rebindDevice&&retry?3u:0u)&&cleanupDevice==(rebindDevice&&retry?2u:0u)&&deviceId(command[0])==(rebindDevice?(retry?3u:1u):0u));check(callbackExecutions==registrations);bool inserted=lazyCalls&&profileCount==0&&!wrongThread;check(allocations==unsigned(inserted));if(inserted){check(!std::strcmp(nameStorage.data(),label));expectedProfile[0x19c8/4]=1;expectedProfile[0x15c8/4]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(nameStorage.data()));}expectedProfile[0x10c8/4]=lazyCalls&&(wrongThread||full)?7u:counter;check(profile==expectedProfile&&threadCalls==(full?0u:lazyCalls)&&compareCalls==(wrongThread||full||profileCount==0?0u:lazyCalls)&&fallback==(lazyCalls&&(wrongThread||full)?99u+expected[1]*2u:99u));std::array<unsigned,21> record{};std::copy(object.begin(),object.end(),record.begin());record[8]=creates;record[9]=flushes;record[10]=cleanups;record[11]=logs;record[12]=publishedCounter?*publishedCounter:counter;record[13]=lazyFlag;record[14]=createDevices[0];record[15]=createDevices[1];record[16]=cleanupDevice;record[17]=deviceId(command[0]);record[18]=lazyCalls;record[19]=registrations;record[20]=callbackExecutions;records.write(reinterpret_cast<const char*>(record.data()),sizeof(record));check(bool(records));++cases;
 }
 check(VirtualFree(code,0,MEM_RELEASE)!=0);std::printf("original_index_buffer_creation_cases=%u passed\n",cases);
}

