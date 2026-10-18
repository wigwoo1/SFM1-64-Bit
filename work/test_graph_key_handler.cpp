#include "graph_key_handler_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Event{unsigned char flags;};struct Point{int x,y;};
struct Adapter{std::vector<int> calls;
 unsigned eventFlags1c(Event* e){return e->flags;}
 Point cursorPosition(){calls.push_back(1);return {500,-10};}
 Point mapFromGlobal(Point p){calls.push_back(2);return {p.x-200,p.y-30};}
 unsigned eventModifiers(Event*){calls.push_back(3);return 0x06000000;}
 unsigned mouseButtons(){calls.push_back(4);return 3;}
 void dispatch100d35b0(Point p,unsigned m,unsigned b,unsigned z){calls.push_back(5);assert(p.x==300 && p.y==-40 && m==0x06000000 && b==3 && z==0);}
};
int main(){for(unsigned flags=0;flags<256;++flags){Adapter a;Event e{static_cast<unsigned char>(flags)};graphKeyHandlerCandidate(&e,a);if(flags&1)assert(a.calls.empty());else assert((a.calls==std::vector<int>{1,2,3,4,5}));}std::cout<<"x64 graph key adapter: all256 flag values, coordinate mapping and modifier/button argument order passed\n";}
