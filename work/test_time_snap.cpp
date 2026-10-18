#include "recovered_time_snap_candidate.h"
#include <iostream>
int main(){
 unsigned checked=0;
 for(int32_t t=-1000000;t<=1000000;t+=7){
  // Original negative adjustment shifts the 25fps boundary by one tick.
  const int32_t frame=t<0?-((-int64_t(t)+199)/400):((int64_t(t)+200)/400);
  if(recoveredTimeSnapCandidate(t,25u|(10000u<<16))!=frame*400)throw std::runtime_error("exact grid snap mismatch");
  ++checked;
 }
 std::cout<<"exact_25fps_snap_cases="<<checked<<"\n";
}
