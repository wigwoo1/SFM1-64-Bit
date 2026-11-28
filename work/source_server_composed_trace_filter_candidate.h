#pragma once
#include "source_server_simple_trace_filter_candidate.h"
#include "source_server_standard_trace_rules_candidate.h"
#include "source_server_pass_entity_filter_candidate.h"

template<class Host>
struct SourceServerFilterBindingsCandidate {
 Host& host;
 bool standardRules(uintptr_t entity,uint32_t mask){return sourceServerStandardTraceRulesCandidate(entity,mask,host);}
 bool passEntity(uintptr_t entity,uintptr_t ignored){return sourceServerPassEntityFilterCandidate(entity,ignored,host);}
 bool isStaticProp(uintptr_t entity){return host.isStaticProp(entity);}
 uintptr_t baseEntity(uintptr_t entity){return host.baseEntity(entity);}
 bool entityAllows(uintptr_t entity,int32_t group,uint32_t mask){return host.entityAllows(entity,group,mask);}
 int32_t collisionGroup(uintptr_t entity){return host.collisionGroup(entity);}
 bool groupsAllow(int32_t a,int32_t b){return host.groupsAllow(a,b);}
 bool extraAllows(uintptr_t entity,uint32_t mask){return host.extraAllows(entity,mask);}
};
template<class Host>
bool sourceServerComposedTraceFilterCandidate(uintptr_t entity,uint32_t mask,uintptr_t ignored,
 int32_t group,bool hasExtra,Host& host){
 SourceServerFilterBindingsCandidate<Host> bindings{host};
 return sourceServerSimpleTraceFilterCandidate(entity,mask,ignored,group,hasExtra,bindings);
}
