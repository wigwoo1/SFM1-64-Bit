#include <cstdlib>
#include <new>
#include <cstdint>
static int64_t failAfter=-1;static size_t heapNew=0,heapDelete=0;
void*operator new(size_t n){if(failAfter>=0&&failAfter--==0)throw std::bad_alloc();auto*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();++heapNew;return p;}
void operator delete(void*p)noexcept{if(p)++heapDelete;std::free(p);}
void operator delete(void*p,size_t)noexcept{::operator delete(p);}
void*operator new[](size_t n){return ::operator new(n);}
void operator delete[](void*p)noexcept{::operator delete(p);}
void operator delete[](void*p,size_t)noexcept{::operator delete(p);}
#include "material_object_store_candidate.h"
#include <fstream>
#include <iostream>
#include <sstream>
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
using Store=MaterialObjectStoreCandidate;
static Store::Id make(Store&s,const std::string&alpha=".25",const std::shared_ptr<MaterialProxyFactoryCandidate>&factory={}){
 auto root=std::make_unique<SourceMaterialPatchNodeCandidate>();root->key="VertexLitGeneric";root->children={{"$alpha",1,alpha},{"$color",1,"[1 .5 0]"},{"$counter",2,"",17},{"$custom_matrix",1,"[1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]"}};
 if(factory){SourceMaterialPatchNodeCandidate proxies;proxies.key="Proxies";proxies.children.push_back({"Spy"});root->children.push_back(std::move(proxies));}
 Store::Id id=0;need(s.createMaterial("fixture/scope/"+std::to_string(s.size()),root,id),"create actual store material");MaterialObjectParameterOptionsCandidate options;options.proxyFactory=factory;need(s.initializeParameters(id,options),"initialize actual store parameters");return id;
}
static const MaterialTypedDispatchCandidate&find(const MaterialObjectParameterStateCandidate&state,const std::string&name){const auto&v=state.material.parameters.values;for(size_t i=v.size();i>0;--i)if(v[i-1].name==name)return v[i-1];throw std::runtime_error("test parameter absent");}
static MaterialSessionOverrideInputCandidate scalar(const char*name="$alpha",uint32_t kind=3,uint32_t bits=0x3f400000){MaterialSessionOverrideInputCandidate r;r.name=name;r.typeFlags=kind;r.words[0]=bits;return r;}
static void put(std::ostream&o,uint32_t v){o.write(reinterpret_cast<const char*>(&v),4);}
static void text(std::ostream&o,const std::string&s){put(o,uint32_t(s.size()));o.write(s.data(),s.size());}
static void record(std::ostream&o,const MaterialTypedDispatchCandidate&v){put(o,v.value.numeric.typeAndMetadata&0x7fu);put(o,v.value.numeric.integer);for(auto w:v.value.numeric.vector)put(o,w);text(o,(v.value.numeric.typeAndMetadata&15u)==1?v.value.text:std::string());}
static std::string snapshot(const Store&s,Store::Id id){std::ostringstream o(std::ios::binary);const auto*p=s.get(id);need(p&&p->parameterState,"snapshot target");put(o,p->flags);put(o,uint32_t(p->parameterCount));put(o,uint32_t(p->references.load()));put(o,p->forwarding.valuesInitialized);for(auto v:p->forwarding.values)put(o,v);for(const auto&v:p->parameterState->material.parameters.values){text(o,v.name);record(o,v);put(o,v.matrix.has_value());if(v.matrix)for(auto w:*v.matrix)put(o,w);}return o.str();}
struct Factory;
struct Spy:MaterialProxyInstanceCandidate{Factory&factory;explicit Spy(Factory&f):factory(f){}bool initialize(const MaterialProxyContextCandidate&,const SourceMaterialPatchNodeCandidate&)override;bool evaluate(MaterialProxyEvaluationCandidate&)override{return true;}};
struct Factory:MaterialProxyFactoryCandidate{MaterialProxyBindingCandidate binding;unsigned created=0,destroyed=0;MaterialProxyInstanceCandidate*create(const std::string&)override{++created;return new Spy(*this);}void destroy(MaterialProxyInstanceCandidate*p)noexcept override{++destroyed;delete p;}};
bool Spy::initialize(const MaterialProxyContextCandidate&c,const SourceMaterialPatchNodeCandidate&){factory.binding=c.find("$alpha");return factory.binding.valid();}
static int safety(){
 static_assert(sizeof(void*)==8,"owned x64 scope");unsigned checks=0;auto check=[&](bool b,const char*what){need(b,what);++checks;};
 Store s;const auto id=make(s);auto rows=std::vector<MaterialSessionOverrideInputCandidate>{scalar()};const auto*original=s.get(id)->parameterState.get();const auto before=snapshot(s,id);unsigned calls=0;
 std::function<bool(const MaterialObjectParameterStateCandidate&)>observe=[&](const auto&state){++calls;return find(state,"$alpha").value.numeric.vector[0]==0x3f400000;};
 for(auto stage:{Store::Allocation::sessionSnapshot,Store::Allocation::sessionApply,Store::Allocation::sessionRestore,Store::Allocation::sessionPublish})for(bool throwing:{false,true}){
  calls=0;auto balance=heapNew-heapDelete;check(!s.withSessionOverrides(id,rows,observe,0,[&](Store::Allocation k,size_t){if(k==stage){if(throwing)throw std::bad_alloc();return false;}return true;}),"policy denial/throw refused");check(calls==0&&s.get(id)->parameterState.get()==original&&snapshot(s,id)==before&&!s.get(id)->sessionPending,"policy failure preserves pointer/state");check(heapNew-heapDelete==balance,"policy failure heap balance");
 }
 for(unsigned mode=0;mode<3;++mode){calls=0;check(!s.withSessionOverrides(id,rows,[&](const auto&state)->bool{++calls;need(&state==s.get(id)->parameterState.get()&&&state!=original,"applied state is actual store publication");if(mode==1)throw std::runtime_error("observer failure");if(mode==2)throw std::bad_alloc();return false;}),"observer failure refused");check(calls==1&&s.get(id)->parameterState.get()==original&&snapshot(s,id)==before&&!s.get(id)->sessionPending,"observer failure rollback original publication");}
 check(!s.withSessionOverrides(0,rows,observe)&&!s.withSessionOverrides(id,rows,{}),"invalid id/absent observer");
 {Store fresh;auto root=std::make_unique<SourceMaterialPatchNodeCandidate>();root->key="VertexLitGeneric";Store::Id uninitialized=0;need(fresh.createMaterial("uninitialized",root,uninitialized),"uninitialized fixture");check(!fresh.withSessionOverrides(uninitialized,rows,observe),"uninitialized refused");}
 auto invalid=rows;invalid.push_back(scalar("$color",5));invalid.back().textureName="texture/name";calls=0;check(!s.withSessionOverrides(id,invalid,observe)&&calls==0&&snapshot(s,id)==before,"late type5 is unavailable texture, not SetString");
 invalid=rows;invalid[0].name.assign(4096,'x');check(!s.withSessionOverrides(id,invalid,observe)&&snapshot(s,id)==before,"row extent unchanged");
 // Fail every actual heap allocation until all preparation can complete.
 unsigned allocationFailures=0;bool completed=false;
 for(int64_t n=0;n<4096;++n){calls=0;const auto balance=heapNew-heapDelete;failAfter=n;bool ok=s.withSessionOverrides(id,rows,observe);failAfter=-1;
  if(ok){check(calls==1,"successful allocation sweep observer");completed=true;break;}
  ++allocationFailures;bool unchanged=snapshot(s,id)==before;check(unchanged&&s.get(id)->parameterState.get()==original&&!s.get(id)->sessionPending&&heapNew-heapDelete==balance,"actual allocation failure rollback/balance");
 }
 check(completed&&allocationFailures>20,"allocation sweep completed");
 check((find(*s.get(id)->parameterState,"$alpha").value.numeric.typeAndMetadata&15u)==0&&find(*s.get(id)->parameterState,"$alpha").value.text.empty()&&s.get(id)->forwarding.values[0]==0x3e800000,"success restore follows native float setter, not original string type");
 auto success=snapshot(s,id);original=s.get(id)->parameterState.get();
 calls=0;check(s.withSessionOverrides(id,rows,[&](const auto&state){++calls;check(s.get(id)->sessionPending&&s.get(id)->forwarding.values[0]==0x3f400000,"published forwarding view");check(&state==s.get(id)->parameterState.get(),"observer views live owned state");check(!s.increment(id)&&!s.decrement(id)&&!s.removeOnly(id)&&!s.removeAndDelete(id)&&!s.deleteIfUnreferenced(id),"target reference/removal guards");check(!s.initializeParameters(id)&&!s.uncacheParameters(id)&&!s.uncacheParameters(id,true)&&!s.evaluateProxies(id),"target initialization/uncache/proxy guards");check(!s.forward(id,Store::Operation::decrement,false)&&!s.forward(id,Store::Operation::decrement,true)&&!s.withSessionOverrides(id,rows,observe),"target queue/nested guards");return true;}),"successful protected scope");check(calls==1&&snapshot(s,id)==success,"successful numeric state restored");
 {auto frozenInput=rows;check(s.withSessionOverrides(id,frozenInput,[&](const auto&){frozenInput[0].words[0]=0;frozenInput.clear();return true;})&&snapshot(s,id)==success,"input snapshot survives caller edits");}
 {auto*root=s.get(id)->source.get();const auto old=root->children[0].text;check(!s.withSessionOverrides(id,rows,[&](const auto&){root->children[0].text="explicit external edit";return true;})&&s.get(id)->parameterState.get()!=nullptr&&snapshot(s,id)==success&&root->children[0].text=="explicit external edit","source change rejects and rolls back parameters, preserves external edit");root->children[0].text=old;}
 for(int phase:{-1,0,1,2,3,4,5})for(uint32_t flags:{0u,0x20u,0x400u}){Store phaseStore;auto target=make(phaseStore);auto input=std::vector<MaterialSessionOverrideInputCandidate>{scalar()};input[0].typeFlags|=flags;
  check(phaseStore.withSessionOverrides(target,input,[](const auto&){return true;},phase),"phase pilot completes");bool captured=!(flags&0x20)&&(phase==0||phase==1||phase==2||(phase==4&&!(flags&0x400)));check(find(*phaseStore.get(target)->parameterState,"$alpha").value.numeric.vector[0]==(captured?0x3e800000u:0x3f400000u),"native capture gate may leave incoming restore value");
 }
 {Store typed;auto target=make(typed);auto input=std::vector<MaterialSessionOverrideInputCandidate>{scalar("$counter",2,99),scalar("$color",10,0x3e800000),scalar("$custom_matrix",14)};input[1].words[1]=input[1].words[2]=0x3e800000;for(unsigned i=0;i<16;++i)input[2].words[i]=i%5==0?0x40000000:0;
  check(typed.withSessionOverrides(target,input,[&](const auto&state){return find(state,"$counter").value.numeric.integer==99&&find(state,"$color").value.numeric.vector[0]==0x3e800000&&find(state,"$custom_matrix").matrix->at(0)==0x40000000;}),"integer/vector/matrix applied");check(find(*typed.get(target)->parameterState,"$counter").value.numeric.integer==17&&find(*typed.get(target)->parameterState,"$color").value.numeric.vector[0]==0x3f800000&&find(*typed.get(target)->parameterState,"$custom_matrix").matrix->at(0)==0x3f800000,"typed values restored");
  check(typed.withSessionOverrides(target,{scalar("$counter",4,1)},[](const auto&){return true;})&&find(*typed.get(target)->parameterState,"$counter").value.numeric.integer==1,"bool backup/restore is native nonzero projection, not raw integer snapshot");
 }
 {Store queued;auto a=make(queued),b=make(queued);need(queued.forward(a,Store::Operation::decrement,true)&&queued.forward(a,Store::Operation::deleteIfUnreferenced,true),"queued existing work");check(queued.withSessionOverrides(a,rows,[&](const auto&){queued.drain();check(queued.queuedCount()==2&&queued.get(a)->references.load()==1,"drain deferred during scope");return queued.withSessionOverrides(b,rows,[&](const auto&){queued.drain();return queued.queuedCount()==2;});}),"nested different targets safe");queued.drain();check(!queued.get(a)&&queued.get(b)&&queued.queuedCount()==0,"deferred original queue drains after all scopes");}
 {auto factory=std::make_shared<Factory>();Store proxies;auto target=make(proxies,".25",factory);const auto*state=proxies.get(target)->parameterState.get();check(!proxies.withSessionOverrides(target,rows,[&](const auto&){check(factory->binding.read()->value.numeric.vector[0]==0x3f400000,"proxy binding reads applied publication");return false;})&&proxies.get(target)->parameterState.get()==state&&factory->binding.read()->value.text==".25","proxy binding rollback retargeted before transient free");check(proxies.withSessionOverrides(target,rows,[](const auto&){return true;})&&factory->binding.read()->value.numeric.vector[0]==0x3e800000,"proxy binding retargeted to successful restore");check(proxies.evaluateProxies(target)&&factory->binding.valid(),"ordinary proxy evaluation remains usable after scope");check(proxies.uncacheParameters(target)&&!factory->binding.valid()&&factory->created==factory->destroyed,"proxy lifetime retires correctly");}
 check(s.decrement(id)&&s.deleteIfUnreferenced(id)&&s.size()==0&&s.statistics().created==s.statistics().destroyed,"target destruction balanced");
 std::cout<<"PASS material_store_session_scope_checks="<<checks<<" actual_allocation_failures="<<allocationFailures<<" texture_handles=REFUSED renderer=NOT_CONNECTED\n";return 0;
}
static uint32_t read(std::istream&i){uint32_t v;need(bool(i.read(reinterpret_cast<char*>(&v),4)),"word");return v;}
static int wire(const char*query,const char*destination){need(!std::ifstream(destination).good(),"fresh wire output");std::ifstream in(query,std::ios::binary);std::ofstream out(destination,std::ios::binary);need(bool(in)&&bool(out),"wire IO");auto count=read(in);need(count&&count<4096,"wire count");put(out,count);
 for(unsigned i=0;i<count;++i){auto n=read(in);need(n<=128,"alpha extent");std::string alpha(n,'\0');need(bool(in.read(alpha.data(),n)),"alpha");const auto bits=read(in);Store store;auto id=make(store,alpha);record(out,find(*store.get(id)->parameterState,"$alpha"));need(store.withSessionOverrides(id,{scalar("$alpha",3,bits)},[&](const auto&state){record(out,find(state,"$alpha"));return bool(out);}),"wire store scope");record(out,find(*store.get(id)->parameterState,"$alpha"));need(store.decrement(id)&&store.deleteIfUnreferenced(id),"wire store deletion");}
 need(in.peek()==std::char_traits<char>::eof(),"wire tail");out.flush();need(bool(out),"wire output");std::cout<<"PASS store_scope_wire_cases="<<count<<'\n';return 0;
}
int main(int argc,char**argv){try{if(argc==1)return safety();if(argc==4&&std::string(argv[1])=="--wire")return wire(argv[2],argv[3]);throw std::runtime_error("no args for safety OR --wire query fresh-output");}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
