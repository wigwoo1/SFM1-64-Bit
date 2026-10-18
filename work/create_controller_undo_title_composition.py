from pathlib import Path
p=Path('work/test_controller_window_title.cpp').read_text()
p='#include "controller_undo_listener_candidate.h"\n'+p
p=p.replace('struct Child{};','struct Child{unsigned char state=0;};')
p=p.replace('unsigned modified=0;','unsigned modified=0;unsigned char flag=0;std::uint64_t timestamp=0;')
p=p.replace('return 128;','return child.state;')
p=p.replace('    void* window()', '''    Child* globalChild344(){return globalChild();}
    auto childByte15(Child* c){return c->state;}
    void setChildByte15(Child* c,unsigned char v){c->state=v;}
    void invokeGlobalControllerChange(std::uint32_t){updateControllerWindowTitleCandidate(*this);}
    void setListenerByte219(unsigned char v){flag=v;}
    std::uint64_t readTimestampCounter(){assert(flag==1);return 0x123456789abcdef0ull;}
    void setListenerTimestamp228(std::uint64_t v){timestamp=v;}
    void* window()''')
p=p[:p.index('int main()')]+'''int main(){
    Adapter a;controllerUndoListenerCandidate(a);
    assert(a.modified==1 && a.title=="session[*] - Source Filmmaker [Beta]" && a.flag==1 && a.timestamp==0x123456789abcdef0ull);
    assert((a.events==std::vector<int>{1,2,3,4,4,5,5,6}));
    a.events.clear();controllerUndoListenerCandidate(a);assert(a.events.empty() && a.flag==1);
    Adapter b;b.hasWindow=false;controllerUndoListenerCandidate(b);assert(b.child.state==1 && b.events.empty() && b.flag==1);
    Adapter c;c.hasChild=false;controllerUndoListenerCandidate(c);assert(c.events.empty() && c.flag==1 && c.timestamp);
    std::cout<<"x64 controller undo-to-title composition: changed updates title/unchanged suppression/missing window or child/timestamp passed\\n";
}
'''
Path('work/test_controller_undo_title_composition.cpp').write_text(p)
