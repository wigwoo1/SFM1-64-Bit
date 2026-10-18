#include <future>
#include <iostream>
#include "static_prop_registry_test_fixture.h"
#include "source_static_prop_point_spatial_candidate.h"
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64");if(argc!=3)throw std::runtime_error("BSP mounts required");
    auto f=std::make_unique<StaticPropRegistryTestFixture>(argv[1],argv[2]);
    for(auto&m:f->models)m.surfaceProperty.reset();
    auto scene=std::make_shared<const SourceStaticPropPointSpatialCandidate>(f->props,f->models,f->ids);
    size_t checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("spatial composition safety assertion");++checks;};
    require(scene->size()==485&&scene->registeredCount()==145);
    size_t inserted=0,disabled=0,nonsolid=0,first=0;
    for(size_t i=0;i<f->ids.size();++i){const auto&r=scene->registration(f->ids[i]);const auto&disk=f->props.records()[i];const auto b=scene->effectiveBinding(f->ids[i]);
        require(r.entityId==f->ids[i]&&r.diskSolidType==disk.solidType&&&r==&scene->registrationAt(i));
        require(scene->diskLease(r.entityId).binding().solidType==disk.solidType&&b.solidType==r.effectiveSolidType&&b.referenceHandle==(0x40000000u|uint32_t(i))&&b.modelId==f->models[disk.model].modelId);
        require(scene->diskLease(r.entityId).assetOwner()==f->models[disk.model].asset);
        if(r.physicalBounds){if(!inserted)first=i;require(r.partitionHandle==inserted++&&r.effectiveSolidType==6&&r.diskSolidType==6);}
        else{require(r.partitionHandle==65535&&r.effectiveSolidType==0);if(r.diskSolidType==6){++disabled;require(scene->diskLease(r.entityId).assetOwner()->missingInConfiguredLooseMounts());}else ++nonsolid;}
    }
    require(inserted==145&&disabled==1&&nonsolid==339);
    const auto box=*scene->registrationAt(first).physicalBounds;SourceWorldLightRayCandidate ray;for(unsigned j=0;j<3;++j)ray.start[j]=(box[j]+box[j+3])*.5f;ray.swept=false;
    const auto baseline=scene->enumerateCandidates(ray);require(!baseline.empty()&&std::find(baseline.begin(),baseline.end(),f->ids[first])!=baseline.end());
    for(uint64_t id:baseline)require(scene->registration(id).effectiveSolidType==6&&scene->diskLease(id).assetOwner()->sharedShape()!=nullptr);
    const auto*owner=scene->diskLease(f->ids[first]).assetOwner().get();
    auto reject=[&](auto&&fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}require(caught);require(scene->enumerateCandidates(ray)==baseline&&scene->diskLease(f->ids[first]).assetOwner().get()==owner);};
    reject([&](){scene->registration(0);});reject([&](){scene->registration(0xdecafbadu);});reject([&](){scene->diskLease(0);});reject([&](){scene->effectiveBinding(0);});reject([&](){scene->registrationAt(scene->size());});
    // Failed replacement cannot publish partial registry, state or partition.
    {auto ids=f->ids;ids[1]=ids[0];reject([&](){scene=std::make_shared<const SourceStaticPropPointSpatialCandidate>(f->props,f->models,ids);});}
    {auto models=f->models;models[0].asset.reset();reject([&](){scene=std::make_shared<const SourceStaticPropPointSpatialCandidate>(f->props,models,f->ids);});}
    {auto ids=f->ids;ids.pop_back();reject([&](){scene=std::make_shared<const SourceStaticPropPointSpatialCandidate>(f->props,f->models,ids);});}
    // Validly parsed solid2 is an explicit unsupported path, never guessed box.
    {auto raw=readBinaryAssetCandidate(argv[1]);auto word=[&](size_t p){uint32_t v;std::memcpy(&v,raw.data()+p,4);return v;};const size_t game=word(568);size_t offset=0;for(size_t i=0;i<word(game);++i)if(word(game+4+i*16)==0x73707270u)offset=word(game+4+i*16+8);if(!offset)throw std::runtime_error("safety prop lump missing");size_t p=offset+4+size_t(word(offset))*128;p+=4+size_t(word(p))*2;p+=4;raw[p+first*60+30]=2;SourceBspStaticPropsCandidate unsupported(raw);reject([&](){scene=std::make_shared<const SourceStaticPropPointSpatialCandidate>(unsupported,f->models,f->ids);});}
    // No surface property invented merely to make broadphase preparation work.
    reject([&](){scene->diskLease(f->ids[first]).clip(ray,1,0x12345678decafbadull);});
    for(unsigned axis=0;axis<3;++axis){auto bad=ray;bad.start[axis]=std::numeric_limits<float>::quiet_NaN();reject([&](){scene->enumerateCandidates(bad);});}
    reject([&](){scene->enumerateCandidates(ray,0x10000);});require(scene->enumerateCandidates(ray,0).empty());
    require(scene->enumerateCandidates(ray,0x65,1)==std::vector<uint64_t>(baseline.begin(),baseline.begin()+1));
    // Caller-owned models/IDs/parsed map/fixture registry may all disappear.
    std::weak_ptr<const SourceStudioLooseCollisionAssetCandidate>weak=scene->diskLease(f->ids[first]).assetOwner();f.reset();require(!weak.expired()&&scene->enumerateCandidates(ray)==baseline);
    const auto retained=scene->diskLease(scene->registrationAt(first).entityId);require(retained.assetOwner().get()==owner);
    std::array<std::future<bool>,4>jobs;for(auto&job:jobs)job=std::async(std::launch::async,[&](){for(unsigned i=0;i<128;++i)if(scene->enumerateCandidates(ray)!=baseline)return false;return true;});for(auto&job:jobs)require(job.get());
    // Returned ID values and retained assets remain valid after scene teardown.
    scene.reset();require(!weak.expired()&&retained.assetOwner()->sharedShape()!=nullptr&&baseline.front()>UINT32_MAX);
    std::cout<<"PASS spatial composition safety checks="<<checks<<" concurrent_queries=512 disk339skip/1disabled/145registered unchanged; failed replacement/lifetimes/surface gate preserved\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
