#include <array>
#include <future>
#include <iostream>
#include "source_surface_name_table_candidate.h"
#include "source_studio_surface_properties_candidate.h"
static std::vector<uint8_t>model(){
    std::vector<uint8_t>b(1024);auto set=[&](size_t p,uint32_t x){std::memcpy(b.data()+p,&x,4);};
    std::memcpy(b.data(),"IDST",4);set(4,48);set(76,1024);set(156,1);set(160,400);set(308,800);set(396,123456789);set(576,420);set(584,987654321);std::memcpy(b.data()+800,"metal",6);std::memcpy(b.data()+820,"wood",5);return b;
}
int main(){try{
    static_assert(sizeof(void*)==8,"owned x64");unsigned checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("surface safety assertion");++checks;};
    auto scripts=std::vector<SourceSurfaceScriptCandidate>{{"one.txt","default {} metal {} wood {}"},{"two.txt","plastic {} metal { density 999 }"},{"ONE.TXT","ignored {}"}};
    auto table=std::make_shared<const SourceSurfaceNameTableCandidate>(scripts);require(table->size()==5&&table->parseCounts()==std::vector<int32_t>{4,5,0});require(table->lookup("default")==0&&table->lookup("Metal")==1&&table->lookup("WOOD")==2&&table->lookup("plastic")==4&&table->lookup("ignored")==-1&&table->lookup("mteal")==-1&&table->lookup("$material_index_shadow")==0xf000);
    auto mdl=model();const auto before=mdl;const auto baseline=sourceStudioResolveSurfacePropertiesCandidate(mdl,*table);require(baseline.model==1&&baseline.bones==std::vector<int32_t>{2}&&baseline.traceSurfaceProperty()==1&&mdl==before);
    auto reject=[&](auto&&fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}require(caught);require(table->lookup("metal")==1&&sourceStudioResolveSurfacePropertiesCandidate(mdl,*table).bones==baseline.bones&&mdl==before);};
    for(const auto&text:std::vector<std::string>{"metal {","metal", "metal { a { b c } }","metal { a }","#include x", "metal {} [$WIN32]", "/* block */ metal {}", "metal { a \"unterminated }", "\"$MATERIAL_INDEX_SHADOW\" {}", "metal { a \"x\\y\" }", "metal { a \"}\" }", "(metal) {}",std::string("metal {}\0",9),std::string(1024,'a')+" {}"})reject([&](){table=std::make_shared<const SourceSurfaceNameTableCandidate>(std::vector<SourceSurfaceScriptCandidate>{{"bad",text}});});
    reject([&](){table=std::make_shared<const SourceSurfaceNameTableCandidate>(std::vector<SourceSurfaceScriptCandidate>{{"","default {}"}});});
    for(size_t offset:{size_t(308),size_t(576),size_t(76),size_t(160)}){auto bad=mdl;const uint32_t x=0xffffffff;std::memcpy(bad.data()+offset,&x,4);reject([&](){sourceStudioResolveSurfacePropertiesCandidate(bad,*table);});}
    {auto bad=mdl;std::fill(bad.begin()+800,bad.end(),uint8_t('x'));reject([&](){sourceStudioResolveSurfacePropertiesCandidate(bad,*table);});}
    {auto unknown=mdl;std::memcpy(unknown.data()+800,"mteal",6);const auto r=sourceStudioResolveSurfacePropertiesCandidate(unknown,*table);require(r.model==-1&&r.traceSurfaceProperty()==65535&&r.bones==baseline.bones);}
    scripts.clear();scripts.shrink_to_fit();require(table->lookup("plastic")==4);std::array<std::future<bool>,4>jobs;for(auto&job:jobs)job=std::async(std::launch::async,[&](){for(unsigned i=0;i<128;++i)if(sourceStudioResolveSurfacePropertiesCandidate(mdl,*table).model!=1)return false;return true;});for(auto&job:jobs)require(job.get());
    std::cout<<"PASS surface provider safety checks="<<checks<<" concurrent_resolutions=512 original bytes/disk caches preserved\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
