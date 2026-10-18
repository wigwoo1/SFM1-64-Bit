#include "attribute_mutation_gate.h"
#include <iostream>
#include <vector>
#include <stdexcept>
int main(){try{unsigned checks=0;
 for(unsigned raw=0;raw<65536;++raw)for(int phase=-1;phase<=6;++phase){uint16_t flags=uint16_t(raw);std::vector<int> events;
  bool expected=(raw&0x20)==0&&(phase==0||phase==1||phase==2||(phase==4&&(raw&0x400)==0));
  bool actual=beginAttributeMutation(flags,phase,[&](){if(flags!=raw)throw std::runtime_error("prepare saw changed flags");events.push_back(0);},[&](){if(flags!=(raw|0x4080))throw std::runtime_error("mark saw old flags");events.push_back(1);});
  std::vector<int> wanted;if(expected){if(!(raw&0x80)&&(raw&0x2000))wanted.push_back(0);wanted.push_back(1);}
  if(actual!=expected||flags!=(expected?(raw|0x4080):raw)||events!=wanted)throw std::runtime_error("mutation gate mismatch");++checks;
 }
 std::cout<<"mutation_flag_phase_checks="<<checks<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
