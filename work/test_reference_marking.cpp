#include "reference_marking.h"
#include <iostream>
int main(){std::vector<MarkReferenceNode> n(4);n[0].edges={{1,1},{2,uint16_t(15|0x1000)},{3,uint16_t(1|0x800)}};n[1].edges={{0,1}};
 markReferenceGraph(n,0,true,1,0);if(n[0].marks!=1||n[1].marks!=1||n[2].marks||n[3].marks!=1)return 1;
 markReferenceGraph(n,0,false,1,0);for(auto&x:n)if(x.marks)return 2;
 markReferenceGraph(n,0,true,2,1);if(n[0].marks!=2||n[1].marks||n[2].marks||n[3].marks!=2)return 3;
 markReferenceGraph(n,0,true,0,1);if(n[1].marks||n[2].marks)return 4; // Already marked root short-circuits even with changed mode.
 markReferenceGraph(n,1,true,3,0);if(n[1].marks!=1||n[0].marks!=2)return 5;
 std::cout<<"reference_graph_marking_checks=5\n";}
