#include "native_queued_query_recreation_candidate.h"
#include <cassert>
#include <cstdio>
struct Query{unsigned id;};
struct Device{unsigned id;unsigned calls=0;int fail=-1;bool otherFailure=false;Query* query;HRESULT CreateQuery(D3DQUERYTYPE type,Query** out){assert(type==D3DQUERYTYPE_EVENT);const unsigned index=calls++;*out=query;if(static_cast<int>(index)==fail)return otherFailure?E_FAIL:D3DERR_NOTAVAILABLE;return S_OK;}};
struct State{Device* device;};
struct Lifecycle{State* state;Device* replacement;bool replace;unsigned captures=0,flushes=0,loads=0;State* commandState(){++captures;return state;}void flush(State* s){assert(s==state);++flushes;if(replace)s->device=replacement;}Device* device(State* s){assert(s==state&&flushes==loads+1);++loads;return s->device;}};
struct RealLifecycle{IDirect3DDevice9** state;IDirect3DDevice9** commandState(){return state;}void flush(IDirect3DDevice9**){}IDirect3DDevice9* device(IDirect3DDevice9** s){return *s;}};
void compileSdk(NativeQueuedQueryOwnerCandidateT<>& owner,RealLifecycle& lifecycle){NativeQueuedQueryServicesCandidateT<RealLifecycle> services{lifecycle};studioQueuedQueryRecreationCandidate(owner,services);}
int main(){unsigned cases=0;for(int32_t capability:{-1,0,1,2})for(bool replace:{false,true})for(int failure:{-1,0,9,19})for(bool other:{false,true}){
 Query oldQuery{1},newQuery{2};Device original{1,0,failure,other,&oldQuery},replacement{2,0,failure,other,&newQuery};State state{&original};Lifecycle lifecycle{&state,&replacement,replace};unsigned char active=7;std::array<Query*,20> queries{};queries.fill(&oldQuery);NativeQueuedQueryOwnerCandidateT<Query> owner{capability,active,queries};NativeQueuedQueryServicesCandidateT<Lifecycle> services{lifecycle};studioQueuedQueryRecreationCandidate(owner,services);
 if(capability!=1){assert(active==0&&lifecycle.captures==0&&original.calls==0&&replacement.calls==0);for(auto* q:queries)assert(q==nullptr);}
 else{assert(lifecycle.captures==20&&lifecycle.flushes==20&&lifecycle.loads==20);assert(original.calls==(replace?0u:20u)&&replacement.calls==(replace?20u:0u));assert(active==((failure>=0&&!other)?0:1));for(unsigned i=0;i<20;++i)assert(queries[i]==((static_cast<int>(i)==failure&&!other)?nullptr:(replace?&newQuery:&oldQuery)));}
 ++cases;
 }std::printf("queued_query_recreation_cases=%u passed\n",cases);
}
