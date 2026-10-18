#include "string_attribute_undo_replay_candidate.h"
#include "string_attribute_update_candidate.h"
#include "test_owner_symbol_composition_fixture.h"
#include <iostream>
struct Fixture {
    struct Owner{} owner;
    struct Attribute {std::uintptr_t name,symbol;std::uint16_t flags=5;} attribute;
    OwnerSymbolCompositionFixture symbols;
    bool stale=false,missing=false;int currentPhase=0;
    unsigned writes=0,propagations=0,modelMask=0;
    Fixture(){attribute.name=symbols.intern("name");attribute.symbol=symbols.intern("old model name");}
    Owner* resolveUndoOwner(std::uint32_t handle){assert(handle==42);return stale?nullptr:&owner;}
    Attribute* findUndoAttribute(Owner* p,const char* name){assert(p==&owner);return missing || std::strcmp(name,"name")?nullptr:&attribute;}
    auto flags(Attribute* p){return p->flags;}
    void updateUndoAttribute(Attribute* p,const std::uintptr_t* source){updateStringAttributeCandidate(p,source,*this);}
    void initializeStringCompareDefault(){}
    std::uintptr_t stringCompareDefault(){return ~std::uintptr_t{0};}
    auto stringSymbol(Attribute* p){return p->symbol;}
    int phase(){return currentPhase;}
    void prepareCallbacks(Attribute*){assert(false);}
    void setFlags(Attribute* p,std::uint16_t f){p->flags=f;}
    void markOwner(Attribute*){}
    bool shouldRecordChange(Attribute*){return false;} // explicit replay fixture scope
    void* createStringChangeRecord(Attribute*,const std::uintptr_t*){assert(false);return nullptr;}
    void submitStringChangeRecord(void*){assert(false);}
    void setStringSymbol(Attribute* p,std::uintptr_t s){p->symbol=s;++writes;}
    bool ownerNotificationReady(Attribute*){return false;}
    void notifyOwner(Attribute*){assert(false);}
    void propagateOwner(Attribute*,unsigned mask){assert(mask==2);++propagations;}
    void markModelChange(unsigned mask){modelMask|=mask;}
    std::uint32_t ownerHandle(Attribute*){return 42;}
    auto attributeName(Attribute* p){return p->name;}
};
int main(){
    Fixture f;auto next=f.symbols.intern("new model name");
    auto record=captureStringUndoPayloadCandidate(&f.attribute,&next,f);
    assert(record.attributeName>0xffffffffull && record.oldSymbol>0xffffffffull && record.newSymbol>0xffffffffull);
    replayStringAttributeUndoCandidate(record,true,f);
    assert(f.attribute.symbol==next && f.writes==1 && f.propagations==1 && f.modelMask==0x2000);
    replayStringAttributeUndoCandidate(record,false,f);
    assert(std::strcmp(reinterpret_cast<const char*>(f.attribute.symbol),"old model name")==0 && f.writes==2);
    f.stale=true;replayStringAttributeUndoCandidate(record,true,f);assert(f.writes==2);
    f.stale=false;f.missing=true;replayStringAttributeUndoCandidate(record,true,f);assert(f.writes==2);
    f.missing=false;f.attribute.flags|=0x20;replayStringAttributeUndoCandidate(record,true,f);assert(f.writes==2);
    f.attribute.flags=5;f.currentPhase=3;replayStringAttributeUndoCandidate(record,true,f);assert(f.writes==2);
    f.currentPhase=0;record.attributeName=~std::uintptr_t{0};replayStringAttributeUndoCandidate(record,true,f);assert(f.writes==2);
    std::cout<<"x64 string undo/redo: actual interned pointers/payload/cycle/stale/missing/denied/sentinel passed\n";
}
