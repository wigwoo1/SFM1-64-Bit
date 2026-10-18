#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
struct StudioInterfaceBindingCandidate{const char* name;std::uintptr_t* output;};
struct StudioInterfaceConnectionRecordCandidate{std::uintptr_t* output=nullptr;std::uint32_t connection=0;};
struct StudioInterfaceConnectorCandidate{std::array<StudioInterfaceConnectionRecordCandidate,64> records{};std::uint32_t tracked=0,connection=0;};
template<class Ops>void studioConnectInterfacesCandidate(StudioInterfaceConnectorCandidate& state,const std::array<StudioInterfaceBindingCandidate,56>& table,int factoryCount,Ops& ops){
 if(static_cast<std::int32_t>(state.tracked)<0){ops.diagnostic(state.tracked);++state.connection;return;}
 const bool first=state.tracked==0;
 auto append=[&](std::uintptr_t* output){if(state.tracked>=state.records.size())throw std::out_of_range("interface connection capacity");state.records[state.tracked++]={output,state.connection};};
 for(int f=0;f<factoryCount;++f)for(const auto& binding:table){if(first&&*binding.output)continue;*binding.output=ops.factory(f,binding.name);if(first){if(*binding.output)append(binding.output);}else{bool found=false;for(std::uint32_t i=0;i<state.tracked;++i){if(i>=state.records.size())throw std::out_of_range("interface connection capacity");if(state.records[i].output==binding.output){state.records[i].output=binding.output;found=true;}}if(!found&&*binding.output)append(binding.output);}}
 ++state.connection;
}

// Verified normal-domain reverse-generation cleanup; records remain retained.
inline void studioDisconnectInterfacesCandidate(StudioInterfaceConnectorCandidate& state){
 --state.connection;
 if(static_cast<std::int32_t>(state.connection)<0||static_cast<std::int32_t>(state.tracked)<=0)return;
 if(state.tracked>state.records.size())throw std::out_of_range("interface connection capacity");
 for(std::uint32_t i=0;i<state.tracked;++i)if(state.records[i].connection==state.connection)*state.records[i].output=0;
}
