#include "operator_attribute_registration.h"
#include <iostream>
#include <stdexcept>
int main(){try{OperatorAttributeRegistration r;uintptr_t a=0x123456789abcdef0ull;r.add(1,{a,a},{9});r.add(2,{9},{a,a});
 if(r.attributes.size()!=2||r.operators.size()!=2||r.attributes[0].identity!=a||!r.attributes[0].excluded||!r.attributes[1].excluded||r.attributes[0].associatedOperators!=std::vector<size_t>{0,0}||r.attributes[1].associatedOperators!=std::vector<size_t>{1}||r.operators[1].attributeEdges!=std::vector<size_t>{0,0})throw std::runtime_error("registration mismatch");
 std::cout<<"attribute_registration_checks=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
