#include "graph_editor_selection_clear_candidate.h"
#include <vector>
#include <cassert>
#include <iostream>
struct Adapter{
 std::vector<unsigned> handles{11,22,33};std::vector<unsigned> released;
 bool service=true;unsigned char flag=1;unsigned count=3;
 unsigned count0c(){return count;}unsigned handleAt(int i){return handles[i];}
 bool modelServicePresent(){return service;}
 void releaseElementHandle(unsigned h,int mode){assert(mode==0);released.push_back(h);}
 void setByte28(unsigned char n){flag=n;}void setCount0c(unsigned n){count=n;}
};
int main(){Adapter a;graphEditorSelectionClearCandidate(a);assert((a.released==std::vector<unsigned>{33,22,11}));assert(!a.flag&&!a.count);
 Adapter b;b.service=false;graphEditorSelectionClearCandidate(b);assert(b.released.empty()&&!b.flag&&!b.count);
 Adapter c;c.count=0;c.flag=1;graphEditorSelectionClearCandidate(c);assert(c.released.empty()&&!c.flag&&!c.count);
 std::cout<<"x64 graph selection cleanup: reverse handle release, missing service, empty list and state reset passed\n";
}
