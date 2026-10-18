#include "registered_operator_order.h"
#include <iostream>
int main(){try{OperatorAttributeRegistration r;r.add(10,{}, {101,102});r.add(20,{101},{103});r.add(30,{102},{103});r.add(40,{103,103},{});
 auto result=orderRegisteredOperators(r,{0});if(result.cycle||result.identities!=std::vector<uintptr_t>{10,30,20,40})throw std::runtime_error("registered diamond ordering mismatch");
 r.add(50,{999},{});result=orderRegisteredOperators(r,{0});if(result.identities.size()!=4)throw std::runtime_error("unreachable operator selected");
 r.operators[3].attributeEdges.push_back(r.attribute(777));r.attributes[r.attribute(777)].associatedOperators.push_back(0);
 result=orderRegisteredOperators(r,{0});if(!result.cycle||result.identities.size()!=4)throw std::runtime_error("registered cycle mismatch");
 bool rejected=false;r.operators[0].attributeEdges.push_back(9999);try{orderRegisteredOperators(r,{0});}catch(const std::runtime_error&){rejected=true;}if(!rejected)throw std::runtime_error("invalid edge accepted");
 std::cout<<"registered_operator_order_checks=4\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
