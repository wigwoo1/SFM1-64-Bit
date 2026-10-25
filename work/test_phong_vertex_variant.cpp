#include "phong_vertex_variant_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");auto require=[](bool b,const char*s){if(!b)throw std::runtime_error(s);};unsigned checks=0,refusals=0;
    auto defaults=phongVertexStaticDefaultsCandidate(20,92,92,0);require(phongVs20StaticArgumentCandidate(defaults,true)==72,"VS20 explicit native defaults select static3");++checks;
    defaults=phongVertexStaticDefaultsCandidate(30,92,0,0);require(phongVs30StaticArgumentCandidate(defaults,false,false)==192,"VS30 explicit native defaults select static4");++checks;
    require(phongVs20DynamicIndexCandidate(0,65,0,true,0)==2&&phongVs30DynamicIndexCandidate(1,65,false,0,0)==3,"skinned stream profile");++checks;
    require(phongVs30DynamicIndexCandidate(1,65,true,0,0)==7,"morph bit changes profile");++checks;
    require(phongVs30DynamicIndexCandidate(1,65,false,1,0)==8,"tessellation disables compression/skinning");++checks;
    require(phongVs20DynamicIndexCandidate(1,-1,400,true,2)==13,"negative bones, static light-count suppression and fog");++checks;
    auto reject=[&](auto action){try{action();}catch(const std::invalid_argument&){++refusals;return;}throw std::runtime_error("invalid variant admitted");};
    reject([&]{phongVertexStaticDefaultsCandidate(21,92,92,0);});reject([&]{phongVs20StaticArgumentCandidate({0,0,2,0},false);});reject([&]{phongVs30StaticArgumentCandidate({0,0,0,1},false,false);});
    reject([&]{phongVs20DynamicIndexCandidate(2,1,0,false,0);});reject([&]{phongVs20DynamicIndexCandidate(0,1,3,false,0);});reject([&]{phongVs30DynamicIndexCandidate(0,1,false,3,0);});reject([&]{phongVs30DynamicIndexCandidate(2,1,false,0,0);});
    require(argc==2,"selector fixture required");std::ifstream in(argv[1],std::ios::binary);require(bool(in),"selector fixture open");std::array<uint32_t,8>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),32)){
        uint32_t value=0;int32_t signedB=0,signedE=0;std::memcpy(&signedB,&q[2],4);std::memcpy(&signedE,&q[5],4);require(q[7]==0,"selector padding");
        if(q[0]==0)value=phongVs20StaticArgumentCandidate({0,0,q[1],q[2]},q[3]!=0);
        else if(q[0]==1)value=phongVs30StaticArgumentCandidate({0,0,q[1],0},q[2]!=0,q[3]!=0);
        else if(q[0]==2)value=phongVs20DynamicIndexCandidate(q[1],signedB,q[3],q[4]!=0,signedE);
        else if(q[0]==3)value=phongVs30DynamicIndexCandidate(q[1],signedB,q[3]!=0,q[4],signedE);
        else throw std::runtime_error("selector kind");
        require(value==q[6],"byte-interpreted selector arithmetic differs");++cases;
    }
    require(in.eof()&&!in.gcount()&&cases,"selector fixture framing");std::cout<<"PASS checks="<<checks<<" refusals="<<refusals<<" selector_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
