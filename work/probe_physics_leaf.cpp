#include "physics_leaf_test_fixture.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned AMD64");if(argc!=3)throw std::runtime_error("input output required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary),metrics(std::string(argv[2])+".stats",std::ios::binary);if(!in||!out||!metrics)throw std::runtime_error("leaf streams");PhysicsLeafInputFixture f;size_t cases=0;
    while(leafReadInput(in,f)){std::ostringstream before(std::ios::binary);leafWriteInput(before,f);auto state=f.state;auto cache=f.shape.cache;SourcePhysicsSearchStatisticsCandidate stats;const auto code=sourcePhysicsLeafCandidate(state,f.contents,f.mask,f.box,f.shape.context,cache,&stats);leafWriteOutput(out,state,code,cache,f.prepared&&f.shape.context.mode==0);supportPut(metrics,stats);std::ostringstream after(std::ios::binary);leafWriteInput(after,f);if(before.str()!=after.str())throw std::runtime_error("leaf input mutation");++cases;}
    if(!cases)throw std::runtime_error("no leaf cases");out.flush();metrics.flush();if(!out||!metrics)throw std::runtime_error("leaf write");std::cout<<"PASS owned leaf/search/hit calls="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
