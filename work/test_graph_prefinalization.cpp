#include "graph_prefinalization_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{int collection=1,state=2,child=3,element=4;bool hasCollection=true,hasChild=true;int count=3;std::vector<int> events;
 void beginImportedScope(){events.push_back(1);}int* ownerCollection14(){return hasCollection?&collection:nullptr;}int collectionCount24(int*){return 2;}
 void releaseRecordNested18(int*,int i){events.push_back(10+i);}void clearOwnerCollection14(){hasCollection=false;events.push_back(2);}
 int* currentGraphState(){return &state;}int stateCount84(int*){return count;}unsigned stateHandle78(int*,int i){events.push_back(20+i);return i;}
 int* resolveModelSlot18(unsigned h){return h==1?nullptr:&element;}unsigned filterElementSlot4(int*){return 1;}
 void refresh10741340(int*){count=0;events.push_back(3);}int* liveChild344(){return hasChild?&child:nullptr;}void updateChild10275f50(int*){events.push_back(4);}void endImportedScope(){events.push_back(5);}
};
int main(){Adapter a;graphPrefinalizationCandidate(a);assert((a.events==std::vector<int>{1,10,11,2,20,3,21,22,3,4,5}));Adapter b;b.hasCollection=false;b.hasChild=false;b.count=-1;graphPrefinalizationCandidate(b);assert((b.events==std::vector<int>{1,5}));std::cout<<"x64 graph prefinalization: snapshot bounds, record release, handle filtering, child gate and scope order passed\n";}
