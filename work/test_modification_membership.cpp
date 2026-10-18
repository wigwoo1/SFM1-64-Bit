#include <cstddef>
#include "modification_membership.h"
#include <stdexcept>
#include <iostream>
int main(){try{std::vector<ModificationMember> m;auto alive=[](uint32_t id){return id==10||id==20||id==30;};
 if(addModificationMember(m,10,alive)!=0||m[0].references!=1)throw std::runtime_error("first add");
 if(addModificationMember(m,20,alive)!=1||m.size()!=2)throw std::runtime_error("append");
 m[0].channel=99;
 if(addModificationMember(m,20,alive)!=1||m[1].references!=2||m[0].channel!=99)throw std::runtime_error("existing after vacancy");
 if(addModificationMember(m,30,alive)!=0||m[0].references!=1||m.size()!=2)throw std::runtime_error("reuse");
 if(modificationNeedsOverlay(0)||!modificationNeedsOverlay(1)||modificationNeedsOverlay(2))throw std::runtime_error("layer count guard");
 unsigned cleaned=0;auto cleanup=[&](uint32_t channel){if(channel!=20||m[1].channel!=20)throw std::runtime_error("cleanup ordering");++cleaned;};
 if(releaseModificationMember(m,1,cleanup)||m[1].references!=1||cleaned)throw std::runtime_error("shared release");
 if(!releaseModificationMember(m,1,cleanup)||m[1].references!=0||m[1].channel!=0xffffffff||cleaned!=1)throw std::runtime_error("final release");
 if(releaseModificationMember(m,-1,cleanup)||releaseModificationMember(m,99,cleanup)||cleaned!=1)throw std::runtime_error("invalid release");
 std::cout<<"modification_membership_checks=8\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
