#include "studio_gamma_startup_candidate.h"
#include <cmath>
#include <cstdio>
struct Ops {
 unsigned floats=0,doubles=0,truncFloats=0,truncDoubles=0,rounds=0;
 float powerFloat(float a,float b){++floats;return std::pow(a,b);}
 double powerDouble(double a,double b){++doubles;return std::pow(a,b);}
 int truncateFloat(float a){++truncFloats;return static_cast<int>(a);}
 int truncateDouble(double a){++truncDoubles;return static_cast<int>(a);}
 int roundFloat(float a){++rounds;return static_cast<int>(std::nearbyint(a));}
};
int main(){
 StudioGammaTablesCandidate t;Ops o;
 studioGammaStartupCandidate(t,2.2f,2.2f,0.0f,2,o);
 if(o.floats!=1536||o.doubles!=5632||o.truncFloats!=1280||o.truncDoubles!=1024||o.rounds!=4096)return 1;
 if(t.textureGamma.front()!=0||t.textureGamma.back()!=255||t.linearToTexture.front()!=0||t.linearToTexture.back()!=255||t.linearToVertex.front()!=0||t.linearToLightmap.front()!=0)return 2;
 std::puts("gamma_startup_candidate_provider_counts_and_endpoints=passed native_parity=unverified");
}
