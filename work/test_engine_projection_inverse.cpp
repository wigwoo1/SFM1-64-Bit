#include "engine_projection_inverse_candidate.h"
#include <emmintrin.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
using Matrix=std::array<float,16>;
void check(bool value){if(!value)std::abort();}
int main(){
 Matrix sentinel;sentinel.fill(123.25f);Matrix out=sentinel;
 Matrix zero{};check(!engineProjectionInverseCandidate(zero.data(),out.data()));check(out==sentinel);
 Matrix identity{};for(int i=0;i<4;++i)identity[i*4+i]=1;
 Matrix singular=identity;singular[15]=0;
 check(!engineProjectionInverseCandidate(singular.data(),out.data()));check(out==sentinel);
 uint32_t bits=0x358637bd;float threshold;std::memcpy(&threshold,&bits,4);
 for(float value:{threshold,-threshold,0.0f}){auto m=identity;m[0]=value;check(!engineProjectionInverseCandidate(m.data(),out.data()));check(out==sentinel);}
 auto m=identity;m[0]=std::nextafter(threshold,1.0f);check(engineProjectionInverseCandidate(m.data(),out.data()));
 unsigned cases=0;double maximumError=0;
 uint32_t random=0x31415926;
 for(unsigned n=0;n<4096;++n){
  Matrix a{};for(int r=0;r<4;++r)for(int c=0;c<4;++c){random=random*1664525u+1013904223u;a[r*4+c]=float(int((random>>16)&255)-128)/128.0f;}
  for(int r=0;r<4;++r)a[r*4+r]+=5.0f;
  // Row permutations require pivot selection and exercise all output row mappings.
  for(int k=0;k<4;++k)std::swap(a[k],a[12+k]);
  Matrix inverse;check(engineProjectionInverseCandidate(a.data(),inverse.data()));
  for(int r=0;r<4;++r)for(int c=0;c<4;++c){double product=0;for(int k=0;k<4;++k)product+=double(a[r*4+k])*inverse[k*4+c];maximumError=std::max(maximumError,std::abs(product-double(r==c)));}
  Matrix aliased=a;check(engineProjectionInverseCandidate(aliased.data(),aliased.data()));check(std::memcmp(aliased.data(),inverse.data(),64)==0);++cases;
 }
 check(maximumError<0.00001);
 std::printf("PASS: %u row-permuted matrix inverse products and bit-exact in-place aliases; maximum residual %.9g\n",cases,maximumError);
 std::puts("PASS: zero/late singular output preservation and strict signed pivot threshold boundary. Native parity and renderer integration not established.");
}
