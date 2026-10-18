#include "anchored_document_time_candidate.h"
#include <iostream>
int main(){
 unsigned cases=0;
 for(unsigned flags=0;flags<16;++flags)for(unsigned mode=0;mode<6;++mode)for(bool gate:{false,true}){
  AnchoredDocumentTimeState s(100);s.saved684=201;s.flag6c0=(flags&1)!=0;s.flag69c=(flags&2)!=0;s.flag228=(flags&4)!=0;s.flag2a8=(flags&8)!=0;
  if(mode==1)s.mode47c=-1;else if(mode>=2)s.modes480[mode-2]=2;
  unsigned gates=0,times=0,rates=0;
  auto get=[&]{return anchoredDocumentTimeCandidate(s,[&]{++gates;return gate;},[&]{++times;return 900;},[&]()->std::optional<float>{++rates;return 25.f;});};
  const bool anchored=gate&&mode!=0;const bool snap=!anchored&&flags==1;
  const auto result=get();
  if(result!=(anchored?800:snap?400:201)||gates!=1||times!=unsigned(anchored)||rates!=unsigned(snap))throw std::runtime_error("branch dispatch mismatch");
  if(anchored){s.anchor430=0x80000000u;times=rates=gates=0;if(get()!=201||times||rates||gates!=1)throw std::runtime_error("sentinel dispatch mismatch");}
  ++cases;
 }
 AnchoredDocumentTimeState s(0xffffffffu);s.mode47c=1;
 if(anchoredDocumentTimeCandidate(s,[]{return true;},[]{return int32_t(0x7fffffffu);},[]()->std::optional<float>{throw std::runtime_error("unexpected rate lookup");})!=INT32_MIN)throw std::runtime_error("wrapping subtraction");
 s.mode47c=0;s.saved684=10000;s.flag6c0=true;
 if(anchoredDocumentTimeCandidate(s,[]{return false;},[]()->int32_t{throw std::runtime_error("unexpected engine time");},[]{return std::optional<float>{};})!=10000)throw std::runtime_error("default rate");
 std::cout<<"anchored_document_time_branch_cases="<<cases<<" sentinel_wrapping_default=passed\n";
}
