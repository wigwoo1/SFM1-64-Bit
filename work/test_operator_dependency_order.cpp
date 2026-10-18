#include "operator_dependency_order.h"
#include <iostream>
#include <random>
static OperatorDependencyOrder reference(const std::vector<OperatorDependencyNode>&nodes,const std::vector<size_t>&roots){
 OperatorDependencyOrder out;std::vector<unsigned char> state(nodes.size());
 auto visit=[&](auto&& self,size_t i)->void{if(state[i]){out.cycle|=state[i]==1;return;}
  state[i]=1;for(size_t edge:nodes[i].edges)self(self,edge);out.identities.push_back(nodes[i].identity);state[i]=2;};
 for(size_t root:roots)visit(visit,root);std::reverse(out.identities.begin(),out.identities.end());return out;
}
int main(){try{
 std::vector<OperatorDependencyNode> n{{0x123456789abcdef0ull,{1,2}},{2,{3}},{3,{3}},{4,{}}};
 auto a=orderOperatorDependencies(n,{0,0});if(a.cycle||a.identities!=std::vector<uintptr_t>{n[0].identity,3,2,4})throw std::runtime_error("diamond ordering mismatch");
 n[3].edges={0};a=orderOperatorDependencies(n,{0});if(!a.cycle||a.identities.size()!=4)throw std::runtime_error("cycle handling mismatch");
 a=orderOperatorDependencies(n,{});if(a.cycle||!a.identities.empty())throw std::runtime_error("empty roots mismatch");
 n={{7,{0}}};a=orderOperatorDependencies(n,{0});if(!a.cycle||a.identities!=std::vector<uintptr_t>{7})throw std::runtime_error("self cycle mismatch");
 std::mt19937 rng(99150);for(size_t trial=0;trial<2000;++trial){
  n.assign(1+rng()%40,{});std::vector<size_t> roots;
  for(size_t i=0;i<n.size();++i){n[i].identity=uintptr_t(0x100000000ull)+i;
   const size_t count=rng()%8;for(size_t j=0;j<count;++j)n[i].edges.push_back(rng()%n.size());}
  const size_t count=rng()%50;for(size_t j=0;j<count;++j)roots.push_back(rng()%n.size());
  const auto expected=reference(n,roots);a=orderOperatorDependencies(n,roots);
  if(a.cycle!=expected.cycle||a.identities!=expected.identities)throw std::runtime_error("random graph reference mismatch");
 }
 n.assign(100000,{});for(size_t i=0;i<n.size();++i){n[i].identity=uintptr_t(0x200000000ull)+i;if(i+1<n.size())n[i].edges={i+1};}
 a=orderOperatorDependencies(n,{0});if(a.cycle||a.identities.size()!=n.size())throw std::runtime_error("deep chain mismatch");
 for(size_t i=0;i<n.size();++i)if(a.identities[i]!=n[i].identity)throw std::runtime_error("deep chain order mismatch");
 n.back().edges={0};a=orderOperatorDependencies(n,{0});if(!a.cycle||a.identities.size()!=n.size())throw std::runtime_error("deep cycle mismatch");
 try{orderOperatorDependencies(n,{n.size()});throw std::logic_error("invalid root accepted");}catch(const std::runtime_error&){}
 n[0].edges={n.size()};try{orderOperatorDependencies(n,{0});throw std::logic_error("invalid edge accepted");}catch(const std::runtime_error&){}
 std::cout<<"dependency_order_checks=4 random_graphs=2000 deep_nodes=100000 deep_cycle=passed invalid_indices=passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
