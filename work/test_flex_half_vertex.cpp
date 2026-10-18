#include "mdl_flex_vertex_candidate.h"
#include <fstream>
#include <iostream>
int main(){try{
 std::ofstream reference("outputs/sfm64-flex-half-decoded.bin",std::ios::binary);for(uint32_t word=0;word<65536;++word){uint32_t bits=UINT32_MAX;try{float value=finiteFlexHalfCandidate(uint16_t(word));std::memcpy(&bits,&value,4);}catch(const std::runtime_error&){}reference.write(reinterpret_cast<const char*>(&bits),4);}if(!reference)throw std::runtime_error("half output write failed");
 std::vector<uint8_t>record(16);uint16_t index=2;std::memcpy(record.data(),&index,2);record[2]=255;record[3]=127;std::array<uint16_t,6>words{0x3c00,0xbc00,0x0001,0x8000,0x4000,0xc000};std::memcpy(record.data()+4,words.data(),12);auto vertex=readMdlFlexHalfVertexCandidate(record,0,3);if(vertex.index!=2||vertex.speed!=255||vertex.side!=127||vertex.positionDelta[0]!=1||vertex.positionDelta[1]!=-1||vertex.normalDelta[1]!=2)throw std::runtime_error("vertex mismatch");
 unsigned rejected=0;for(int scenario=0;scenario<4;++scenario){auto bad=record;if(scenario==0)bad.resize(15);if(scenario==1)bad[0]=3;if(scenario==2){bad[4]=0;bad[5]=0x7c;}bool failed=false;try{readMdlFlexHalfVertexCandidate(bad,scenario==3?SIZE_MAX:0,3);}catch(const std::runtime_error&){failed=true;}if(!failed)throw std::runtime_error("invalid flex record accepted");++rejected;}
 std::cout<<"flex half vertex decoder passed malformed_rejected="<<rejected<<" exhaustive_half_reference_written\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
