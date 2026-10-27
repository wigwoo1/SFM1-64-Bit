#include "studio_proxy_prebind_selection_candidate.h"
#include <fstream>
#include <iostream>
#include <array>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <new>
static int64_t failAfter=-1;
void*operator new(size_t n){if(failAfter>=0&&failAfter--==0)throw std::bad_alloc();if(auto*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete(void*p,size_t)noexcept{std::free(p);}
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
using Input=StudioProxyPrebindInputCandidate;using Plan=StudioProxyPrebindPlanCandidate;
static Input fixture(const std::array<uint32_t,9>&q){
 Input in;in.contextSkin=q[1]&1?1:0;std::memcpy(&in.drawSkin,&q[5],4);std::memcpy(&in.body,&q[7],4);in.textureCount=8;in.lod=q[6];in.skinFamilies={{0,1,2,3,4,5,6,7},{7,6,5,4,3,2,1,0}};in.lodMaterials.resize(2);
 for(unsigned lod=0;lod<2;++lod)for(unsigned i=0;i<8;++i){auto id=8*lod+((q[1]&2)&&i==2?0:i);in.lodMaterials[lod].push_back({uintptr_t(id+1),bool((q[3]>>(id%8))&1)});}
 for(unsigned b=0;b<q[2];++b){StudioProxyPrebindBodyCandidate body;body.models.push_back({b?std::vector<int32_t>{2,3,7,3}:std::vector<int32_t>{0,1,0,2}});in.bodies.push_back(std::move(body));}
 if(q[0]){in.forcedMaterial=StudioProxyPrebindMaterialCandidate{17,bool(q[3]&1)};in.forcedType=int32_t(q[0]-1);}return in;
}
static int wire(const char*query,const char*destination){
 need(!std::ifstream(destination).good(),"fresh output required");std::ifstream in(query,std::ios::binary);std::ofstream out(destination,std::ios::binary);need(bool(in)&&bool(out),"wire IO");std::array<uint32_t,9>q{};unsigned count=0,events=0;
 while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){need(q[0]<=2&&q[2]<=2&&q[6]<=1,"checked query shape");auto input=fixture(q);Plan plan;need(studioProxyPrebindSelectionCandidate(input,plan),"owned selection");put(out,count);put(out,q[2]);unsigned n=0;for(const auto&v:plan.visits)n+=1+unsigned(v.bind);put(out,n);for(const auto&v:plan.visits){put(out,0);put(out,uint32_t(v.material-1));put(out,uint32_t(v.bind));if(v.bind){put(out,1);put(out,uint32_t(v.material-1));put(out,q[4]);}}events+=n;++count;}
 need(in.eof()&&!in.gcount()&&count,"query framing");out.flush();need(bool(out),"wire complete");std::cout<<"PASS owned_prebind_cases="<<count<<" events="<<events<<'\n';return 0;
}
static int safety(){
 unsigned checks=0;auto check=[&](bool b,const char*s){need(b,s);++checks;};auto input=fixture({0,2,2,255,0,0,0,3,0});Plan out;out.visits={{123,0x123456789abcdef0ull,true}};
 auto unchanged=[&](){return out.visits.size()==1&&out.visits[0].textureIndex==123u&&out.visits[0].material==0x123456789abcdef0ull&&out.visits[0].bind;};
 auto refuse=[&](const Input&in){check(!studioProxyPrebindSelectionCandidate(in,out)&&unchanged(),"invalid preparation preserves complete old plan");};
 {auto bad=input;bad.lod=2;refuse(bad);bad=input;bad.skinFamilies.clear();refuse(bad);bad=input;bad.textureCount=32769;refuse(bad);bad=input;bad.bodies[0].base=0;refuse(bad);bad=input;bad.bodies[0].models.clear();refuse(bad);bad=input;bad.bodies[0].models[0].meshSkinReferences[0]=-1;refuse(bad);bad=input;bad.skinFamilies[0][0]=-1;refuse(bad);bad=input;bad.skinFamilies[0][0]=8;refuse(bad);bad=input;bad.lodMaterials[0].clear();refuse(bad);bad=input;bad.lodMaterials[0][0].identity=0;refuse(bad);bad=input;bad.lodMaterials[0][0].hasProxy.reset();refuse(bad);}
 unsigned failures=0;bool complete=false;for(int64_t i=0;i<64;++i){failAfter=i;bool ok=studioProxyPrebindSelectionCandidate(input,out);failAfter=-1;if(ok){complete=true;break;}++failures;check(unchanged(),"allocation refusal preserves complete old plan");}check(complete&&failures>1,"actual allocation sweep completed");
 check(out.visits.size()==5&&out.visits[2].material==out.visits[0].material&&out.visits[2].textureIndex!=out.visits[0].textureIndex,"different texture indices sharing material are both visited");
 {auto body=input;body.bodies.resize(1);body.bodies[0].base=2;body.bodies[0].models={{{0}},{{7}}};for(int b=0;b<8;++b){body.body=b;check(studioProxyPrebindSelectionCandidate(body,out)&&out.visits.size()==1&&out.visits[0].textureIndex==uint32_t((b/2)%2?7:0),"bodygroup base and model selection");}body.body=-2;out.visits={{123,0x123456789abcdef0ull,true}};refuse(body);}
 {auto forced=input;forced.forcedMaterial=StudioProxyPrebindMaterialCandidate{99,true};forced.lodMaterials.clear();forced.skinFamilies.clear();forced.bodies.clear();check(studioProxyPrebindSelectionCandidate(forced,out)&&out.visits.size()==1&&!out.visits[0].textureIndex&&out.visits[0].material==99,"forced material ignores model metadata");forced.forcedType=-1;forced.forcedMaterial->hasProxy.reset();forced.forcedMaterial->identity=0;check(studioProxyPrebindSelectionCandidate(forced,out)&&out.visits.empty(),"nonzero override type suppresses before material query");}
 {auto known=input;known.contextSkin=0;known.drawSkin=999;check(studioProxyPrebindSelectionCandidate(known,out)&&out.visits.front().textureIndex==0u,"invalid family selects family zero");known.contextSkin=1;check(studioProxyPrebindSelectionCandidate(known,out)&&out.visits.front().textureIndex==7u,"positive context skin overrides draw skin");known.contextSkin=-1;known.drawSkin=0;check(studioProxyPrebindSelectionCandidate(known,out)&&out.visits.front().textureIndex==0u,"nonpositive context skin uses draw skin");}
 {auto empty=input;empty.bodies.clear();check(studioProxyPrebindSelectionCandidate(empty,out)&&out.visits.empty(),"empty body list produces empty plan");}
 {auto wide=input;wide.lodMaterials[0][0].identity=0x123456789abcdef0ull;check(studioProxyPrebindSelectionCandidate(wide,out)&&out.visits.front().material==0x123456789abcdef0ull,"successful plan preserves native-width owned identity");}
 {auto unknown=input;unknown.forcedMaterial=StudioProxyPrebindMaterialCandidate{99,{}};out.visits={{123,0x123456789abcdef0ull,true}};refuse(unknown);}
 std::cout<<"PASS owned_prebind_safety_checks="<<checks<<" allocation_failures="<<failures<<" renderer=NOT_CONNECTED clock=NOT_INFERRED\n";return 0;
}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"owned x64 selection");if(argc==1)return safety();need(argc==4&&std::string(argv[1])=="--wire","no args OR --wire query fresh-output");return wire(argv[2],argv[3]);}catch(const std::exception&e){failAfter=-1;std::cerr<<e.what()<<'\n';return 1;}}
