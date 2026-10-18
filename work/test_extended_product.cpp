#include "extended_product.h"
#include <iostream>
struct Fixture{ExtendedLimbs left,right;ExtendedProduct expected;};
int main(){Fixture fixtures[]={
#include "extended_product_fixtures.inc"
};
 size_t count=0;for(auto&f:fixtures){if(extendedPartialProduct(f.left,f.right)!=f.expected){std::cerr<<"Mismatch "<<count<<'\n';return 1;}++count;}
 std::cout<<count<<" extended partial-product fixtures passed\n";
}
