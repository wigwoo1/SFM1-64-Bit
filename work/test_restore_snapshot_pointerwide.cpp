#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include "native_restore_snapshot_candidate.h"
#include "native_restore_callback_lists_candidate.h"
#include "native_restore_dispatch_snapshot_finish_candidate.h"
#include "native_manager_callback_registry_candidate.h"
struct State{std::array<uint32_t,926> values{};std::array<uint32_t*,6> staging{},primary{};};
static void check(bool condition);
struct Services{State& source;State& destination;const State& expected;uint8_t& force;unsigned& gate;unsigned finished=0;bool mutate;
 void clearRestoreForce(){force=0;}
 void completeRestore(bool value){check(value&&force==0&&gate==2);check(destination.values==expected.values&&destination.primary==expected.primary&&destination.staging==expected.staging);++finished;if(mutate)force=7;}
 auto staging(){return destination.staging;}
 void copyState(){destination=source;}
 void restoreStaging(const std::array<uint32_t*,6>& saved){destination.staging=saved;}
};
static void check(bool condition){if(!condition)throw std::runtime_error("pointerwide snapshot mismatch");}
struct Command{std::array<uint32_t*,6> replacement;unsigned callbacks=0;};
static void updateSource(Command* command,State* current,State* previous,bool force){check(force);++command->callbacks;previous->values[17]=0x12345678;current->staging=command->replacement;}
static void updateLater(Command* command,State* current,State* previous,bool force){check(force);++command->callbacks;check(current->staging==command->replacement||command->callbacks==1);previous->values[19]=0xfedcba98;}
int main(){try{
 std::array<std::array<uint32_t,8>,24> buffers{};
 for(unsigned i=0;i<buffers.size();++i){check(reinterpret_cast<uintptr_t>(buffers[i].data())>UINT32_MAX);buffers[i].fill(i+1);}
 const auto originalBuffers=buffers;
 for(bool mutation:{false,true})for(bool suppressFirst:{false,true})for(bool suppressSecond:{false,true}){
  State source,destination;Command command;for(unsigned i=0;i<source.values.size();++i){source.values[i]=i*31u;destination.values[i]=~source.values[i];}
  for(unsigned i=0;i<6;++i){source.staging[i]=buffers[i].data();source.primary[i]=buffers[i+6].data();destination.staging[i]=buffers[i+12].data();destination.primary[i]=buffers[i+18].data();command.replacement[i]=buffers[i+18].data();}
  const auto savedSource=source;const auto savedDestination=destination;
  using Callback=void(*)(Command*,State*,State*,bool);NativeManagerCallbackAllocatorCandidate allocator;
  NativeManagerCallbackRegistryCandidateT<NativeManagerCallbackAllocatorCandidate,Callback> first{allocator},second{allocator};first.append(updateSource);second.append(updateLater);
  uint8_t firstFlags=2,secondFlags=1;unsigned gate=0;
  State atFinish=source;if(!suppressFirst)atFinish.values[17]=0x12345678;if(!suppressSecond)atFinish.values[19]=0xfedcba98;atFinish.staging=suppressFirst?savedDestination.staging:command.replacement;uint8_t force=1;Services services{source,destination,atFinish,force,gate,0,mutation};nativeRestoreDispatchSnapshotFinishCandidate(first,firstFlags,second,secondFlags,[&]{return gate++==0?suppressFirst:suppressSecond;},services,&command,&destination,&source,true);
  const auto stagedAfterCallbacks=suppressFirst?savedDestination.staging:command.replacement;
  auto expectedSource=savedSource;if(!suppressFirst)expectedSource.values[17]=0x12345678;if(!suppressSecond)expectedSource.values[19]=0xfedcba98;
  check(source.values==expectedSource.values&&source.staging==savedSource.staging&&source.primary==savedSource.primary);
  check(destination.values==expectedSource.values&&destination.primary==source.primary&&destination.staging==stagedAfterCallbacks);
  check(destination.staging==(suppressFirst?savedDestination.staging:command.replacement));
  for(auto* pointer:destination.staging)check(reinterpret_cast<uintptr_t>(pointer)>UINT32_MAX);
  check(first.size()==(suppressFirst?1:0)&&second.size()==(suppressSecond?1:0));check(firstFlags==(suppressFirst?2:0)&&secondFlags==(suppressSecond?1:0));
  check(services.finished==1&&force==(mutation?7:0));check(gate==2&&command.callbacks==unsigned(!suppressFirst)+unsigned(!suppressSecond));check(buffers==originalBuffers);
 }
 std::puts("restore_snapshot_pointerwide_composition_cases=8 passed");return 0;
 }catch(const std::exception& error){std::puts(error.what());return 1;}}

