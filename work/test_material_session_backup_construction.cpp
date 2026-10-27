#include "material_session_backup_construction_candidate.h"
#include <string>
#include <iostream>
#include <stdexcept>
struct BackupConstructionSafety:MaterialSessionBackupConstructionServicesCandidate{
 std::vector<std::string>events;unsigned resolutions=0;uintptr_t absent=0,notElement=0;std::optional<uintptr_t>copy=uintptr_t(7);bool material=true,detach=true,unstable=false,throwCopy=false;uint32_t nextHandle=50;
 uintptr_t resolve(uint32_t h)override{++resolutions;events.push_back("r"+std::to_string(h));return h==absent||(unstable&&resolutions==2)?0:h;}
 bool isElement(uintptr_t p)override{events.push_back("e"+std::to_string(p));return p!=notElement;}
 std::optional<uintptr_t>copyElement(uintptr_t p,uint32_t mode)override{if(mode!=2)throw std::runtime_error("Wrong clone mode");events.push_back("c"+std::to_string(p));if(throwCopy)throw std::runtime_error("Explicit copy failure");return copy;}
 bool isMaterial(uintptr_t p)override{events.push_back("m"+std::to_string(p));return material;}
 bool setFileId(uintptr_t p,uint32_t file,uint32_t mode,uint8_t same)override{if(file!=UINT32_MAX||mode!=2||same)throw std::runtime_error("Wrong detach contract");events.push_back("d"+std::to_string(p));return detach;}
 uint32_t handle(uintptr_t p)override{events.push_back("h"+std::to_string(p));return nextHandle++;}
};
int main(){try{
 static_assert(sizeof(void*)==8,"Owned construction safety x64");unsigned checks=0;auto check=[&](bool b){if(!b)throw std::runtime_error("Backup construction assertion "+std::to_string(checks));++checks;};
 std::vector<uint32_t>source{1},result{99};BackupConstructionSafety s;
 check(constructMaterialSessionBackupsCandidate(nullptr,result,s)&&result.empty()&&s.events.empty());
 s={};result={99};source.assign(4097,1);check(!constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({99})&&s.events.empty());
 s={};source={1};result={99};check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({50})&&s.events==std::vector<std::string>({"r1","e1","r1","e1","c1","m7","d7","h7"}));
 s={};source={1,1};check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({50,51}));
 s={};source={0,1};check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({UINT32_MAX,50}));
 s={};s.notElement=1;source={1};check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({UINT32_MAX})&&s.events==std::vector<std::string>({"r1","e1"}));
 s={};s.copy=uintptr_t(0);check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({UINT32_MAX})&&s.events.back()=="c1");
 s={};s.material=false;check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({UINT32_MAX})&&s.events.back()=="m7");
 s={};s.copy.reset();result={99};check(!constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({99}));
 s={};s.detach=false;result={99};check(!constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({99})&&s.events.back()=="d7");
 s={};s.unstable=true;result={99};check(!constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({99})&&s.events==std::vector<std::string>({"r1","e1","r1"}));
 s={};s.throwCopy=true;result={99};bool threw=false;try{constructMaterialSessionBackupsCandidate(&source,result,s);}catch(const std::runtime_error&){threw=true;}check(threw&&result==std::vector<uint32_t>({99}));
 s={};source={1};constructMaterialSessionBackupsCandidate(&source,result,s);check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result==std::vector<uint32_t>({51}));
 s={};source.clear();result={99};check(constructMaterialSessionBackupsCandidate(&source,result,s)&&result.empty()&&s.events.empty());
 std::cout<<"PASS backup_construction_safety_checks="<<checks<<" created_element_lifetimes=SERVICE_OWNED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
