// Reuse the accepted private constructor fixture without altering it.
// This test joins whole backup builder/copy wrapper to the actual constructor.
#define main preservedElementBindingMain
#include "probe_material_element_binding.cpp"
#undef main
#ifndef SFM64_NATIVE_ELEMENT_BINDING
#include "material_session_backup_construction_candidate.h"
#endif
struct SourceSpec{uint32_t available=1,mode=0,name=1,file=123,extra=1,text=2;};
static std::array<SourceSpec,2>specs;
static std::vector<std::array<uint32_t,3>>trace;
static unsigned sourceCount=0,modelPresent=1;
static void emit(uint32_t k,uint32_t a,uint32_t b){trace.push_back({k,a,b});}
#ifdef SFM64_NATIVE_ELEMENT_BINDING
struct BuilderFixture:Backend{
 inline static BuilderFixture*active=nullptr;
 std::array<std::array<uint8_t,0x68>,64>poolObjects{};
 std::array<std::array<uint8_t,24>,256>poolAttributes{};
 std::array<uint32_t,128>queued{};
 std::array<uint8_t*,2>sources{};
 struct Copy{uint8_t*element;uint32_t source,handle;};std::vector<Copy>copies;
 std::array<uint8_t,0x64>model{};
 unsigned objectCount=0;bool unavailableCopy=false;
 explicit BuilderFixture(const char*path):Backend(path){active=this;dmMethods[16]=reinterpret_cast<uintptr_t>(&create);dmMethods[18]=reinterpret_cast<uintptr_t>(&resolveHandle);}
 uint8_t*make(uint32_t handle,const char*name,uint32_t file){need(objectCount<poolObjects.size(),"object pool extent");std::array<uint32_t,4>id{};using Factory=void*(__thiscall*)(void*,uint32_t,uint32_t,const char*,uint32_t,const void*);auto p=static_cast<uint8_t*>(reinterpret_cast<Factory>(0x105c13b0)(factory.data(),handle,0,name,file,id.data()));need(p==poolObjects[objectCount++].data(),"actual factory free-list identity");reinterpret_cast<void(__thiscall*)(void*)>(0x105c1380)(p);return p;}
 static uint32_t __fastcall create(void*,void*,uint32_t type,const char*name,uint32_t file,void*id){auto&a=*active;need(!id&&type==reinterpret_cast<uintptr_t>(intern("DmeMaterial")),"create16 type/id contract");uint32_t source=0;for(unsigned i=0;i<sourceCount;++i){auto p=reinterpret_cast<const char*>(uintptr_t(word(a.sources[i],0x14)));if(!std::strcmp(name,p)){source=i+1;break;}}need(source&&file==specs[source-1].file,"create16 source name/file");const auto mode=specs[source-1].mode;if(mode==1){emit(1,source,UINT32_MAX);return UINT32_MAX;}const auto handle=1000+uint32_t(a.copies.size());auto p=a.make(handle,name,file);a.copies.push_back({p,source,handle});a.unavailableCopy=mode==2;emit(1,source,handle);return handle;}
 static void*__fastcall resolveHandle(void*,void*,uint32_t handle){auto&a=*active;uint8_t*p=nullptr;uint32_t resolved=0;if(handle==900){if(modelPresent){p=a.model.data();resolved=900;}}else if(handle>=1000&&handle<1000+a.copies.size()){if(!a.unavailableCopy){p=a.copies[handle-1000].element;resolved=handle;}a.unavailableCopy=false;}else if(handle&&handle<=sourceCount&&specs[handle-1].available){p=a.sources[handle-1];resolved=handle;}emit(0,handle,resolved);return p;}
 void resetCase(){need(allocations.empty(),"previous heap cleanup");strings.clear();copies.clear();objectCount=0;unavailableCopy=false;model.fill(0);sources.fill(nullptr);for(auto&o:poolObjects)o.fill(0);for(auto&a:poolAttributes)a.fill(0);for(size_t i=0;i+1<poolObjects.size();++i)field(poolObjects[i].data(),0,reinterpret_cast<uintptr_t>(poolObjects[i+1].data()));for(size_t i=0;i+1<poolAttributes.size();++i)field(poolAttributes[i].data(),0,reinterpret_cast<uintptr_t>(poolAttributes[i+1].data()));
  std::memset(image.base+0xf62cbc,0,0x28);data(0xf62cbc,0x68);data(0xf62cc8,reinterpret_cast<uintptr_t>(poolObjects[0].data()));std::memset(image.base+0xf49cac,0,0x28);data(0xf49cac,24);data(0xf49cb8,reinterpret_cast<uintptr_t>(poolAttributes[0].data()));queue.fill(0);queued.fill(0);field(queue.data(),0x64,reinterpret_cast<uintptr_t>(queued.data()));field(queue.data(),0x68,queued.size());data(0xf4c774,reinterpret_cast<uintptr_t>(&system));data(0xe9a684,reinterpret_cast<uintptr_t>(intern("DmElement")));data(0xeb4ec8,reinterpret_cast<uintptr_t>(intern("DmeMaterial")));field(factory.data(),0x18,reinterpret_cast<uintptr_t>("DmeMaterial"));currentPhase=0;
  for(unsigned i=0;i<sourceCount;++i){auto&s=specs[i];sources[i]=make(i+1,("source"+std::to_string(i+1)).c_str(),s.file);if(s.name<4)reinterpret_cast<void(__thiscall*)(void*,const char*)>(0x105c11c0)(sources[i],names[s.name]);if(s.extra){auto p=reinterpret_cast<void*(__thiscall*)(void*,const char*,uint32_t)>(0x103614f0)(sources[i],"$probe",5);if(s.text<4){const auto text=uint32_t(reinterpret_cast<uintptr_t>(intern(names[s.text])));reinterpret_cast<void(__thiscall*)(void*,const uint32_t*)>(0x10366100)(p,&text);}}}
  image.verify();
 }
 uint32_t acquirePointer(uint8_t*p){auto r=reinterpret_cast<Mat*(__thiscall*)(void*)>(0x105c1220)(p);return r?r->id:0;}
 void warm(){for(unsigned i=0;i<sourceCount;++i)if(specs[i].available)acquirePointer(sources[i]);}
 void build(const std::vector<uint32_t>&handles,uint32_t attributeType,std::vector<uint32_t>&result){std::array<uint8_t,24>attribute{};std::array<uint8_t,20>array{};field(array.data(),0,reinterpret_cast<uintptr_t>(handles.data()));field(array.data(),12,handles.size());field(attribute.data(),4,reinterpret_cast<uintptr_t>(array.data()));field(attribute.data(),0x10,attributeType);field(attribute.data(),0x14,reinterpret_cast<uintptr_t>(intern("materials")));field(model.data(),0x30,attributeType==UINT32_MAX?0:reinterpret_cast<uintptr_t>(attribute.data()));std::array<uint8_t,0xe4>puppet{};std::array<uint32_t,64>storage{};field(puppet.data(),0x2c,900);field(puppet.data(),0x98,reinterpret_cast<uintptr_t>(storage.data()));field(puppet.data(),0x9c,storage.size());field(puppet.data(),0xa4,7);field(puppet.data(),0xa8,reinterpret_cast<uintptr_t>(storage.data()));reinterpret_cast<void(__thiscall*)(void*)>(0x10240950)(puppet.data());const auto size=word(puppet.data(),0xa4);need(size<=handles.size(),"backup output bound");result.assign(storage.begin(),storage.begin()+size);need(!unavailableCopy,"copy resolution completed");image.verify();}
 void acquirePairs(const std::vector<uint32_t>&handles,const std::vector<uint32_t>&result){for(size_t i=0;i<result.size();++i){const auto h=handles[i];if(h&&h<=sourceCount&&specs[h-1].available)acquirePointer(sources[h-1]);if(result[i]!=UINT32_MAX)acquirePointer(copies.at(result[i]-1000).element);}image.verify();}
 void snapshot(std::ostream&o){put(o,uint32_t(copies.size()));for(const auto&c:copies){auto p=c.element;auto a=reinterpret_cast<void*>(uintptr_t(word(p,0x64)));need(word(a,4)==reinterpret_cast<uintptr_t>(p+0x60),"copied member backed by destination");put(o,c.source);put(o,c.handle);put(o,word(p,0x38));put(o,word(p,0x60)==UINT32_MAX?4:nameId(reinterpret_cast<const char*>(uintptr_t(word(p,0x60)))));auto r=reinterpret_cast<Mat*>(uintptr_t(word(p,0x5c)));put(o,r?r->id:0);put(o,word(a,0x10)&0xffff);auto copyAttr=reinterpret_cast<void*>(uintptr_t(word(p,0x30)));if(!std::strcmp(reinterpret_cast<const char*>(uintptr_t(word(copyAttr,0x14))),"$probe")){put(o,1);const auto text=word(reinterpret_cast<void*>(uintptr_t(word(copyAttr,4))));put(o,text==UINT32_MAX?4:nameId(reinterpret_cast<const char*>(uintptr_t(text))));put(o,word(copyAttr,0x10)&0xffff);auto sourceExtra=reinterpret_cast<void*>(uintptr_t(word(sources[c.source-1],0x30)));need(copyAttr!=sourceExtra&&word(copyAttr,4)!=word(sourceExtra,4),"copied dynamic string has distinct attribute/storage");}else{put(o,0);put(o,4);put(o,0);}}}
 void finishCase(){image.verify();for(unsigned i=0;i<objectCount;++i){auto a=reinterpret_cast<uint8_t*>(uintptr_t(word(poolObjects[i].data(),0x30)));while(a){if(!(word(a,0x10)&0x200)){auto p=reinterpret_cast<void*>(uintptr_t(word(a,4)));reinterpret_cast<void(__thiscall*)(void*,void*)>(0x103643d0)(&type5,p);}a=reinterpret_cast<uint8_t*>(uintptr_t(word(a)));}}need(allocations.empty(),"all temporary allocations and explicitly freed dynamic strings balanced");}
};
#else
struct OwnedElement{MaterialElementBindingCandidate binding;uint32_t source=0,handle=0,file=0,text=4,extraFlags=0;bool extra=false;};
struct BuilderFixture:MaterialSessionBackupConstructionServicesCandidate,MaterialElementBindingServicesCandidate{
 std::array<OwnedElement,2>sources{};std::vector<OwnedElement>copies;
 explicit BuilderFixture(const char*p){need(!std::ifstream(p).good(),"owned must not read engine");}
 void resetCase(){sources={};copies.clear();for(unsigned i=0;i<sourceCount;++i){auto&s=sources[i];const auto&q=specs[i];s.source=i+1;s.handle=i+1;s.file=q.file;if(q.name<4)need(setMaterialElementNameCandidate(s.binding,names[q.name],0),"source name");s.extra=q.extra!=0;s.text=q.extra?q.text:4;s.extraFlags=q.extra?(q.text==4?5u:0x4085u):0;}}
 uintptr_t resolve(uint32_t h)override{const auto r=h&&h<=sourceCount&&specs[h-1].available?h:0;emit(0,h,r);return r;}
 bool isElement(uintptr_t)override{return true;}
 std::optional<uintptr_t>copyElement(uintptr_t p,uint32_t mode)override{need(mode==2,"copy mode");const auto&q=specs[p-1];if(q.mode==1){emit(1,uint32_t(p),UINT32_MAX);emit(0,UINT32_MAX,0);return uintptr_t(0);}OwnedElement c;c.source=uint32_t(p);c.handle=1000+uint32_t(copies.size());c.file=q.file;emit(1,uint32_t(p),c.handle);emit(0,c.handle,q.mode==2?0:c.handle);if(q.mode!=2){const auto&s=sources[p-1];if(!s.binding.invalidName)need(setMaterialElementNameCandidate(c.binding,*s.binding.binding.name,0),"copy member name");c.extra=s.extra;c.text=s.text;c.extraFlags=s.extraFlags;}copies.push_back(std::move(c));return q.mode==2?uintptr_t(0):uintptr_t(copies.size());}
 bool isMaterial(uintptr_t)override{return true;}
 bool setFileId(uintptr_t p,uint32_t file,uint32_t mode,uint8_t same)override{need(file==UINT32_MAX&&mode==2&&!same,"metadata arguments");copies[p-1].file=file;return true;}
 uint32_t handle(uintptr_t p)override{return copies[p-1].handle;}
 std::optional<uintptr_t>findMaterial(std::string_view s)override{return lookup(std::string(s).c_str());}
 void addReference(uintptr_t p)override{retain(uint32_t(p));}
 void releaseReference(uintptr_t p)override{releaseRef(uint32_t(p));}
 void acquire(OwnedElement&e){need(acquireMaterialSessionResourceCandidate(e.binding.binding,*this).has_value(),"known material provider");}
 void warm(){for(unsigned i=0;i<sourceCount;++i)if(specs[i].available)acquire(sources[i]);}
 void build(const std::vector<uint32_t>&handles,uint32_t type,std::vector<uint32_t>&result){emit(0,900,modelPresent?900:0);need(constructMaterialSessionBackupsCandidate(modelPresent&&(type&31)==15?&handles:nullptr,result,*this),"owned joined construction");}
 void acquirePairs(const std::vector<uint32_t>&handles,const std::vector<uint32_t>&result){for(size_t i=0;i<result.size();++i){const auto h=handles[i];if(h&&h<=sourceCount&&specs[h-1].available)acquire(sources[h-1]);if(result[i]!=UINT32_MAX)acquire(copies.at(result[i]-1000));}}
 void snapshot(std::ostream&o){put(o,uint32_t(copies.size()));for(auto&c:copies){put(o,c.source);put(o,c.handle);put(o,c.file);put(o,c.binding.invalidName?4:nameId(c.binding.binding.name->c_str()));put(o,uint32_t(c.binding.binding.resource));put(o,c.binding.nameFlags);put(o,c.extra);put(o,c.text);put(o,c.extraFlags);}}
 void finishCase(){}
};
#endif
int main(int argc,char**argv){try{need(argc==4,"private-or-absent-image query fresh-output");need(!std::ifstream(argv[3]).good(),"fresh output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"IO");
#ifdef SFM64_NATIVE_ELEMENT_BINDING
 static_assert(sizeof(void*)==4,"private reference x86");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&fault);
#else
 static_assert(sizeof(void*)==8,"owned x64");
#endif
 BuilderFixture fixture(argv[1]);const auto count=get(in);need(count&&count<=10000,"case count");put(out,count);uint32_t created=0;
 for(unsigned ci=0;ci<count;++ci){sourceCount=get(in);const auto nh=get(in),repeats=get(in),type=get(in),warm=get(in);modelPresent=get(in);lookupMode=get(in);need(sourceCount<=2&&nh<=12&&repeats&&repeats<=2&&warm<2&&modelPresent<2&&lookupMode<2,"case extent");for(unsigned i=0;i<sourceCount;++i){auto&s=specs[i];s.available=get(in);s.mode=get(in);s.name=get(in);s.file=get(in);s.extra=get(in);s.text=get(in);need(s.available<2&&s.mode<3&&s.name<=4&&s.extra<2&&s.text<=4,"source extent");}std::vector<uint32_t>handles(nh);for(auto&h:handles)h=get(in);trace.clear();events.clear();refs.fill(0);fixture.resetCase();if(warm)fixture.warm();for(unsigned pass=0;pass<repeats;++pass){std::vector<uint32_t>result;fixture.build(handles,type,result);put(out,uint32_t(result.size()));for(auto h:result)put(out,h);fixture.snapshot(out);fixture.acquirePairs(handles,result);fixture.snapshot(out);}created+=uint32_t(fixture.copies.size());for(auto r:refs)put(out,r);put(out,uint32_t(trace.size()));for(auto e:trace){put(out,e[0]);put(out,e[1]);put(out,e[2]);}put(out,uint32_t(events.size()));for(auto e:events){put(out,e[0]);put(out,e[1]);}fixture.finishCase();}
 need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"output flush");
#ifdef SFM64_NATIVE_ELEMENT_BINDING
 need(enters==leaves&&allocations.empty(),"profile/heap balance");std::cout<<"profile_pairs="<<enters<<'\n';
#endif
 std::cout<<"PASS backup_factory_cases="<<count<<" created_roots="<<created<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
