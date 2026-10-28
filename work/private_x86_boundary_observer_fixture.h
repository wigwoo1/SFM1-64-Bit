#pragma once
// Test-only execution boundaries on THIS disposable probe thread. No remote
// processes/threads, software code patches, DLL initialization or privileges.
// Four hardware execution breakpoints replace explicitly declared callees with
// observers. The original caller still executes; intercepted callees DO NOT.
// Include windows.h,array,cstdint,stdexcept before this x86-only header.
#ifdef _WIN64
#error x86 reference fixture only
#endif
struct PrivateX86BoundaryObserverFixture {
 using Observer=void(*)(unsigned,CONTEXT&);
 inline static PrivateX86BoundaryObserverFixture*active=nullptr;
 static constexpr DWORD configureCode=0xe0426f51;
 std::array<uint32_t,4>entries{};Observer observe=nullptr;void*handler=nullptr;
 std::array<DWORD,6>saved{};bool armed=false,captured=false;uint32_t hits=0;
 static LONG CALLBACK dispatch(EXCEPTION_POINTERS*p){
  auto*f=active;if(!f)return EXCEPTION_CONTINUE_SEARCH;auto&c=*p->ContextRecord;
  if(p->ExceptionRecord->ExceptionCode==configureCode&&p->ExceptionRecord->NumberParameters==1&&p->ExceptionRecord->ExceptionInformation[0]==reinterpret_cast<ULONG_PTR>(f)){
   if(f->armed){if(!f->captured){f->saved={c.Dr0,c.Dr1,c.Dr2,c.Dr3,c.Dr6,c.Dr7};f->captured=true;}c.Dr0=f->entries[0];c.Dr1=f->entries[1];c.Dr2=f->entries[2];c.Dr3=f->entries[3];c.Dr6=0;c.Dr7=0x55;}
   else if(f->captured){c.Dr0=f->saved[0];c.Dr1=f->saved[1];c.Dr2=f->saved[2];c.Dr3=f->saved[3];c.Dr6=f->saved[4];c.Dr7=f->saved[5];}
   c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;return EXCEPTION_CONTINUE_EXECUTION;
  }
  if(f->armed&&p->ExceptionRecord->ExceptionCode==EXCEPTION_SINGLE_STEP){
   for(unsigned i=0;i<4;++i)if((c.Dr6&(1u<<i))&&c.Eip==f->entries[i]){
    try{f->observe(i,c);}catch(const std::exception&e){std::cerr<<"OBSERVER_FAILURE: "<<e.what()<<'\n';ExitProcess(92);}catch(...){ExitProcess(93);}
    ++f->hits;c.Dr6=0;return EXCEPTION_CONTINUE_EXECUTION;
   }
  }
  return EXCEPTION_CONTINUE_SEARCH;
 }
 PrivateX86BoundaryObserverFixture(std::array<uint32_t,4>addresses,Observer observer):entries(addresses),observe(observer){
  if(active||!observe)throw std::runtime_error("exclusive probe observer");active=this;handler=AddVectoredExceptionHandler(1,&dispatch);if(!handler){active=nullptr;throw std::runtime_error("probe observer registration");}
 }
 void configure(bool enable){armed=enable;ULONG_PTR arg=reinterpret_cast<ULONG_PTR>(this);RaiseException(configureCode,0,1,&arg);}
 static void returnCdecl(CONTEXT&c,uint32_t result=0){auto*stack=reinterpret_cast<const uint32_t*>(c.Esp);c.Eip=stack[0];c.Esp+=4;c.Eax=result;}
 ~PrivateX86BoundaryObserverFixture(){if(armed)configure(false);if(handler)RemoveVectoredExceptionHandler(handler);active=nullptr;}
};
