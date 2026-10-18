#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "session_mutation_dispatch.h"
int main(int argc,char**argv){try{
 if(argc!=2)return 1;SessionRuntime r(loadGraph(argv[1]));MutationCoordinator m;SessionMutationDispatch d{r,m};
 if(r.graph.elements.size()<3)throw std::runtime_error("fixture too small");
 auto stale=r.elements.graphHandles[0],live=r.elements.graphHandles[1],later=r.elements.graphHandles[2];
 auto&dead=m.owners[stale];dead.state=3;dead.attributeFlags[7]=0x80;dead.callbacks.prepared={{1,7}};
 auto&active=m.owners[live];active.state=3;active.attributeFlags[8]=0x80;active.callbacks.prepared={{2,8}};
 m.queues.marked={stale,live};m.queues.prepared={stale,live};
 if(!r.elements.retire(stale))throw std::runtime_error("retire failed");
 auto replacement=r.elements.bind(r.graph,0);
 if(replacement==stale||d.live(stale)||!d.live(replacement))throw std::runtime_error("generation resolution failed");
 unsigned marked=0,prepared=0,general=0;
 d.processMarked([&](MutationOwner* o){if(o!=&active)throw std::runtime_error("stale mark dispatched");++marked;});
 d.processPrepared([&](uintptr_t,uintptr_t,uint32_t){++general;},[&](uintptr_t cb,uintptr_t attr){if(cb!=2||attr!=8)throw std::runtime_error("stale prepared dispatched");++prepared;});
 if(general||marked!=1||prepared!=1||dead.state!=3||dead.attributeFlags[7]!=0x80||dead.callbacks.prepared.size()!=1||active.state!=2||active.attributeFlags[8]!=0||!m.queues.marked.empty()||!m.queues.prepared.empty())throw std::runtime_error("live drain/reset mismatch");
 auto&next=m.owners[later];next.state=3;m.queues.marked={live,later};
 d.processMarked([&](MutationOwner* o){if(o!=&active)throw std::runtime_error("retired later entry dispatched");r.elements.retire(later);});
 if(next.state!=3||!m.queues.marked.empty())throw std::runtime_error("per-entry lookup mismatch");
 dead.state=0xff;dead.mask=0xffffffff;active.state=0xff;active.mask=0xabcdef03;
 m.queues.propagated={stale,live,later};d.processPropagated();
 if(dead.state!=0xff||dead.mask!=0xffffffff||active.state!=0xef||active.mask!=0xabcdef00||!m.queues.propagated.empty())throw std::runtime_error("propagated live reset mismatch");
 for(unsigned raw=0;raw<256;++raw)for(unsigned low=0;low<4;++low){uint8_t state=uint8_t(raw);uint32_t mask=0xabcdef00|low;clearElementPropagation(state,mask);if(state!=(raw&0xef)||mask!=0xabcdef00)throw std::runtime_error("propagation bit preservation mismatch");}
 active.state=0x13;active.mask=3;active.attributeFlags[8]=0x80;active.callbacks.prepared={{2,8}};
 m.queues.marked={live};m.queues.prepared={live};m.queues.propagated={live};d.listeners={0x123456789abcdef0ull};std::vector<int> order;
 d.drain([&](MutationOwner* o){if(o!=&active||o->state!=0x13)throw std::runtime_error("ordered mark mismatch");order.push_back(1);},
 [&](uintptr_t,uintptr_t,uint32_t){++general;},
 [&](uintptr_t cb,uintptr_t attr){if(cb!=2||attr!=8||active.state!=0x12||active.mask!=3||active.attributeFlags[8])throw std::runtime_error("ordered prepared mismatch");order.push_back(2);d.addListener(9);},
 [&](uintptr_t listener){if(active.state!=0x12||active.mask!=3)throw std::runtime_error("propagation reset too early");if(listener==0x123456789abcdef0ull){order.push_back(3);d.addListener(10);}else if(listener==9)order.push_back(4);else throw std::runtime_error("listener append visited too early");});
 if(order!=std::vector<int>{1,2,3,4}||active.state!=2||active.mask||d.listeners.size()!=3)throw std::runtime_error("drain stage ordering mismatch");
 unsigned listenerCalls=0;d.drain([](MutationOwner*){},[](uintptr_t,uintptr_t,uint32_t){},[](uintptr_t,uintptr_t){},[&](uintptr_t){++listenerCalls;});
 if(listenerCalls!=3)throw std::runtime_error("persistent listener mismatch");
 d.listeners.clear();d.addListener(0x123456789abcdef0ull);d.addListener(9);d.addListener(0x123456789abcdef0ull);
 std::vector<uintptr_t> delivered;d.drain([](MutationOwner*){},[](uintptr_t,uintptr_t,uint32_t){},[](uintptr_t,uintptr_t){},[&](uintptr_t listener){delivered.push_back(listener);});
 if(delivered!=d.listeners||delivered.size()!=3||delivered[0]!=delivered[2])throw std::runtime_error("duplicate listener registration changed order");
 d.evaluationListeners={41,42};std::vector<uintptr_t> evaluated;
 d.evaluate([&](uintptr_t listener){evaluated.push_back(listener);if(listener==41)d.evaluationListeners.push_back(43);});
 if(evaluated!=std::vector<uintptr_t>{41,42}||d.listeners!=delivered)throw std::runtime_error("evaluation listener separation mismatch");
 evaluated.clear();d.evaluate([&](uintptr_t listener){evaluated.push_back(listener);});
 if(evaluated!=std::vector<uintptr_t>{41,42,43})throw std::runtime_error("evaluation append timing mismatch");
 active.state=3;m.queues.marked={live};d.remaining([](uintptr_t,uintptr_t,uint32_t){},[](uintptr_t,uintptr_t){},[](uintptr_t){});
 if(active.state!=3||m.queues.marked!=std::vector<uint32_t>{live})throw std::runtime_error("remaining stage consumed marked queue");
 m.queues.marked.clear();d.listeners={9};d.evaluationListeners={41};m.cycle.phase=0;std::vector<int> cycleEvents;
 auto cleanup=[&](){if(m.cycle.phase!=1)throw std::runtime_error("cleanup phase mismatch");cycleEvents.push_back(1);};
 auto build=[&](){if(m.cycle.phase!=3)throw std::runtime_error("build phase mismatch");cycleEvents.push_back(3);};
 auto eval=[&](uintptr_t id){if(id!=41||m.cycle.phase!=4)throw std::runtime_error("evaluation phase mismatch");cycleEvents.push_back(4);m.queues.mark(active.state,live);};
 auto notify=[&](MutationOwner* o){if(o!=&active||m.cycle.phase!=5)throw std::runtime_error("cycle notify mismatch");cycleEvents.push_back(5);};
 auto listener=[&](uintptr_t id){if(id!=9||(m.cycle.phase!=2&&m.cycle.phase!=5))throw std::runtime_error("cycle listener mismatch");cycleEvents.push_back(m.cycle.phase);};
 auto finish=[&](){if(m.cycle.phase!=5||active.state&1||!m.queues.marked.empty())throw std::runtime_error("finish phase mismatch");cycleEvents.push_back(7);};
 active.state=0;d.fullCycle(cleanup,build,eval,notify,[](uintptr_t,uintptr_t,uint32_t){},[](uintptr_t,uintptr_t){},listener,finish);
 if(cycleEvents!=std::vector<int>{1,2,3,4,5,5,7}||m.cycle.phase!=6)throw std::runtime_error("first cycle ordering mismatch");
 cycleEvents.clear();d.fullCycle(cleanup,build,eval,notify,[](uintptr_t,uintptr_t,uint32_t){},[](uintptr_t,uintptr_t){},listener,finish);
 if(cycleEvents!=std::vector<int>{3,4,5,5,7}||m.cycle.phase!=6)throw std::runtime_error("subsequent cycle ordering mismatch");
 std::cout<<"session_full_cycle_checks=2 evaluation_stage_checks=3 listener_registration_checks=1 ordered_drain_checks=2 propagation_bit_checks=1024 session_mutation_dispatch_checks=4 stale_generation_checks=1\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
