#pragma once
#include <cstdint>
#include <cstring>
struct StudioInterfaceFactoryNodeCandidate{const char* name;std::uintptr_t token;StudioInterfaceFactoryNodeCandidate* next;};
template<class Ops>std::uintptr_t studioLocalInterfaceLookupCandidate(StudioInterfaceFactoryNodeCandidate* head,const char* name,int* status,Ops& ops){for(auto* node=head;node;node=node->next)if(std::strcmp(node->name,name)==0){if(status)*status=0;return ops.create(*node);}if(status)*status=1;return 0;}
template<class Ops>std::uintptr_t studioInterfaceFactoryFallbackCandidate(StudioInterfaceFactoryNodeCandidate* head,const char* name,int* status,Ops& ops){if(status)*status=0;auto result=ops.external(name,status);if(result)return result;result=studioLocalInterfaceLookupCandidate(head,name,status,ops);if(result)return result;if(status)*status=1;return 0;}
