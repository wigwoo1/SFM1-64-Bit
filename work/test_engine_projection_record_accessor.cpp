#include "engine_projection_record_accessor_candidate.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
struct Shadow{uint16_t projectionIndex;};struct Projection{uint32_t identity;};
int main(){std::vector<Shadow> shadows(65536);std::vector<Projection> projections(65536);for(unsigned i=0;i<65536;++i){shadows[i].projectionIndex=uint16_t(i^0x9a37);projections[i].identity=i;}
 for(unsigned i=0;i<65536;++i){auto* a=engineProjectionRecordCandidate(i,shadows.data(),projections.data());auto* b=engineProjectionRecordCandidate(i|0xdead0000,shadows.data(),projections.data());if(a!=b||a!=&projections[i^0x9a37]||a->identity!=(i^0x9a37))std::abort();}
 std::puts("PASS: all65536 handles, remapped projection indices and high-word aliases. Native storage integration pending.");}
