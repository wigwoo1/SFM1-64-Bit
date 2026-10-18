#include "native_container_composition_fixture.h"
int main(){static_assert(sizeof(void*)==8);Adapter a;std::vector<Item> items(4096),replacement(4096);
 for(unsigned i=0;i<items.size();++i){items[i].code=(i+1)<<12;assert(!insertControllerContainerCandidate(&items[i],a));assert(!a.error);if(i%97==0){a.validate();for(unsigned j=0;j<=i;++j)assert(lookupControllerContainerCandidate(&items[j].code,a)==&items[j]);}}
 a.validate();assert(a.doubleCount>=8 && a.splitCount>1000 && a.count==items.size());
 for(unsigned i=0;i<items.size();++i){replacement[i]=items[i];assert(insertControllerContainerCandidate(&replacement[i],a)==&items[i]);assert(lookupControllerContainerCandidate(&items[i].code,a)==&replacement[i]);}a.validate();assert(a.count==items.size() && a.replaced==items.size());
 Item absent{0x70000000};a.failNode=true;assert(!insertControllerContainerCandidate(&absent,a) && a.error==1);a.validate();assert(!lookupControllerContainerCandidate(&absent.code,a));
 Adapter b;std::vector<Item> faultItems(32);for(unsigned i=0;i<30;++i){faultItems[i].code=(i+1)<<12;insertControllerContainerCandidate(&faultItems[i],b);}assert(b.split==7 && b.capacity==16);b.failBuckets=true;faultItems[30].code=31<<12;insertControllerContainerCandidate(&faultItems[30],b);assert(b.error==1 && b.split==0 && b.capacity==16 && b.count==31);
 // Failed doubling resets split while retaining prior split chains. The original
 // failure state is not assumed to preserve lookup reachability for every key.
 std::cout<<"native x64 container composition: 4096 insertions/replacements, repeated growth, bucket invariants, lookup and allocation failures passed\n";
}
