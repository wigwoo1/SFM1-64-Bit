#pragma once
#include "studio_device_manager_base_startup_candidate.h"
#include "studio_interface_connector_candidate.h"
#include "studio_interface_factory_fallback_candidate.h"
#include "studio_startup_interface_names_candidate.h"
struct StudioBaseStartupBindingsCandidate {
 std::array<std::uintptr_t,56> cells{};
 std::array<StudioInterfaceBindingCandidate,56> table(StudioDeviceManagerBaseStartupCandidate& state){
  std::array<StudioInterfaceBindingCandidate,56> result{};
  for(unsigned i=0;i<56;++i)result[i]={studioStartupInterfaceNamesCandidate[i],i==1?&state.cvar:i==7?&state.fileSystem:&cells[i]};return result;
 }
};
template<class Factories,class Host>struct StudioComposedBaseStartupOpsCandidate {
 StudioDeviceManagerBaseStartupCandidate& state;StudioInterfaceConnectorCandidate& connector;
 std::array<StudioInterfaceBindingCandidate,56> bindings;StudioInterfaceFactoryNodeCandidate* head;Factories& factories;Host& host;
 std::uintptr_t external(const char* name,int* status){return factories.factory(state.incomingFactory,name,status);}
 std::uintptr_t create(const StudioInterfaceFactoryNodeCandidate& node){return factories.create(node);}
 std::uintptr_t factory(int index,const char* name){if(index!=0)throw std::out_of_range("startup factory index");return studioInterfaceFactoryFallbackCandidate(head,name,nullptr,*this);}
 void diagnostic(unsigned count){host.diagnostic(count);}
 void connectInterfaces(){studioConnectInterfacesCandidate(connector,bindings,1,*this);}
 void initializeCvarAccess(){host.initializeCvarAccess();}
 void registerCvars(unsigned flags){host.registerCvars(flags);}
 void connectResources(){host.connectResources();}
 std::uintptr_t externalFactory(std::uintptr_t token,const char* name){return factories.factory(token,name,nullptr);}
 std::uintptr_t localFactory(const char* name){return studioLocalInterfaceLookupCandidate(head,name,nullptr,*this);}
 void warning(const char* message){host.warning(message);}
 void initializeMathLibrary(float a,float b,float c,int d,int e,int f,int g,int h){host.initializeMathLibrary(a,b,c,d,e,f,g,h);}
};
template<class Factories,class Host>bool studioComposedBaseStartupCandidate(StudioDeviceManagerBaseStartupCandidate& state,StudioInterfaceConnectorCandidate& connector,StudioBaseStartupBindingsCandidate& bindings,StudioInterfaceFactoryNodeCandidate* head,std::uintptr_t self,std::uintptr_t factory,Factories& factories,Host& host){
 StudioComposedBaseStartupOpsCandidate<Factories,Host> ops{state,connector,bindings.table(state),head,factories,host};return studioDeviceManagerBaseConnectCandidate(state,self,factory,ops);
}

