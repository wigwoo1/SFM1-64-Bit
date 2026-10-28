#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include "private_x86_boundary_observer_fixture.h"
static volatile uint32_t bodies=0;
__declspec(noinline) static uint32_t __cdecl a(uint32_t x){++bodies;return x+1;}
__declspec(noinline) static uint32_t __cdecl b(uint32_t x){++bodies;return x+2;}
__declspec(noinline) static uint32_t __cdecl c(uint32_t x){++bodies;return x+3;}
__declspec(noinline) static uint32_t __cdecl d(uint32_t x){++bodies;return x+4;}
static void observer(unsigned id,CONTEXT&context){auto*stack=reinterpret_cast<const uint32_t*>(context.Esp);if(stack[1]!=101+id)throw std::runtime_error("stack framing");PrivateX86BoundaryObserverFixture::returnCdecl(context,201+id);}
int main(){try{PrivateX86BoundaryObserverFixture fixture({uint32_t(reinterpret_cast<uintptr_t>(&a)),uint32_t(reinterpret_cast<uintptr_t>(&b)),uint32_t(reinterpret_cast<uintptr_t>(&c)),uint32_t(reinterpret_cast<uintptr_t>(&d))},&observer);fixture.configure(true);auto x=a(101),y=b(102),z=c(103),w=d(104);fixture.configure(false);if(x!=201||y!=202||z!=203||w!=204||bodies||fixture.hits!=4)return 1;if(a(5)!=6||bodies!=1)return 2;std::cout<<"PASS private_thread_boundaries=4 callee_bodies_skipped=4 caller_stack=INTACT disarmed_original_body=PASS\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 3;}}
