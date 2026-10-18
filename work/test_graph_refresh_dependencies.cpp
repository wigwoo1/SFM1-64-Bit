#include "graph_refresh_dependencies_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{int first=1,other=2;int* storage=&first;int count=3;unsigned released=0;std::vector<int> visited;
 void invokeSlot0(int* p,unsigned mode){assert(p==&first && mode==1);storage=&other;++released;}
 int controllerViewCount(){return count;}int* liveControllerView(int i){visited.push_back(i);return i==1?nullptr:&first;}
 void invokeViewSlot3(int* p){assert(p==&first);count=0;}
};
int main(){Adapter a;graphReleaseNestedCandidate(&a.storage,a);assert(!a.storage && a.released==1);graphReleaseNestedCandidate(&a.storage,a);assert(a.released==1);graphUpdateChildViewsCandidate(a);assert((a.visited==std::vector<int>{0,1,2}));std::cout<<"x64 graph cleanup dependencies: callback-mutated pointer cleared, null release and snapshot view loop passed\n";}
