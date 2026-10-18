#define NOMINMAX
#include <array>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include "studio_extended_vertex_constants_candidate.h"
#include "native_vertex_constant_runs_candidate.h"
#include "native_constant_storage_initialize_candidate.h"
#include "native_manager_callback_registry_candidate.h"
static void require(bool p){if(!p)throw std::runtime_error("extended restore composition mismatch");}
struct Device{
 std::vector<std::array<uint32_t,2>> runs;
 HRESULT SetVertexShaderConstantF(UINT start,const float*,UINT count){runs.push_back({start,count});return E_FAIL;}
 HRESULT SetPixelShaderConstantF(UINT,const float*,UINT){throw std::runtime_error("unexpected pixel submission");}
};
struct QueueLifecycle{void refill(StudioDeviceQueueStateCandidate&){throw std::runtime_error("unexpected refill");}};
using Submission=NativeConstantStorageInitializeServicesCandidateT<Device,QueueLifecycle>;
struct Command{Submission& submission;};struct State{float* values;};
struct BufferLifecycle{State& source;State& target;float* current(){return source.values;}float* cache(){return target.values;}};
static void restore(Command* command,State* source,State* target,bool force){BufferLifecycle lifecycle{*source,*target};NativeVertexConstantRunsServicesCandidateT<Submission,BufferLifecycle> services{command->submission,lifecycle};studioVertexConstantRunsCandidate(1024,512,force,services);}
using Callback=void(*)(Command*,State*,State*,bool);
using Registry=NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,Callback>;
struct SetterServices{
 State& source;Registry& registry;uint8_t flags=0;int32_t capacity=1536;unsigned warnings=0;
 int32_t vertexFloatCapacity(){return capacity;}float* staging(){return source.values;}uint8_t& registrationFlags(){return flags;}
 void copy(float* to,const float* from,uint32_t n){std::memmove(to,from,n);}void warnClipped(unsigned){++warnings;}void enqueueExtendedConstants(){registry.append(restore);}
};
static __declspec(noinline) bool setRange(int32_t start,int32_t count,const float* values,SetterServices& services){return studioExtendedVertexConstantsCandidate(start,count,values,services);}
int main(){try{unsigned cases=0;for(bool queuedMode:{false,true})for(bool force:{false,true}){
 std::array<float,6144> values{},cached{};State source{values.data()},target{cached.data()};NativeManagerCallbackAllocatorCandidate allocator;Registry registry{allocator};SetterServices setter{source,registry};float first[8]{1,2,3,4,5,6,7,8},second[4]{9,10,11,12};
 require(setRange(0,2,first,setter));require(setRange(4,1,second,setter));require(registry.size()==1&&setter.flags==2&&setter.warnings==0);
 Device object;Device* device=&object;uint32_t queued=queuedMode?1:0;std::array<uint32_t,4096> block{};StudioDeviceQueueStateCandidate queue{block.data(),block.data()+1,4095};QueueLifecycle lifecycle;Submission submission{device,queued,queue,lifecycle};Command command{submission};if(force){require(registry.dispatchRestoreWith(false,setter.flags,&command,&source,&target,true));require(registry.size()==0&&setter.flags==0);}else registry.dispatchWith(&command,&source,&target,false);
 require(values==cached);const std::vector<std::array<uint32_t,2>> expected=force?std::vector<std::array<uint32_t,2>>{{1024,512}}:std::vector<std::array<uint32_t,2>>{{1024,2},{1028,1}};
 if(!queuedMode)require(object.runs==expected);else{auto* cursor=block.data()+1;for(auto run:expected){require(cursor[0]==10&&cursor[1]==run[0]&&cursor[2]==run[1]);require(std::memcmp(cursor+3,values.data()+run[0]*4,run[1]*16)==0);cursor+=3+run[1]*4;}require(cursor==queue.cursor&&queue.remaining==4095-uint32_t(cursor-block.data()-1));}
 const auto oldCursor=queue.cursor;const auto oldRuns=object.runs.size();registry.dispatchWith(&command,&source,&target,false);require(queue.cursor==oldCursor&&object.runs.size()==oldRuns);
 if(force){first[0]=21;require(setRange(0,2,first,setter)&&registry.size()==1&&setter.flags==2);require(registry.dispatchRestoreWith(false,setter.flags,&command,&source,&target,false));require(values==cached&&registry.size()==0&&setter.flags==0);if(queuedMode)require(queue.cursor==oldCursor+7);else require(object.runs.size()==oldRuns+1&&object.runs.back()==std::array<uint32_t,2>{1024,1});}++cases;
 }std::printf("extended_vertex_restore_composition_cases=%u passed\n",cases);
 }catch(const std::exception& e){std::puts(e.what());return 1;}}
