#define main precedingSingleMain
#include "test_material_store_session_scope.cpp"
#undef main
#include "material_session_store_bindings_candidate.h"
using Bindings=MaterialSessionStoreBindingsCandidate;
static uint32_t integer(const Store&s,Store::Id id){return find(*s.get(id)->parameterState,"$a").value.numeric.integer;}
static Store::Id resource(Store&s,const std::string&name){
 Store::Source absent;Store::Id id=0;need(s.createOrReuse(name,"fixture",absent,id),"filename category resource");
 SourceMaterialPatchNodeCandidate root;root.key="VertexLitGeneric";root.children={{"$alpha",1,".25"},{"$color",1,"[1 .5 0]"},{"$a",2,"",7}};
 MaterialObjectParameterOptionsCandidate options;options.externalSource=&root;need(s.initializeParameters(id,options),"actual resource initialization");return id;
}
static void define(Bindings&b,uint32_t element,const std::string&name){need(b.define(element)&&b.setName(element,name)&&b.clearDirty(element),"binding definition");}
static int bindingSafety(){
 unsigned checks=0;auto check=[&](bool b,const char*s){need(b,s);++checks;};Store store;auto a=resource(store,"models/a"),b=resource(store,"models/b");check(store.get(a)->references.load()==0,"lookup object starts unretained");
 {Bindings bindings(store);define(bindings,1,"MODELS\\A.VMT");define(bindings,2,"models/a");
  check(bindings.acquire(1)==a&&store.get(a)->references.load()==1,"normalized actual filename cache retain");check(bindings.acquire(1)==a&&store.get(a)->references.load()==1&&bindings.statistics().lookups==1,"cached element no duplicate AddRef");
  check(bindings.acquire(2)==a&&store.get(a)->references.load()==2,"distinct session element same material retains separately");
  check(bindings.setName(1,"models/b")&&bindings.acquire(1)==a,"dirty name does not silently release/acquire");check(bindings.resolve(1,false)&&bindings.acquire(1)==a&&store.get(a)->references.load()==2,"unavailable system keeps cache");
  check(bindings.resolve(1)&&!bindings.get(1)->binding.resource&&(bindings.get(1)->nameFlags&0x80)&&store.get(a)->references.load()==1,"resolve releases exactly one and leaves dirty bit");check(bindings.acquire(1)==b&&store.get(b)->references.load()==1,"resolve then lazy reacquire changed resource");check(bindings.clearDirty(1)&&bindings.resolve(1)&&bindings.acquire(1)==b&&store.get(b)->references.load()==1,"explicit DataModel dirty clear suppresses repeated invalidation");
  check(bindings.erase(2)&&store.get(a)->references.load()==0,"erase releases other owner only");check(!bindings.erase(999)&&!bindings.define(0)&&!bindings.define(1)&&!bindings.setNameAttributeFlags(1,3),"invalid definitions/flags");
 }
 check(store.get(a)->references.load()==0&&store.get(b)->references.load()==0,"destructor balances actual refs; cache objects survive");
 {Bindings missing(store);define(missing,1,"absent");check(!missing.acquire(1)&&!missing.get(1)->binding.resource,"cache miss is unavailable");unsigned calls=0;Bindings::Attempt attempt;check(!missing.withCollection({{1,{scalar()}}},[&](const auto&){++calls;return true;},0,{},&attempt)&&!calls&&attempt.status==Bindings::Status::UnavailableBinding,"missing resource cannot fake skip");}
 {Bindings zero(store,[](auto)->std::optional<Store::Id>{return Store::Id(0);});define(zero,1,"known-null");unsigned calls=0;check(zero.withCollection({{1,{scalar()}},{0,{scalar()}}},[&](const auto&v){++calls;return v.targets.empty()&&v.sourceTargets.size()==2;})&&calls==1&&zero.statistics().retains==0,"explicit known no-resource skips, no fake handles");}
 {Store categories;auto filename=resource(categories,"same/name");Store::Id supplied=0;auto root=std::make_unique<SourceMaterialPatchNodeCandidate>();root->key="VertexLitGeneric";need(categories.createMaterial("same/name",root,supplied)&&categories.initializeParameters(supplied),"supplied-source category");check(filename!=supplied,"actual distinct category IDs");Bindings binding(categories);define(binding,1,"same/name.vmt");check(binding.acquire(1)==filename,"filename lookup uses category0");}
 {Bindings bindings(store);define(bindings,1,"models/a");define(bindings,2,"models/a");unsigned calls=0;
  const auto*before=store.get(a)->parameterState.get();auto old=snapshot(store,a);Bindings::Attempt attempt;
  // Snapshot includes the reference count at byte8. Acquisition deliberately
  // persists two element-owned refs even when value observation later refuses.
  const uint32_t expectedRetained=2;std::memcpy(old.data()+8,&expectedRetained,4);
  check(!bindings.withCollection({{1,{scalar("$a",2,11)}},{2,{scalar("$a",2,22)}}},[&](const auto&v){++calls;check(integer(store,a)==22&&v.backups[1][0].words[0]==11&&store.get(a)->references.load()==2,"actual acquired resources reach batch scope");check(!bindings.erase(1)&&!bindings.resolve(1)&&!bindings.setName(2,"models/b")&&!bindings.clearDirty(1)&&!bindings.acquire(1)&&!bindings.define(3),"bridge mutation/reentry blocked while observing");return false;},0,{},&attempt)&&calls==1&&attempt.status==Bindings::Status::ValuesRefused,"value observer refusal");
  check(store.get(a)->parameterState.get()==before&&snapshot(store,a)==old&&store.get(a)->references.load()==2&&bindings.get(1)->binding.resource==a,"value failure preserves original parameters AND explicitly retains acquired caches");
  check(bindings.withCollection({{1,{scalar("$a",2,11)}},{1,{scalar("$a",2,22)}}},[](const auto&){return true;})&&integer(store,a)==11&&store.get(a)->references.load()==2,"repeated source handle no duplicate ref; ordered restore");
  check(bindings.erase(1)&&bindings.erase(2)&&store.get(a)->references.load()==0,"retained caches explicitly released");
 }
 {Bindings binding(store);define(binding,1,"models/a");Bindings::Attempt attempt;unsigned calls=0;check(!binding.withCollection({{1,{scalar()}},{999,{scalar()}}},[&](const auto&){++calls;return true;},0,{},&attempt)&&!calls&&attempt.acquiredPositions==1&&store.get(a)->references.load()==1,"late unavailable binding keeps earlier acquisition but no applied prefix");}
 check(store.get(a)->references.load()==0,"late refusal destructor releases acquisition");
 {Bindings*owner=nullptr;Bindings reentry(store,[&](auto)->std::optional<Store::Id>{check(!owner->erase(1)&&!owner->acquire(1)&&!owner->setName(1,"models/b")&&!owner->define(2),"lookup reentry guard");return a;});owner=&reentry;define(reentry,1,"anything");check(reentry.acquire(1)==a,"lookup actual ID retained");}
 {Bindings throwing(store,[](auto)->std::optional<Store::Id>{throw std::runtime_error("read unavailable");});define(throwing,1,"anything");check(!throwing.acquire(1)&&!throwing.get(1)->binding.resource&&store.get(a)->references.load()==0,"lookup throw keeps empty cache");}
 {Bindings bad(store,[&](auto)->std::optional<Store::Id>{return 999999;});define(bad,1,"anything");check(!bad.acquire(1)&&!bad.get(1)->binding.resource&&bad.statistics().retains==0,"stale returned ID cannot populate cache");}
 {Store uninitialized;Store::Source absent;Store::Id id=0;need(uninitialized.createRaw("raw","fixture",absent,id),"raw");Bindings binding(uninitialized,[&](auto)->std::optional<Store::Id>{return id;});define(binding,1,"raw");check(binding.acquire(1)==id&&uninitialized.get(id)->references.load()==1&&!uninitialized.get(id)->parameterState,"cached resource existence does not imply initialized parameters");Bindings::Attempt attempt;unsigned calls=0;check(!binding.withCollection({{1,{scalar()}}},[&](const auto&){++calls;return true;},0,{},&attempt)&&!calls&&attempt.status==Bindings::Status::ValuesRefused&&uninitialized.get(id)->references.load()==1,"uninitialized resource retained; values explicitly refused");}
 {Store stale;auto id=resource(stale,"stale");Bindings binding(stale);define(binding,1,"stale");need(binding.acquire(1)==id&&binding.setName(1,"changed")&&stale.removeAndDelete(id),"explicit force removal with dirty cache fixture");check(!binding.acquire(1)&&!binding.resolve(1)&&!binding.erase(1),"forced removed cached ID refuses; not silently skipped");}
 unsigned failures=0;bool complete=false;
 for(int64_t n=0;n<8192;++n){Store owned;auto id=resource(owned,"models/a");auto balance=heapNew-heapDelete;bool result=false;failAfter=n;
  try{Bindings binding(owned);if(binding.define(1)&&binding.setName(1,"models/a")){result=binding.withCollection({{1,{scalar("$a",2,11)}}},[](const auto&){return true;});}}
  catch(const std::bad_alloc&){}failAfter=-1;
  check(owned.get(id)->references.load()==0&&heapNew-heapDelete==balance,"real allocation failure/success releases all acquired refs");
  if(result){complete=true;break;}++failures;
 }
 check(complete&&failures>100,"full binding/value allocation sweep");
 std::cout<<"PASS session_store_binding_checks="<<checks<<" allocation_failures="<<failures<<" acquisition_cache=PERSISTENT value_failure=ROLLBACK renderer=NOT_CONNECTED\n";return 0;
}
static int bindingWire(const char*query,const char*destination){
 need(!std::ifstream(destination).good(),"fresh binding projection");std::ifstream in(query,std::ios::binary);std::ofstream out(destination,std::ios::binary);need(bool(in)&&bool(out),"wire IO");const char*names[]={"","matA","matB","missing"};auto count=read(in);put(out,count);
 for(unsigned ci=0;ci<count;++ci){auto initial=read(in),replacement=read(in),phase=read(in),flags=read(in),available=read(in),clear=read(in),mode=read(in);need(initial<4&&replacement<4&&mode<2,"wire shape");Store store;auto a=resource(store,"a"),b=resource(store,"b");
  {Bindings binding(store,[&](std::string_view name)->std::optional<Store::Id>{return mode? a:name=="matA"?a:name=="matB"?b:Store::Id(0);});define(binding,1,names[initial]);need(binding.define(2)&&binding.setName(2,names[initial]),"destination member name copy");
   auto acquire=[&](uint32_t id){need(binding.acquire(id).has_value(),"known lazy result");};acquire(1);acquire(2);
   auto snapshot=[&](){for(unsigned id=1;id<=2;++id){auto*e=binding.get(id);unsigned name=0;while(name<4&&*e->binding.name!=names[name])++name;need(name<4,"name projection");put(out,name);put(out,e->binding.resource==a?1:e->binding.resource==b?2:0);put(out,e->nameFlags);}};
   snapshot();need(binding.setNameAttributeFlags(1,uint16_t(flags))&&binding.setName(1,names[replacement],int(phase)),"name phase");snapshot();need(binding.resolve(1,available!=0),"resolve availability");snapshot();acquire(1);snapshot();if(clear)need(binding.clearDirty(1),"clear DataModel dirty");need(binding.resolve(1),"resolve");acquire(1);snapshot();put(out,0);put(out,uint32_t(store.get(a)->references.load()));put(out,uint32_t(store.get(b)->references.load()));const auto&c=binding.statistics();put(out,uint32_t(c.lookups));put(out,uint32_t(c.retains));put(out,uint32_t(c.releases));
  }
  need(store.get(a)->references.load()==0&&store.get(b)->references.load()==0,"wire destructor releases every actual reference");
 }
 out.flush();need(bool(out)&&in.peek()==std::char_traits<char>::eof(),"wire complete");std::cout<<"PASS actual_store_binding_wire_cases="<<count<<'\n';return 0;
}
int main(int argc,char**argv){try{if(argc==1)return bindingSafety();if(argc==4&&std::string(argv[1])=="--wire")return bindingWire(argv[2],argv[3]);throw std::runtime_error("no args OR --wire query fresh-output");}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
