#include "undo_retained_target_assignment.h"
#include "undo_reference_target_counts.h"
#include "reference_record_transfer.h"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{static_assert(sizeof(void*)==8,"x64 required");if(argc!=2)throw std::runtime_error("oracle required");std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("oracle unavailable");
 for(unsigned seed=0;seed<784;++seed){unsigned old=seed%7,incoming=seed/7%7;uint32_t value=old==6?0xffffffffu:old;std::array<uint32_t,64>out{},expected{};unsigned cursor=0;uint8_t state=0xa5;std::array<RecoveredTransferReferenceRecordCandidate,5>records{};const uint32_t counts[]={0,1,1023,17};for(auto&r:records){r.counts=0xc0000456u|(counts[seed/49%4]<<20);r.flags=0xfe;}auto emit=[&](uint32_t c,uint32_t h){out[cursor++]=c;out[cursor++]=h;out[cursor++]=value;};auto resolve=[&](uint32_t h){emit(2,h);return h<4?uint64_t(h)+0x100000001ull:0ull;};auto mutate=[&](uint32_t h,bool release){mutateRecoveredUndoTargetCountCandidate(h,release,state,[&](uint32_t target){emit(1,target);return target<5?&records[target]:nullptr;},[&](uint32_t target){return !resolve(target)||(seed/392%2)!=0;});};
 assignRecoveredUndoRetainedTargetCandidate(value,incoming==6?0xffffffffu:incoming,[&](uint32_t h){mutate(h,true);},resolve,[&](uint64_t target){auto h=uint32_t(target-0x100000001ull);emit(3,h);return !((seed/196%2)&&(h&1));},[&](uint32_t h){mutate(h,false);});for(unsigned h=0;h<5;++h){out[48+h*2]=records[h].counts;out[49+h*2]=records[h].flags;}out[58]=value;out[59]=state;out[60]=cursor;input.read(reinterpret_cast<char*>(expected.data()),sizeof(expected));if(!input||out!=expected)throw std::runtime_error("native mismatch");
 }if(input.peek()!=EOF)throw std::runtime_error("trailing oracle");std::cout<<"x64_retained_assignment_cases=784 native_order_counts_filter_flags=matched\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
