#pragma once
#include <cstdint>
// Owned x64 fields; original PE32 offsets are deliberately not reused.
struct StudioRestoreResourceCreationCandidate {
 std::uintptr_t* resources=nullptr;
 std::uintptr_t finalResource=0;
 std::uint32_t sample=0,quality=0;
};
template<class Services> void studioRestoreResourceCreationCandidate(StudioRestoreResourceCreationCandidate& state,Services& services){
 if(services.diagnosticEnabled())services.diagnostic();
 auto initial=state.resources;
 if(initial[0]==0){
  if(services.padding()>0){
   auto output=initial+2;
   services.flush();services.acquireExtra(output);
   std::uint32_t width=0,height=0;services.dimensions(width,height);
   const auto padding=static_cast<std::uint32_t>(services.padding());
   width+=padding*2u;height+=padding*2u;
   auto firstOutput=state.resources;
   const auto sample=state.sample,quality=state.quality;
   const auto format=services.format();
   services.flush();services.create(width,height,format,sample,quality,firstOutput);
  }else{
   services.flush();services.acquireFirst(initial);
   state.resources[2]=0;
  }
 }
 if(services.mode()==2&&state.resources[1]==0){
  std::uint32_t width=0,height=0;services.dimensions(width,height);
  const auto sample=state.sample,quality=state.quality;
  auto output=state.resources+1;
  services.flush();const auto result=services.create(width,height,0x71u,sample,quality,output);
  if(result!=0||state.resources[1]==0){
   output=state.resources+1;
   services.flush();services.create(width,height,0x71u,0,0,output);
  }
 }
 if(state.finalResource==0){services.flush();services.acquireFinal(&state.finalResource);}
}
