#include "model_render_record_assignment_candidate.h"
#include <cstdio>
#include <cstdlib>
void check(bool x){if(!x)std::abort();}
int main(){unsigned cases=0;for(unsigned flags=0;flags<65536;++flags)for(unsigned mode=0;mode<4;++mode){ModelRenderRecordAssignmentCandidate record{uint16_t(flags),0xa5a5a5a5};uint32_t value=flags*65537u;unsigned reads=0;bool invalid=(mode&2)!=0;modelRenderRecordAssignmentCandidate(invalid?0xabcdffffu:0xabcd1234u,mode&1?&value:nullptr,[&](uint16_t index)->ModelRenderRecordAssignmentCandidate&{check(index==0x1234);++reads;return record;});check(reads==unsigned(!invalid));check(record.flags==(invalid?uint16_t(flags):mode&1?uint16_t(flags|1):uint16_t(flags&0xfffe)));check(record.value==(invalid?0xa5a5a5a5u:mode&1?value:0));++cases;}
 std::printf("PASS: %u assignment states, all16-bit flags, null/non-null supplied value, invalid-index skip/high aliases and preserved unrelated bits. Native parity pending.\n",cases);
}
