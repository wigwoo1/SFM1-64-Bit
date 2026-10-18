#pragma once
template<class Adapter>void graphBookmarkUndoReplayCandidate(bool redo,Adapter& a){
 auto name=a.savedName14();if(a.invalidName(name))name=a.defaultName10a8a4f0();
 auto* owner=a.resolveModelSlot18(a.savedHandle18());if(!owner)return;
 auto* attribute=a.find1035dd50(owner,name);if(!attribute||(a.type10(attribute)&31)!=21)return;
 auto* captured=a.data4(attribute);a.clear10377310(attribute,captured);
 for(int i=0;i<a.snapshotCount(redo);++i){
  auto* live=a.data4(attribute);auto* value=a.snapshotBase(redo)+i;
  const auto index=a.count0c(captured);a.insert103770a0(attribute,live,index,value);
 }
 if(redo&&a.flag2c())a.finish10377420(attribute,a.data4(attribute));
}
