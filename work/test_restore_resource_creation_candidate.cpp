#include "studio_restore_resource_creation_candidate.h"
#include <array>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include "restore_format_lookup_init.h"
static void check(bool b){if(!b)throw std::runtime_error("resource candidate mismatch");}
struct Services{
 StudioRestoreResourceCreationCandidate& state;std::vector<unsigned> events;int pad=0;unsigned modeValue=0,result=0,output=1,creates=0,modeCalls=0;
 bool diagnosticEnabled(){return false;}void diagnostic(){throw std::runtime_error("unexpected diagnostic");}
 int padding(){return pad;}unsigned mode(){++modeCalls;return modeValue;}
 void flush(){events.push_back(1);}void acquireFirst(std::uintptr_t* p){events.push_back(38);*p=11;}
 void acquireExtra(std::uintptr_t* p){events.push_back(18);*p=22;}
 void dimensions(std::uint32_t& w,std::uint32_t& h){events.push_back(3);w=640;h=480;}
 unsigned format(){events.push_back(2);return 0x15;}
 unsigned create(unsigned w,unsigned h,unsigned f,unsigned sample,unsigned quality,std::uintptr_t* p){
  check(w==640+unsigned(pad)*2&&h==480+unsigned(pad)*2);
  if(modeValue==2){check(f==0x71);check(sample==(creates?0:state.sample)&&quality==(creates?0:state.quality));events.push_back(creates?282:281);*p=creates?0xfeed1234u:output;}
  else{check(f==0x15&&sample==state.sample&&quality==state.quality);events.push_back(28);*p=11;}
  ++creates;return result;
 }
 void acquireFinal(std::uintptr_t* p){events.push_back(40);*p=0x2468ace0;}
};
struct PaddedServices:Services{
 unsigned index=0;bool mutation=false;unsigned capturedSample=0,capturedQuality=0;
 void acquireExtra(std::uintptr_t* p){check(p==state.resources+2);events.push_back(18);*p=0xdead4321;}
 unsigned format(){capturedSample=state.sample;capturedQuality=state.quality;events.push_back(2);if(mutation){state.sample=99;state.quality=123;}return restoreFormatInstalledWindow.at(index);}
 unsigned create(unsigned w,unsigned h,unsigned f,unsigned sample,unsigned quality,std::uintptr_t* p){check(w==640+unsigned(pad)*2&&h==480+unsigned(pad)*2&&f==restoreFormatInstalledWindow.at(index)&&sample==capturedSample&&quality==capturedQuality&&p==state.resources);events.push_back(28);*p=0x99887766;return 0x80004005u;}
};
struct BasicServices:Services{
 std::uintptr_t* initial=nullptr;std::uintptr_t* replacement=nullptr;bool mutation=false;
 void acquireFirst(std::uintptr_t* p){check(p==initial);events.push_back(38);*p=0x13579bdf;if(mutation)state.resources=replacement;}
};
int main(){try{static_assert(sizeof(std::uintptr_t)==8,"x64 resource policy");unsigned cases=0;
 for(unsigned result:{0u,1u,0x80004005u})for(unsigned output:{0u,9u}){
  std::array<std::uintptr_t,3> slots={8,0,7};StudioRestoreResourceCreationCandidate state{slots.data(),0,4,5};Services services{state,{},0,2,result,output};
  studioRestoreResourceCreationCandidate(state,services);bool retry=result!=0||output==0;
  std::vector<unsigned> expected={3,1,281};if(retry){expected.push_back(1);expected.push_back(282);}expected.push_back(1);expected.push_back(40);
  check(services.events==expected&&slots[0]==8&&slots[1]==(retry?0xfeed1234u:output)&&slots[2]==7&&state.finalResource==0x2468ace0);++cases;
 }
 for(int pad:{0,7}){std::array<std::uintptr_t,3> slots={0,8,7};StudioRestoreResourceCreationCandidate state{slots.data(),9,4,5};Services services{state};services.pad=pad;
  studioRestoreResourceCreationCandidate(state,services);check(slots[0]==11&&slots[1]==8&&slots[2]==(pad?22u:0u)&&state.finalResource==9);check(services.events==(pad?std::vector<unsigned>{1,18,3,2,1,28}:std::vector<unsigned>{1,38}));++cases;
 }
 std::ifstream records("outputs/native-resource-retry.bin",std::ios::binary);check(bool(records));unsigned compared=0;
 for(unsigned pattern=0;pattern<256;++pattern)for(unsigned result:{0u,1u,0x80004005u})for(unsigned output:{0u,0x11223344u})for(unsigned missing=0;missing<2;++missing){
  std::array<std::uintptr_t,8> slots{};slots.fill(0xaabb0000u+pattern);slots[1]=0;StudioRestoreResourceCreationCandidate state{slots.data(),missing?0u:0x55555555u,pattern*0x01010101u,pattern*0x01010101u};Services services{state,{},0,2,result,output};studioRestoreResourceCreationCandidate(state,services);
  std::array<std::uint32_t,28> actual{},original{};for(unsigned i=0;i<8;++i)actual[i]=static_cast<unsigned>(slots[i]);actual[8]=static_cast<unsigned>(state.finalResource);actual[9]=state.sample;actual[10]=state.quality;actual[11]=static_cast<unsigned>(services.events.size());for(unsigned i=0;i<services.events.size();++i)actual[12+i]=services.events[i];actual[24]=services.modeCalls;actual[26]=services.creates;
  records.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(records.gcount()==sizeof(original));check(actual==original);++compared;
 }
 check(records.peek()==std::char_traits<char>::eof());
 std::ifstream paddedRecords("outputs/native-resource-padded.bin",std::ios::binary);check(bool(paddedRecords));unsigned paddedCompared=0;
 for(unsigned pattern=0;pattern<256;++pattern)for(unsigned padding:{1u,7u})for(unsigned index:{0u,12u,24u})for(unsigned missing=0;missing<2;++missing)for(unsigned mutation=0;mutation<2;++mutation){
  std::array<std::uintptr_t,8> slots{};slots.fill(0xaabb0000u+pattern);slots[0]=0;StudioRestoreResourceCreationCandidate state{slots.data(),missing?0u:0x55555555u,pattern*0x01010101u,pattern*0x01010101u};PaddedServices services{{state},index};services.mutation=mutation!=0;services.pad=static_cast<int>(padding);studioRestoreResourceCreationCandidate(state,services);
  std::array<std::uint32_t,28> actual{},original{};for(unsigned i=0;i<8;++i)actual[i]=static_cast<unsigned>(slots[i]);actual[8]=static_cast<unsigned>(state.finalResource);actual[9]=state.sample;actual[10]=state.quality;actual[11]=static_cast<unsigned>(services.events.size());for(unsigned i=0;i<services.events.size();++i)actual[12+i]=services.events[i];actual[24]=services.modeCalls;
  paddedRecords.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(paddedRecords.gcount()==sizeof(original));check(actual==original);++paddedCompared;
 }
 check(paddedRecords.peek()==std::char_traits<char>::eof());
 std::ifstream basicRecords("outputs/native-resource-basic.bin",std::ios::binary);check(bool(basicRecords));unsigned basicCompared=0;
 for(unsigned pattern=0;pattern<256;++pattern)for(unsigned mode=0;mode<3;++mode)for(unsigned negative=0;negative<2;++negative)for(unsigned missing=0;missing<4;++missing)for(unsigned mutation=0;mutation<2;++mutation){
  std::array<std::uintptr_t,8> slots{},replacement{};slots.fill(0x12340000u+pattern);replacement.fill(0xabcd0000u+pattern);if(missing&1)slots[0]=0;
  StudioRestoreResourceCreationCandidate state{slots.data(),(missing&2)?0u:0x76540000u+pattern,pattern*0x01010101u,pattern*0x01010101u};BasicServices services{{state},slots.data(),replacement.data(),mutation!=0};services.modeValue=mode;services.pad=negative?-1:0;studioRestoreResourceCreationCandidate(state,services);
  std::array<std::uint32_t,36> actual{},original{};for(unsigned i=0;i<8;++i){actual[i]=static_cast<unsigned>(slots[i]);actual[8+i]=static_cast<unsigned>(replacement[i]);}actual[16]=static_cast<unsigned>(state.finalResource);actual[17]=state.sample;actual[18]=state.quality;actual[19]=state.resources==replacement.data();actual[20]=static_cast<unsigned>(services.events.size());for(unsigned i=0;i<services.events.size();++i)actual[21+i]=services.events[i];actual[34]=services.modeCalls;
  basicRecords.read(reinterpret_cast<char*>(original.data()),sizeof(original));check(basicRecords.gcount()==sizeof(original));check(actual==original);++basicCompared;
 }
 check(basicRecords.peek()==std::char_traits<char>::eof());std::cout<<"x64_resource_candidate_cases="<<cases<<" original_retry_records_matched="<<compared<<" original_padded_records_matched="<<paddedCompared<<" original_basic_records_matched="<<basicCompared<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}


