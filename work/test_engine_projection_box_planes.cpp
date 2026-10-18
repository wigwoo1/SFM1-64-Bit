#include <initializer_list>
#include "engine_projection_box_planes_candidate.h"
#include "engine_projection_generate_candidate.h"
#include <cstdio>
#include <cstdlib>
int main(){float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};EngineProjectionPlanesCandidate planes;engineProjectionGeneratePlanesCandidate(identity,planes);unsigned cases=0;
 for(int ax=0;ax<9;++ax)for(int bx=ax;bx<9;++bx)for(int ay=0;ay<9;++ay)for(int by=ay;by<9;++by)for(int az=0;az<9;++az)for(int bz=az;bz<9;++bz){float low[3]={ax*.25f-.5f,ay*.25f-.5f,az*.25f-.5f},high[3]={bx*.25f-.5f,by*.25f-.5f,bz*.25f-.5f};bool expected=false;for(int k=0;k<3;++k)expected|=high[k]<0||low[k]>1;if(engineProjectionBoxOutsidePlanesCandidate(planes,low,high)!=expected)std::abort();++cases;}
 std::printf("PASS: %u independent unit-cube box rejection cases including tangencies and point boxes. Native parity pending.\n",cases);
}
