#include "recovered_type_chain.h"
#include <iostream>
#include <stdexcept>
int main(){try{
 RecoveredTypeChain chain{0x123,0x456,0x789};size_t checks=0;
 for(uint32_t value=0;value<65536;++value){int expected=value==0x123?0:value==0x456?1:value==0x789?2:-1;if(recoveredTypeDepth(chain,value)!=expected||recoveredTypeAccepts(chain,value)!=(expected>=0))throw std::runtime_error("type chain mismatch");++checks;}
 if(!recoveredTypeAccepts(chain,0xffffffff)||recoveredTypeDepth(chain,0xffffffff)!=2)throw std::runtime_error("invalid symbol sentinel mismatch");
 RecoveredTypeChain collision{7,7,7};if(recoveredTypeDepth(collision,7)!=0)throw std::runtime_error("comparison precedence mismatch");
 std::cout<<"type_query_checks="<<checks<<" sentinel_and_precedence_checks=2\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
