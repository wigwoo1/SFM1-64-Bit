#include "material_session_collection_candidate.h"
#include <stdexcept>
#include <iostream>
struct ResourceSafety:MaterialSessionResourceServicesCandidate{
 unsigned finds=0,refs=0;std::string seen;std::optional<uintptr_t>result=uintptr_t(7);bool fail=false;MaterialSessionCachedResourceCandidate*binding=nullptr;bool storedBeforeReference=false;
 std::optional<uintptr_t>findMaterial(std::string_view s)override{++finds;seen=s;if(fail)throw std::runtime_error("Explicit lookup failure");return result;}
 void addReference(uintptr_t p)override{++refs;storedBeforeReference=binding&&binding->resource==p;}
};
struct CollectionSafety:MaterialSessionCollectionServicesCandidate{
 std::vector<std::string>events;uintptr_t missing=0,wrongType=0,zeroResource=0,unknownResource=0;bool failCapture=false,throwCapture=false;uintptr_t failApply=0;std::vector<uint32_t>*appendSource=nullptr;
 uintptr_t resolve(uint32_t h)override{events.push_back("r"+std::to_string(h));if(appendSource){appendSource->push_back(2);appendSource=nullptr;}return h==missing?0:h;}
 bool isMaterial(uintptr_t p)override{events.push_back("t"+std::to_string(p));return p!=wrongType;}
 std::optional<uintptr_t>acquire(uintptr_t p)override{events.push_back("a"+std::to_string(p));if(p==unknownResource)return {};return p==zeroResource?0:p*10;}
 bool capture(uintptr_t b,uintptr_t m)override{events.push_back("c"+std::to_string(b)+":"+std::to_string(m));if(throwCapture)throw std::runtime_error("Explicit capture failure");return !failCapture;}
 bool apply(uintptr_t e,uintptr_t m)override{events.push_back("w"+std::to_string(e)+":"+std::to_string(m));return e!=failApply;}
};
int main(){try{
 static_assert(sizeof(void*)==8,"Owned collection safety x64");unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Collection safety assertion "+std::to_string(checks));++checks;};
 ResourceSafety rs;MaterialSessionCachedResourceCandidate binding;binding.resource=9;binding.name=std::string(5000,'x');check(acquireMaterialSessionResourceCandidate(binding,rs)==9&&!rs.finds&&!rs.refs);
 rs={};binding={};check(acquireMaterialSessionResourceCandidate(binding,rs)==0&&!rs.finds&&!rs.refs);
 rs={};binding={};binding.name="";rs.result=uintptr_t(0);check(acquireMaterialSessionResourceCandidate(binding,rs)==0&&rs.finds==1&&!rs.refs&&rs.seen.empty()&&!binding.resource);
 rs={};binding={};binding.name="";rs.binding=&binding;check(acquireMaterialSessionResourceCandidate(binding,rs)==7&&rs.finds==1&&rs.refs==1&&rs.storedBeforeReference);
 rs={};binding={};binding.name="x";rs.result.reset();check(!acquireMaterialSessionResourceCandidate(binding,rs)&&!binding.resource&&!rs.refs);
 rs={};binding={};binding.name=std::string(4096,'x');check(!acquireMaterialSessionResourceCandidate(binding,rs)&&!rs.finds&&!binding.resource);
 rs={};binding={};binding.name=std::string("abc\0other",9);check(acquireMaterialSessionResourceCandidate(binding,rs)==7&&rs.seen=="abc");
 rs={};binding={};binding.name="x";rs.result=uintptr_t(0);acquireMaterialSessionResourceCandidate(binding,rs);check(acquireMaterialSessionResourceCandidate(binding,rs)==0&&rs.finds==2&&!rs.refs);
 rs={};binding={};binding.name="x";acquireMaterialSessionResourceCandidate(binding,rs);check(acquireMaterialSessionResourceCandidate(binding,rs)==7&&rs.finds==1&&rs.refs==1);
 rs={};binding={};binding.name="x";rs.fail=true;bool threw=false;try{acquireMaterialSessionResourceCandidate(binding,rs);}catch(const std::runtime_error&){threw=true;}check(threw&&!binding.resource&&!rs.refs);
 CollectionSafety s;std::vector<uint32_t>back{3};check(applyMaterialSessionCollectionCandidate({1,2},&back,s)&&s.events==std::vector<std::string>({"r1","t1","a1","r3","t3","c3:10","w1:10","r2","t2","a2","w2:20"}));
 s={};s.missing=1;check(applyMaterialSessionCollectionCandidate({1},&back,s)&&s.events==std::vector<std::string>({"r1"}));
 s={};s.wrongType=1;check(applyMaterialSessionCollectionCandidate({1},&back,s)&&s.events==std::vector<std::string>({"r1","t1"}));
 s={};s.zeroResource=1;check(applyMaterialSessionCollectionCandidate({1},&back,s)&&s.events==std::vector<std::string>({"r1","t1","a1"}));
 s={};s.unknownResource=1;check(!applyMaterialSessionCollectionCandidate({1},&back,s)&&s.events==std::vector<std::string>({"r1","t1","a1"}));
 s={};s.wrongType=3;check(applyMaterialSessionCollectionCandidate({1},&back,s)&&s.events==std::vector<std::string>({"r1","t1","a1","r3","t3","c0:10","w1:10"}));
 s={};s.missing=3;check(applyMaterialSessionCollectionCandidate({1},&back,s)&&s.events==std::vector<std::string>({"r1","t1","a1","r3","c0:10","w1:10"}));
 s={};back.clear();check(applyMaterialSessionCollectionCandidate({1,1},&back,s)&&s.events==std::vector<std::string>({"r1","t1","a1","w1:10","r1","t1","a1","w1:10"}));
 s={};check(!applyMaterialSessionCollectionCandidate(std::vector<uint32_t>(4097,1),nullptr,s)&&s.events.empty());
 s={};back.assign(4097,1);check(!applyMaterialSessionCollectionCandidate({1},&back,s)&&s.events.empty());
 s={};s.failCapture=true;back={3};check(!applyMaterialSessionCollectionCandidate({1,2},&back,s)&&s.events==std::vector<std::string>({"r1","t1","a1","r3","t3","c3:10"}));
 s={};s.failApply=2;check(!applyMaterialSessionCollectionCandidate({1,2},nullptr,s)&&s.events==std::vector<std::string>({"r1","t1","a1","w1:10","r2","t2","a2","w2:20"})); // Already applied1: caller owns rollback.
 s={};s.throwCapture=true;threw=false;try{applyMaterialSessionCollectionCandidate({1},&back,s);}catch(const std::runtime_error&){threw=true;}check(threw&&s.events.back()=="c3:10");
 s={};check(applyMaterialSessionCollectionCandidate({},nullptr,s)&&s.events.empty());
 s={};std::vector<uint32_t>growingSource{1};s.appendSource=&growingSource;check(applyMaterialSessionCollectionCandidate(growingSource,nullptr,s)&&growingSource.size()==2&&s.events==std::vector<std::string>({"r1","t1","a1","w1:10"}));
 std::cout<<"PASS collection_safety_checks="<<checks<<" rollback_owner=CALLER original_state_restore=NOT_GUARANTEED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
