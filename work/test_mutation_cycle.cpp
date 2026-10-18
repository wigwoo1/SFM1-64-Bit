#include "mutation_cycle.h"
#include <iostream>
#include <vector>
#include <stdexcept>
int main(){try{MutationCycle cycle;std::vector<int> phases;
 auto record=[&](){phases.push_back(cycle.phase);bool expected=cycle.phase==1||cycle.phase==2||cycle.phase==4;if(cycle.permits(0)!=expected||cycle.permits(0x20)||cycle.permits(0x400)!=(cycle.phase==1||cycle.phase==2))throw std::runtime_error("phase permission mismatch");};
 cycle.fullCycle(record,record,record,record,record,record,record);
 if(phases!=std::vector<int>{1,2,3,4,5,5,5}||cycle.phase!=6||cycle.permits(0))throw std::runtime_error("full phase order mismatch");
 phases.clear();cycle.fullCycle(record,record,record,record,record,record,record);
 if(phases!=std::vector<int>{3,4,5,5,5}||cycle.phase!=6)throw std::runtime_error("nonzero phase initial processing mismatch");
 cycle.shortCycle([&](){throw std::runtime_error("nonzero short cleanup ran");},[&](){throw std::runtime_error("nonzero short initial ran");});
 if(cycle.phase!=0||!cycle.permits(0))throw std::runtime_error("short reset mismatch");
 phases.clear();cycle.shortCycle(record,record);if(phases!=std::vector<int>{1,2}||cycle.phase!=0)throw std::runtime_error("short phase order mismatch");
 std::cout<<"mutation_cycle_checks=4\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
