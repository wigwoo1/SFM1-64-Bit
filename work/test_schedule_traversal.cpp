#include "schedule_traversal.h"
#include <iostream>
#include <random>
static ScheduleResult reference(const std::vector<ScheduleNode>&nodes,const std::vector<uint32_t>&roots){
 ScheduleResult out;std::vector<unsigned char> state(nodes.size());
 auto visit=[&](auto&&self,uint32_t i)->void{if(state[i]){out.cycle|=state[i]==1;return;}state[i]=1;
  for(const auto&group:nodes[i].edgeGroups)for(auto edge:group)self(self,edge);out.order.push_back(nodes[i].operation);state[i]=2;};
 for(auto root:roots)visit(visit,root);std::reverse(out.order.begin(),out.order.end());return out;
}
int main(){try{
 std::vector<ScheduleNode> n={{10,{{1,2}}},{20,{{3}}},{30,{{3}}},{40,{}}};auto r=traverseSchedule(n,{0});if(r.cycle||r.order!=std::vector<uint32_t>{10,30,20,40})throw std::runtime_error("diamond order mismatch");
 n[3].edgeGroups={{0}};r=traverseSchedule(n,{0});if(!r.cycle||r.order!=std::vector<uint32_t>{10,30,20,40})throw std::runtime_error("cycle continuation mismatch");r=traverseSchedule({},{});if(r.cycle||!r.order.empty())throw std::runtime_error("empty mismatch");
 std::mt19937 rng(99850);for(size_t trial=0;trial<2000;++trial){n.assign(1+rng()%40,{});std::vector<uint32_t> roots;
  for(size_t i=0;i<n.size();++i){n[i].operation=static_cast<uint32_t>(i+100);const size_t groups=rng()%5;
   for(size_t j=0;j<groups;++j){std::vector<uint32_t> edges;const size_t count=rng()%5;
    for(size_t k=0;k<count;++k)edges.push_back(static_cast<uint32_t>(rng()%n.size()));n[i].edgeGroups.push_back(edges);}}
  const size_t count=rng()%50;for(size_t i=0;i<count;++i)roots.push_back(static_cast<uint32_t>(rng()%n.size()));
  const auto expected=reference(n,roots);r=traverseSchedule(n,roots);if(r.cycle!=expected.cycle||r.order!=expected.order)throw std::runtime_error("grouped graph reference mismatch");}
 n.assign(100000,{});for(uint32_t i=0;i<n.size();++i){n[i].operation=i;n[i].edgeGroups={{}};if(i+1<n.size())n[i].edgeGroups.push_back({i+1});n[i].edgeGroups.push_back({});}
 r=traverseSchedule(n,{0});if(r.cycle||r.order.size()!=n.size())throw std::runtime_error("deep grouped graph mismatch");
 for(size_t i=0;i<n.size();++i)if(r.order[i]!=i)throw std::runtime_error("deep grouped order mismatch");
 n.back().edgeGroups.push_back({0});r=traverseSchedule(n,{0});if(!r.cycle||r.order.size()!=n.size())throw std::runtime_error("deep grouped cycle mismatch");
 std::cout<<"scheduler_cases=3 randomized_grouped_graphs=2000 deep_chain_and_cycle_nodes=100000 passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
