#include "physics_support_preparation_test_fixture.h"
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("support preparation assertion");++checks;};PhysicsSupportPreparationInputFixture f;
    f.matrix={1,0,0,1000,0,1,0,-2000,0,0,1,3000};f.cache.groups=3;f.cache.storage.resize(12,{{91,92,93}});f.metadata={true,2,5,2,{}};
    const float nan=std::numeric_limits<float>::quiet_NaN();for(unsigned i=0;i<8;++i)f.points.push_back({float(i),float(i*2),float(i*3),nan});f.direction={1,0,0};const auto old=f.cache;
    auto result=preparationInvokeOwned(f);require(result.built&&result.cache.groups==2);for(unsigned i=0;i<8;++i){const unsigned raw=i<5?i+2:6;require(result.cache.storage[i]==std::array<float,3>{float(raw),float(raw*2),float(raw*3)});}require(result.selected.feature==4&&result.selected.point==std::array<float,3>{6,12,18});require(std::memcmp(result.cache.storage.data()+8,old.storage.data()+8,48)==0);
    f.matrix[3]=nan;f.matrix[7]=nan;f.matrix[11]=nan;result=preparationInvokeOwned(f);require(result.built&&result.selected.point==std::array<float,3>{6,12,18});
    f.metadata={true,4,5,1,{2,2,3}};result=preparationInvokeOwned(f);const std::array<unsigned,8>indices{1,2,5,6,7,7,7,7};for(unsigned i=0;i<8;++i)require(result.cache.storage[i][0]==float(indices[i]));require(result.selected.feature==4&&result.selected.point==std::array<float,3>{7,14,21});
    auto unchanged=[](const auto&a,const auto&b){return a.groups==b.groups&&a.storage.size()==b.storage.size()&&std::memcmp(a.storage.data(),b.storage.data(),a.storage.size()*12)==0;};
    for(unsigned condition=0;condition<3;++condition){auto disabled=f;if(condition==0)disabled.metadata.present=false;else if(condition==1)disabled.metadata.flags=1;else disabled.blocked=true;disabled.points.clear();disabled.metadata.count=0;disabled.matrix.fill(nan);const auto before=disabled.cache;require(!sourcePhysicsPreparePackedSupportCandidate(disabled.metadata,disabled.blocked,disabled.points,disabled.matrix,disabled.cache)&&unchanged(before,disabled.cache));}
    auto refuse=[&](PhysicsSupportPreparationInputFixture bad){const auto before=bad.cache;bool rejected=false;try{sourcePhysicsPreparePackedSupportCandidate(bad.metadata,bad.blocked,bad.points,bad.matrix,bad.cache);}catch(const std::runtime_error&){rejected=true;}require(rejected&&unchanged(before,bad.cache));};
    auto bad=f;bad.metadata.count=0;refuse(bad);bad=f;bad.metadata.runs={2,2};refuse(bad);bad=f;bad.metadata.runs={2,255,3};refuse(bad);bad=f;bad.metadata.count=4;refuse(bad);bad=f;bad.cache.storage.resize(4);refuse(bad);bad=f;bad.matrix[0]=nan;refuse(bad);bad=f;bad.points[5][0]=nan;refuse(bad);bad=f;bad.matrix[0]=std::numeric_limits<float>::max();refuse(bad);
    std::cout<<"PASS support preparation analytical/transaction checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
