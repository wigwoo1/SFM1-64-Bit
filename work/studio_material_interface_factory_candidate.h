#pragma once
#include "studio_shader_interface_factory_candidate.h"
#include "studio_interface_factory_fallback_candidate.h"
template<class Ops>std::uintptr_t studioMaterialInterfaceFactoryCandidate(StudioInterfaceFactoryNodeCandidate* head,const char* name,int* status,Ops& ops){
 if(status)*status=0;
 if(studioInterfaceNameEqualCandidate(name,"VFileSystem017"))return ops.direct(0);
 if(studioInterfaceNameEqualCandidate(name,"QueuedLoaderVersion001"))return ops.direct(1);
 if(studioInterfaceNameEqualCandidate(name,"VShaderUtil001"))return ops.direct(2);
 if(ops.hasUpstream()){auto result=ops.upstream(name,nullptr);if(result)return result;}
 auto result=studioLocalInterfaceLookupCandidate(head,name,nullptr,ops);if(result)return result;
 if(status)*status=1;return 0;
}
