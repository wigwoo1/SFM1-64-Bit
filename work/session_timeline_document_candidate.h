#pragma once
#include "session_render_root_candidate.h"
#include "session_document_root_candidate.h"
#include "document_time_candidate.h"
#include "session_timeline_frames_candidate.h"
#include "runtime_undo_journal.h"
#include "atomic_session_save_candidate.h"
#include "float_component_write_sequence.h"
#include "animation_keys.h"
#include <tuple>
#include <functional>
#include <memory>
#include <random>
#include <set>
#include <cmath>
// Prototype document edit layer; native notifications/capture remain separate.
class SessionTimelineDocumentCandidate {
 struct AttributeSnapshot{uint32_t element;size_t attribute;Attribute before;};
 struct RemovedDagRecord{uint32_t element=UINT32_MAX;Element removed;std::vector<AttributeSnapshot> incoming;std::vector<std::pair<std::tuple<uint32_t,size_t,size_t>,Value>> savedAttributes;std::vector<std::pair<uint32_t,std::string>> savedNames;};
 struct AddedBookmarkRecord{uint32_t bookmarkSet=UINT32_MAX;size_t bookmarksAttribute=SIZE_MAX;uint32_t element=UINT32_MAX;size_t originalStringCount=0;std::vector<std::string> addedStrings;Element bookmark;Value reference;bool stringsCommitted=false;};
 struct AddedTrackClipRecord{uint32_t track=UINT32_MAX;size_t childrenAttribute=SIZE_MAX;uint32_t firstElement=UINT32_MAX;size_t originalStringCount=0;std::vector<std::string> addedStrings;std::array<Element,2> elements;Attribute childrenBefore,childrenAfter;bool stringsCommitted=false;};
 struct KeyDeletionRecord{uint32_t element=UINT32_MAX;size_t timesAttribute=SIZE_MAX,valuesAttribute=SIZE_MAX;std::vector<size_t>indices;Attribute timesBefore,valuesBefore;};
 struct TrackClipRemovalRecord{uint32_t track=UINT32_MAX;size_t childrenAttribute=SIZE_MAX;Value clipReference;Attribute before;};
 struct Edit{uint32_t element;size_t attribute;Value before,after;size_t valueIndex=0;bool elementName=false;uint32_t nameBefore=0,nameAfter=0;std::shared_ptr<RemovedDagRecord> removedDag;std::shared_ptr<AddedBookmarkRecord> addedBookmark;std::shared_ptr<AddedTrackClipRecord> addedTrackClip;std::shared_ptr<KeyDeletionRecord> keyDeletion;std::shared_ptr<TrackClipRemovalRecord> trackClipRemoval;};Graph graph_;uint32_t root_,frame_;size_t duration_,savedElementCount_;
 std::map<std::tuple<uint32_t,size_t,size_t>,Value>saved_;
 std::map<uint32_t,std::string>savedNames_;
 std::map<std::pair<uint32_t,size_t>,Attribute>savedArrays_;
 RuntimeUndoJournal<Edit>journal_;
 Attribute&duration(){return graph_.elements.at(frame_).attributes.at(duration_);}
 const Attribute*findAttributeForDocumentCandidate(const Element&element,const char*name)const{for(const auto&attribute:element.attributes)if(graph_.strings.at(attribute.name)==name)return &attribute;return nullptr;}
 bool isTrackClip(uint32_t clip)const{
  if(clip>=graph_.elements.size()||clip==root_)return false;
 const auto&root=graph_.elements.at(root_);
  std::vector<uint32_t>groups=hierarchyRefs(graph_,root,"subClipTrackGroup",1);
  const auto regular=hierarchyRefs(graph_,root,"trackGroups",15);groups.insert(groups.end(),regular.begin(),regular.end());
  for(uint32_t group:groups)for(uint32_t track:hierarchyRefs(graph_,graph_.elements.at(group),"tracks",15))
   for(uint32_t child:hierarchyRefs(graph_,graph_.elements.at(track),"children",15))if(child==clip)return true;
  return false;
 }
 static bool sameValue(const Value&a,const Value&b){return a.wire==b.wire&&a.reference==b.reference&&a.text==b.text;}
 static bool sameAttribute(const Attribute&a,const Attribute&b){if(a.name!=b.name||a.type!=b.type||a.values.size()!=b.values.size())return false;for(size_t i=0;i<a.values.size();++i)if(!sameValue(a.values[i],b.values[i]))return false;return true;}
 void restoreKeyDeletion(const KeyDeletionRecord&record){auto&element=graph_.elements.at(record.element);element.attributes.at(record.timesAttribute)=record.timesBefore;element.attributes.at(record.valuesAttribute)=record.valuesBefore;}
 void applyKeyDeletion(const KeyDeletionRecord&record){auto&element=graph_.elements.at(record.element);auto&times=element.attributes.at(record.timesAttribute);auto&values=element.attributes.at(record.valuesAttribute);if(times.values.size()!=values.values.size())throw std::runtime_error("animation key delete array mismatch");for(auto it=record.indices.rbegin();it!=record.indices.rend();++it){if(*it>=times.values.size())throw std::runtime_error("animation key delete index outside array");times.values.erase(times.values.begin()+*it);values.values.erase(values.values.begin()+*it);}}
 void restoreTrackClipRemoval(const TrackClipRemovalRecord&record){graph_.elements.at(record.track).attributes.at(record.childrenAttribute)=record.before;}
 void applyTrackClipRemoval(const TrackClipRemovalRecord&record){auto&children=graph_.elements.at(record.track).attributes.at(record.childrenAttribute);auto found=std::find_if(children.values.begin(),children.values.end(),[&](const Value&value){return value.reference==record.clipReference.reference;});if(found==children.values.end())throw std::runtime_error("track clip reference missing during redo");children.values.erase(found);}
 bool isDagType(uint32_t element)const{const auto&type=graph_.strings.at(graph_.elements.at(element).type);return type=="DmeDag"||type=="DmeGameModel"||type=="DmeCamera"||type=="DmeLight"||type=="DmeParticleSystem";}
 size_t dagChildCount(uint32_t element)const{size_t count=0;for(const auto&attribute:graph_.elements.at(element).attributes)if(graph_.strings.at(attribute.name)=="children"){if(attribute.type!=15)throw std::runtime_error("DAG children attribute is not an element array");for(const auto&value:attribute.values)if(value.reference!=0xffffffffu&&value.reference!=0xfffffffeu){if(value.reference>=graph_.elements.size())throw std::runtime_error("DAG child reference outside document");++count;}}return count;}
 void shiftSavedStateForRemoval(const RemovedDagRecord&record,bool restore){
  const auto index=record.element;std::map<std::tuple<uint32_t,size_t,size_t>,Value> attributes;
  for(auto&entry:saved_){auto key=entry.first;auto owner=std::get<0>(key);if(!restore&&owner==index)continue;if(restore&&owner>=index)++owner;else if(!restore&&owner>index)--owner;std::get<0>(key)=owner;attributes.emplace(std::move(key),entry.second);}
  std::map<uint32_t,std::string> names;
  for(auto&entry:savedNames_){auto owner=entry.first;if(!restore&&owner==index)continue;if(restore&&owner>=index)++owner;else if(!restore&&owner>index)--owner;names.emplace(owner,entry.second);}
  if(restore){for(const auto&entry:record.savedAttributes)attributes.emplace(entry.first,entry.second);for(const auto&entry:record.savedNames)names.emplace(entry.first,entry.second);}
  saved_=std::move(attributes);savedNames_=std::move(names);
 }
 void captureDagDeletion(RemovedDagRecord&record){
  const auto index=record.element;if(index>=graph_.elements.size())throw std::runtime_error("DAG deletion index outside document");
  if(!isDagType(index)||dagChildCount(index))throw std::runtime_error("Delete Empty Dag requires a childless DAG element");
  record.removed=graph_.elements[index];record.incoming.clear();record.savedAttributes.clear();record.savedNames.clear();
   for(uint32_t owner=0;owner<graph_.elements.size();++owner){if(owner==index)continue;const auto&element=graph_.elements[owner];for(size_t a=0;a<element.attributes.size();++a){const auto&attribute=element.attributes[a];if(attribute.type!=1&&attribute.type!=15)continue;if(std::any_of(attribute.values.begin(),attribute.values.end(),[&](const Value&value){return value.reference==index;}))record.incoming.push_back({owner,a,attribute});}}
   for(const auto&entry:saved_)if(std::get<0>(entry.first)==index)record.savedAttributes.push_back(entry);
   for(const auto&entry:savedNames_)if(entry.first==index)record.savedNames.push_back(entry);
 }
 void removeDagGraphElement(RemovedDagRecord&record){
  const auto index=record.element;if(index>=graph_.elements.size())throw std::runtime_error("DAG deletion index outside document");
  if(!isDagType(index)||dagChildCount(index))throw std::runtime_error("Delete Empty Dag requires a childless DAG element");
  for(uint32_t owner=0;owner<graph_.elements.size();++owner){if(owner==index)continue;auto&element=graph_.elements[owner];for(auto&attribute:element.attributes){if(attribute.type==15){for(size_t i=0;i<attribute.values.size();){auto&value=attribute.values[i];if(value.reference==index){attribute.values.erase(attribute.values.begin()+i);continue;}if(value.reference<graph_.elements.size()&&value.reference>index)--value.reference;++i;}}else if(attribute.type==1&&!attribute.values.empty()){auto&value=attribute.values.front();if(value.reference==index){value.reference=0xffffffffu;value.text.clear();}else if(value.reference<graph_.elements.size()&&value.reference>index)--value.reference;}}}
  graph_.elements.erase(graph_.elements.begin()+index);if(root_>index)--root_;if(frame_>index)--frame_;shiftSavedStateForRemoval(record,false);
 }
 void restoreDagGraphElement(const RemovedDagRecord&record){
  const auto index=record.element;if(index>graph_.elements.size())throw std::runtime_error("DAG undo index outside document");graph_.elements.insert(graph_.elements.begin()+index,record.removed);
  for(uint32_t owner=0;owner<graph_.elements.size();++owner){if(owner==index)continue;for(auto&attribute:graph_.elements[owner].attributes)if(attribute.type==1||attribute.type==15)for(auto&value:attribute.values)if(value.reference!=0xfffffffeu&&value.reference!=0xffffffffu&&value.reference<graph_.elements.size()&&value.reference>=index)++value.reference;}
  for(const auto&snapshot:record.incoming)graph_.elements.at(snapshot.element).attributes.at(snapshot.attribute)=snapshot.before;
  if(root_>=index)++root_;if(frame_>=index)++frame_;shiftSavedStateForRemoval(record,true);
 }
 void removeAddedBookmark(const AddedBookmarkRecord&record){
  if(record.element+1!=graph_.elements.size()||record.bookmarkSet>=graph_.elements.size())throw std::runtime_error("bookmark undo order or element index mismatch");
  auto&bookmarks=graph_.elements.at(record.bookmarkSet).attributes.at(record.bookmarksAttribute);
  if(bookmarks.type!=15||bookmarks.values.empty()||bookmarks.values.back().reference!=record.element)throw std::runtime_error("bookmark set undo reference mismatch");
  if(!record.stringsCommitted){
   if(graph_.strings.size()!=record.originalStringCount+record.addedStrings.size())throw std::runtime_error("bookmark string undo boundary mismatch");
   for(size_t i=0;i<record.addedStrings.size();++i)if(graph_.strings[record.originalStringCount+i]!=record.addedStrings[i])throw std::runtime_error("bookmark string undo payload mismatch");
  }
  bookmarks.values.pop_back();graph_.elements.pop_back();if(!record.stringsCommitted)graph_.strings.resize(record.originalStringCount);
 }
 void restoreAddedBookmark(const AddedBookmarkRecord&record){
  if(graph_.elements.size()!=record.element||record.bookmarkSet>=graph_.elements.size())throw std::runtime_error("bookmark redo element order mismatch");
  Element bookmark=record.bookmark;Value reference=record.reference;std::vector<std::string>addedStrings;if(!record.stringsCommitted)addedStrings=record.addedStrings;
  if(record.stringsCommitted){
   if(graph_.strings.size()<record.originalStringCount+record.addedStrings.size())throw std::runtime_error("saved bookmark strings missing on redo");
   for(size_t i=0;i<record.addedStrings.size();++i)if(graph_.strings[record.originalStringCount+i]!=record.addedStrings[i])throw std::runtime_error("saved bookmark string mismatch on redo");
  }else{
   if(graph_.strings.size()!=record.originalStringCount)throw std::runtime_error("bookmark redo string boundary mismatch");
   graph_.strings.reserve(graph_.strings.size()+record.addedStrings.size());
  }
  graph_.elements.reserve(graph_.elements.size()+1);
  auto&bookmarks=graph_.elements.at(record.bookmarkSet).attributes.at(record.bookmarksAttribute);
  if(bookmarks.type!=15)throw std::runtime_error("bookmark redo set attribute type mismatch");
  bookmarks.values.reserve(bookmarks.values.size()+1);
  if(!record.stringsCommitted)for(auto&value:addedStrings)graph_.strings.push_back(std::move(value));
  graph_.elements.push_back(std::move(bookmark));bookmarks.values.push_back(std::move(reference));
 }
 void removeAddedTrackClip(const AddedTrackClipRecord&record){
  if(record.firstElement+2!=graph_.elements.size()||record.track>=graph_.elements.size())throw std::runtime_error("track-clip undo order or element index mismatch");
  for(size_t i=0;i<2;++i)if(graph_.elements[record.firstElement+i].id!=record.elements[i].id)throw std::runtime_error("track-clip undo element identity mismatch");
  auto&children=graph_.elements[record.track].attributes.at(record.childrenAttribute);if(!sameAttribute(children,record.childrenAfter))throw std::runtime_error("track-clip undo children array mismatch");
  if(!record.stringsCommitted){if(graph_.strings.size()!=record.originalStringCount+record.addedStrings.size())throw std::runtime_error("track-clip string undo boundary mismatch");for(size_t i=0;i<record.addedStrings.size();++i)if(graph_.strings[record.originalStringCount+i]!=record.addedStrings[i])throw std::runtime_error("track-clip string undo payload mismatch");}
  children=record.childrenBefore;graph_.elements.resize(record.firstElement);if(!record.stringsCommitted)graph_.strings.resize(record.originalStringCount);
 }
 void restoreAddedTrackClip(const AddedTrackClipRecord&record){
  if(graph_.elements.size()!=record.firstElement||record.track>=graph_.elements.size())throw std::runtime_error("track-clip redo element order mismatch");
  if(record.stringsCommitted){if(graph_.strings.size()<record.originalStringCount+record.addedStrings.size())throw std::runtime_error("saved track-clip strings missing on redo");for(size_t i=0;i<record.addedStrings.size();++i)if(graph_.strings[record.originalStringCount+i]!=record.addedStrings[i])throw std::runtime_error("saved track-clip string mismatch on redo");}
  else if(graph_.strings.size()!=record.originalStringCount)throw std::runtime_error("track-clip redo string boundary mismatch");
  graph_.elements.reserve(graph_.elements.size()+2);graph_.strings.reserve(graph_.strings.size()+record.addedStrings.size());graph_.elements[record.track].attributes[record.childrenAttribute].values.reserve(record.childrenAfter.values.size());graph_.elements.push_back(record.elements[0]);graph_.elements.push_back(record.elements[1]);graph_.elements[record.track].attributes[record.childrenAttribute]=record.childrenAfter;
  if(!record.stringsCommitted)for(const auto&value:record.addedStrings)graph_.strings.push_back(value);
 }
public:
 explicit SessionTimelineDocumentCandidate(Graph graph):graph_(std::move(graph)),root_(sessionRenderRootCandidate(graph_)),savedElementCount_(graph_.elements.size()){
  sessionTimelineFramesCandidate(graph_,root_);frame_=hierarchyRefs(graph_,graph_.elements.at(root_),"timeFrame",1).at(0);duration_=SIZE_MAX;
  for(size_t i=0;i<graph_.elements[frame_].attributes.size();++i)if(graph_.strings.at(graph_.elements[frame_].attributes[i].name)=="duration")duration_=i;
  saved_[{frame_,duration_,0}]=duration().values.at(0);
 }
 const Graph&graph()const{return graph_;}
 uint32_t documentRoot()const{return sessionDocumentRootCandidate(graph_);}
 // Caller supplies recovered runtime state and the engine's shared retained
 // clock. Sampling follows this document's current settings, including edits.
 int32_t documentTime(const AnchoredDocumentTimeState&state,bool engineGate,
                      uint32_t currentSecondsWord,uint32_t&retainedSecondsWord)const{
  return DocumentTimeCandidate(documentRoot(),state).sample(graph_,engineGate,currentSecondsWord,retainedSecondsWord);
 }
 // Stage document/history changes, then publish the scene before committing
 // the document. Publish must itself provide a strong exception guarantee.
 // This correctness-first path copies history; a compact edit plan can replace
 // staging later without weakening the two-owner commit contract.
 template<class Mutate,class Publish>bool transact(Mutate mutate,Publish publish){
  static_assert(std::is_nothrow_move_assignable<SessionTimelineDocumentCandidate>::value,"document commit must not throw");
  SessionTimelineDocumentCandidate staged(*this);if(!mutate(staged))return false;
  publish(staged.graph());*this=std::move(staged);return true;
 }
 bool dirty()const{if(graph_.elements.size()!=savedElementCount_)return true;for(const auto&item:savedArrays_)if(!sameAttribute(graph_.elements.at(item.first.first).attributes.at(item.first.second),item.second))return true;for(const auto&item:saved_)if(!savedArrays_.count({std::get<0>(item.first),std::get<1>(item.first)})&&!sameValue(graph_.elements.at(std::get<0>(item.first)).attributes.at(std::get<1>(item.first)).values.at(std::get<2>(item.first)),item.second))return true;for(const auto&item:savedNames_)if(graph_.strings.at(graph_.elements.at(item.first).name)!=item.second)return true;return false;}
 size_t frameCount()const{return sessionTimelineFramesCandidate(graph_,root_).size();}
  bool addBookmarkAtTime(uint32_t bookmarkSet,int32_t timeTicks){
   if(bookmarkSet>=graph_.elements.size()||graph_.strings.at(graph_.elements[bookmarkSet].type)!="DmeBookmarkSet")throw std::runtime_error("bookmark target must be a DmeBookmarkSet");
   size_t bookmarksAttribute=SIZE_MAX;for(size_t i=0;i<graph_.elements[bookmarkSet].attributes.size();++i)if(graph_.strings.at(graph_.elements[bookmarkSet].attributes[i].name)=="bookmarks"){bookmarksAttribute=i;break;}
   if(bookmarksAttribute==SIZE_MAX||graph_.elements[bookmarkSet].attributes[bookmarksAttribute].type!=15)throw std::runtime_error("bookmark set has no element-reference array");
   const auto&existing=graph_.elements[bookmarkSet].attributes[bookmarksAttribute].values;const size_t existingCount=existing.size();std::set<std::string>names;
   for(const auto&value:existing){if(value.reference>=graph_.elements.size()||graph_.strings.at(graph_.elements[value.reference].type)!="DmeBookmark")throw std::runtime_error("bookmark set contains an invalid bookmark reference");const auto&bookmark=graph_.elements[value.reference];names.insert(graph_.strings.at(bookmark.name));const auto*time=findAttributeForDocumentCandidate(bookmark,"time");if(time&&time->type==7&&time->values.size()==1&&wireInt(time->values[0])==timeTicks)return false;}
   if(graph_.elements.size()>=UINT32_MAX)throw std::runtime_error("bookmark element table is full");
   auto record=std::make_shared<AddedBookmarkRecord>();record->bookmarkSet=bookmarkSet;record->bookmarksAttribute=bookmarksAttribute;record->element=uint32_t(graph_.elements.size());record->originalStringCount=graph_.strings.size();
   try{
    auto intern=[&](const std::string&value){return internString(graph_,value);};
    Element bookmark{};bookmark.type=intern("DmeBookmark");std::string bookmarkName="bookmark";if(names.count(bookmarkName)){for(uint32_t suffix=2;;++suffix){bookmarkName="bookmark"+std::to_string(suffix);if(!names.count(bookmarkName))break;if(suffix==UINT32_MAX)throw std::runtime_error("bookmark names exhausted");}}bookmark.name=intern(bookmarkName);
    std::random_device random;bool unique=false;for(unsigned attempt=0;attempt<16&&!unique;++attempt){for(auto&byte:bookmark.id)byte=static_cast<char>(random()&0xff);bookmark.id[6]=char((static_cast<unsigned char>(bookmark.id[6])&0x0f)|0x40);bookmark.id[8]=char((static_cast<unsigned char>(bookmark.id[8])&0x3f)|0x80);unique=std::none_of(graph_.elements.begin(),graph_.elements.end(),[&](const Element&other){return other.id==bookmark.id;});}if(!unique)throw std::runtime_error("could not generate unique bookmark identity");
    auto makeTime=[&](const char*name,int32_t ticks){Attribute attribute{};attribute.name=intern(name);attribute.type=7;Value value;value.wire.resize(4);setWireU32(value,uint32_t(ticks));attribute.values.push_back(std::move(value));bookmark.attributes.push_back(std::move(attribute));};
    makeTime("time",timeTicks);makeTime("duration",0);
    Attribute note{};note.name=intern("note");note.type=5;Value noteValue;noteValue.text="";noteValue.stringIndex=intern("");note.values.push_back(std::move(noteValue));bookmark.attributes.push_back(std::move(note));
    record->bookmark=std::move(bookmark);record->addedStrings.assign(graph_.strings.begin()+record->originalStringCount,graph_.strings.end());
    Element preparedBookmark=record->bookmark;Value preparedReference;preparedReference.reference=record->element;record->reference=preparedReference;
    graph_.elements.reserve(graph_.elements.size()+1);graph_.elements[bookmarkSet].attributes[bookmarksAttribute].values.reserve(existingCount+1);
    Edit edit{};edit.element=record->element;edit.attribute=bookmarksAttribute;edit.addedBookmark=record;std::vector<Edit>edits;edits.push_back(std::move(edit));journal_.appendGroup(std::move(edits));
    graph_.elements.push_back(std::move(preparedBookmark));graph_.elements[bookmarkSet].attributes[bookmarksAttribute].values.push_back(std::move(preparedReference));return true;
   }catch(...){graph_.strings.resize(record->originalStringCount);throw;}
  }
 // Document history adapter; native owner/class mutation flags are separate.
 bool setScalarFloat(uint32_t element,const std::string&name,float value){
 if(!std::isfinite(value))throw std::runtime_error("document scalar edit requires a finite float");
 auto&e=graph_.elements.at(element);for(size_t i=0;i<e.attributes.size();++i){auto&a=e.attributes[i];if(graph_.strings.at(a.name)!=name)continue;if(a.type!=3||a.values.size()!=1||a.values[0].wire.size()!=4)throw std::runtime_error("document scalar edit requires float");float current;std::memcpy(&current,a.values[0].wire.data(),4);if(!floatMutationNeeded(current,value))return false;
 auto before=a.values[0],after=before;uint32_t bits;std::memcpy(&bits,&value,4);bits=recoveredX87FloatStoreBitsCandidate(bits);std::memcpy(after.wire.data(),&bits,4);auto nextSaved=saved_;nextSaved.emplace(std::make_tuple(element,i,size_t(0)),before);std::vector<Edit>edits;edits.push_back({element,i,before,after});journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);a.values[0]=std::move(after);return true;
 }throw std::runtime_error("missing document scalar attribute");
 }
 bool setBooleanAttribute(uint32_t element,const std::string&name,bool value){
  auto&e=graph_.elements.at(element);for(size_t i=0;i<e.attributes.size();++i){auto&a=e.attributes[i];if(graph_.strings.at(a.name)!=name)continue;if(a.type!=4||a.values.size()!=1||a.values[0].wire.size()!=1)throw std::runtime_error("document boolean edit requires a scalar boolean attribute");const bool current=a.values[0].wire[0]!=0;if(current==value)return false;auto before=a.values[0],after=before;after.wire[0]=value?1:0;auto nextSaved=saved_;nextSaved.emplace(std::make_tuple(element,i,size_t(0)),before);std::vector<Edit>edits;edits.push_back({element,i,before,after});journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);a.values[0]=std::move(after);return true;
  }throw std::runtime_error("missing document boolean attribute");
 }
 bool setStringAttribute(uint32_t element,const std::string&name,std::string value){
  if(value.find('\0')!=std::string::npos)throw std::runtime_error("document string edit contains an embedded NUL");
  auto&e=graph_.elements.at(element);for(size_t i=0;i<e.attributes.size();++i){auto&a=e.attributes[i];if(graph_.strings.at(a.name)!=name)continue;if(a.type!=5||a.values.size()!=1)throw std::runtime_error("document string edit requires a scalar string attribute");auto&currentValue=a.values[0];if(currentValue.stringIndex>=graph_.strings.size()||graph_.strings[currentValue.stringIndex]!=currentValue.text)throw std::runtime_error("document string attribute is inconsistent");if(currentValue.text==value)return false;
   uint32_t nextIndex=UINT32_MAX;for(uint32_t index=0;index<graph_.strings.size();++index)if(graph_.strings[index]==value){nextIndex=index;break;}const bool append=nextIndex==UINT32_MAX;if(append){if(graph_.strings.size()>=UINT32_MAX)throw std::runtime_error("DMX string table is full");nextIndex=uint32_t(graph_.strings.size());graph_.strings.reserve(graph_.strings.size()+1);}
   Value before=currentValue,after=before;after.text=value;after.stringIndex=nextIndex;auto nextSaved=saved_;nextSaved.emplace(std::make_tuple(element,i,size_t(0)),before);std::vector<Edit>edits;edits.push_back({element,i,before,after});journal_.appendGroup(std::move(edits));if(append)graph_.strings.push_back(std::move(value));saved_=std::move(nextSaved);a.values[0]=std::move(after);return true;
  }throw std::runtime_error("missing document string attribute");
 }
 bool renameElement(uint32_t element,std::string name){
  if(element>=graph_.elements.size())throw std::runtime_error("rename element outside document");if(name.empty()||name.find_first_of("\r\n\0",0,3)!=std::string::npos)throw std::runtime_error("element name is empty or contains line breaks");
  auto&target=graph_.elements[element];const auto current=graph_.strings.at(target.name);if(current==name)return false;
  uint32_t nextIndex=UINT32_MAX;for(uint32_t i=0;i<graph_.strings.size();++i)if(graph_.strings[i]==name){nextIndex=i;break;}
  const bool append=nextIndex==UINT32_MAX;if(append){if(graph_.strings.size()>=UINT32_MAX)throw std::runtime_error("DMX string table is full");nextIndex=uint32_t(graph_.strings.size());graph_.strings.reserve(graph_.strings.size()+1);}
  auto nextSaved=savedNames_;nextSaved.try_emplace(element,current);Edit edit{};edit.element=element;edit.attribute=SIZE_MAX;edit.elementName=true;edit.nameBefore=target.name;edit.nameAfter=nextIndex;std::vector<Edit>edits;edits.push_back(std::move(edit));journal_.appendGroup(std::move(edits));
  if(append)graph_.strings.push_back(std::move(name));savedNames_=std::move(nextSaved);target.name=nextIndex;return true;
 }
 bool deleteEmptyDag(uint32_t element){
  if(element>=graph_.elements.size())throw std::runtime_error("DAG deletion target outside document");if(!isDagType(element))throw std::runtime_error("Delete Empty Dag requires a DAG element");if(dagChildCount(element))throw std::runtime_error("Delete Empty Dag requires a childless DAG element");
  auto record=std::make_shared<RemovedDagRecord>();record->element=element;captureDagDeletion(*record);Edit edit{};edit.element=element;edit.attribute=SIZE_MAX;edit.removedDag=record;std::vector<Edit>edits;edits.push_back(std::move(edit));journal_.appendGroup(std::move(edits));removeDagGraphElement(*record);return true;
 }
 bool deleteAnimationKey(uint32_t layer,size_t index){return deleteAnimationKeys(layer,{index});}
 bool deleteAnimationKeys(uint32_t layer,std::vector<size_t>indices){
  if(indices.empty())return false;std::sort(indices.begin(),indices.end());indices.erase(std::unique(indices.begin(),indices.end()),indices.end());
  auto&element=graph_.elements.at(layer);const auto&type=graph_.strings.at(element.type);if(type!="DmeFloatLogLayer"&&type!="DmeVector3LogLayer")throw std::runtime_error("key deletion requires a supported log layer");
  KeyTrack track(graph_,element);if((type=="DmeFloatLogLayer"&&track.values->type!=17)||(type=="DmeVector3LogLayer"&&track.values->type!=24)||track.times->values.size()!=track.values->values.size()||indices.back()>=track.values->values.size())throw std::runtime_error("animation key delete index/type invalid");
  size_t timesAttribute=SIZE_MAX,valuesAttribute=SIZE_MAX;for(size_t i=0;i<element.attributes.size();++i){const auto&name=graph_.strings.at(element.attributes[i].name);if(name=="times")timesAttribute=i;else if(name=="values")valuesAttribute=i;}
  if(timesAttribute==SIZE_MAX||valuesAttribute==SIZE_MAX||timesAttribute==valuesAttribute)throw std::runtime_error("missing animation key arrays");
  auto record=std::make_shared<KeyDeletionRecord>();record->element=layer;record->timesAttribute=timesAttribute;record->valuesAttribute=valuesAttribute;record->indices=std::move(indices);record->timesBefore=element.attributes[timesAttribute];record->valuesBefore=element.attributes[valuesAttribute];
  auto nextSaved=saved_;auto nextSavedArrays=savedArrays_;
  for(size_t attribute:{timesAttribute,valuesAttribute}){
   const auto key=std::make_pair(layer,attribute);if(!nextSavedArrays.count(key)){Attribute baseline=element.attributes[attribute];for(const auto&saved:nextSaved)if(std::get<0>(saved.first)==layer&&std::get<1>(saved.first)==attribute&&std::get<2>(saved.first)<baseline.values.size())baseline.values[std::get<2>(saved.first)]=saved.second;nextSavedArrays.emplace(key,std::move(baseline));}
   for(auto saved=nextSaved.begin();saved!=nextSaved.end();)if(std::get<0>(saved->first)==layer&&std::get<1>(saved->first)==attribute)saved=nextSaved.erase(saved);else ++saved;
  }
  Edit edit{};edit.element=layer;edit.attribute=SIZE_MAX;edit.keyDeletion=record;std::vector<Edit>edits;edits.push_back(std::move(edit));journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);savedArrays_=std::move(nextSavedArrays);applyKeyDeletion(*record);return true;
 }
 bool setFloatKeyValue(uint32_t layer,size_t index,float value){
 if(!std::isfinite(value))throw std::runtime_error("document animation key edit requires a finite float");
 auto&e=graph_.elements.at(layer);if(graph_.strings.at(e.type)!="DmeFloatLogLayer")throw std::runtime_error("float key edit requires float log layer");KeyTrack track(graph_,e);if(track.values->type!=17||index>=track.values->values.size())throw std::runtime_error("float key index/type invalid");
  for(size_t i=0;i<e.attributes.size();++i){auto&a=e.attributes[i];if(graph_.strings.at(a.name)!="values")continue;auto before=a.values.at(index);if(before.wire.size()!=4)throw std::runtime_error("invalid float key payload");if(!floatMutationNeeded(wireFloat(before),value))return false;auto after=before;uint32_t bits;std::memcpy(&bits,&value,4);bits=recoveredX87FloatStoreBitsCandidate(bits);setWireU32(after,bits);auto nextSaved=saved_;if(!savedArrays_.count({layer,i}))nextSaved.emplace(std::make_tuple(layer,i,index),before);std::vector<Edit>edits;edits.push_back({layer,i,before,after,index});journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);a.values[index]=std::move(after);return true;
 }throw std::runtime_error("missing float key values");
 }
 bool setFloatKey(uint32_t layer,size_t index,int32_t tick,float value){
  if(!std::isfinite(value))throw std::runtime_error("document animation key edit requires a finite float");
  auto&e=graph_.elements.at(layer);if(graph_.strings.at(e.type)!="DmeFloatLogLayer")throw std::runtime_error("float key edit requires float log layer");KeyTrack track(graph_,e);if(track.values->type!=17||index>=track.values->values.size())throw std::runtime_error("float key index/type invalid");
  const int32_t previous=index?wireInt(track.times->values[index-1]):INT32_MIN;
  const int32_t next=index+1<track.times->values.size()?wireInt(track.times->values[index+1]):INT32_MAX;
  if(tick<previous||tick>next)throw std::runtime_error("animation key cannot cross a neighboring key");
  size_t timesAttribute=SIZE_MAX,valuesAttribute=SIZE_MAX;for(size_t i=0;i<e.attributes.size();++i){const auto&name=graph_.strings.at(e.attributes[i].name);if(name=="times")timesAttribute=i;else if(name=="values")valuesAttribute=i;}
  if(timesAttribute==SIZE_MAX||valuesAttribute==SIZE_MAX)throw std::runtime_error("missing float key arrays");
  auto beforeTime=e.attributes[timesAttribute].values.at(index),afterTime=beforeTime;setWireU32(afterTime,uint32_t(tick));
  auto beforeValue=e.attributes[valuesAttribute].values.at(index),afterValue=beforeValue;if(beforeValue.wire.size()!=4)throw std::runtime_error("invalid float key payload");
  uint32_t bits;std::memcpy(&bits,&value,4);bits=recoveredX87FloatStoreBitsCandidate(bits);setWireU32(afterValue,bits);
  const bool timeChanged=beforeTime.wire!=afterTime.wire,valueChanged=beforeValue.wire!=afterValue.wire;if(!timeChanged&&!valueChanged)return false;
  auto nextSaved=saved_;std::vector<Edit>edits;edits.reserve(size_t(timeChanged)+size_t(valueChanged));
  if(timeChanged){if(!savedArrays_.count({layer,timesAttribute}))nextSaved.emplace(std::make_tuple(layer,timesAttribute,index),beforeTime);edits.push_back({layer,timesAttribute,beforeTime,afterTime,index});}
  if(valueChanged){if(!savedArrays_.count({layer,valuesAttribute}))nextSaved.emplace(std::make_tuple(layer,valuesAttribute,index),beforeValue);edits.push_back({layer,valuesAttribute,beforeValue,afterValue,index});}
  journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);if(timeChanged)e.attributes[timesAttribute].values[index]=std::move(afterTime);if(valueChanged)e.attributes[valuesAttribute].values[index]=std::move(afterValue);return true;
 }
 bool setVectorKeyComponent(uint32_t layer,size_t index,unsigned component,int32_t tick,float value){
  if(!std::isfinite(value))throw std::runtime_error("document vector animation key edit requires a finite float");
  auto&e=graph_.elements.at(layer);if(graph_.strings.at(e.type)!="DmeVector3LogLayer")throw std::runtime_error("vector key edit requires vector3 log layer");if(component>=3)throw std::runtime_error("vector key component outside xyz");KeyTrack track(graph_,e);if(track.values->type!=24||index>=track.values->values.size())throw std::runtime_error("vector key index/type invalid");
  const int32_t previous=index?wireInt(track.times->values[index-1]):INT32_MIN;const int32_t next=index+1<track.times->values.size()?wireInt(track.times->values[index+1]):INT32_MAX;if(tick<previous||tick>next)throw std::runtime_error("animation key cannot cross a neighboring key");
  size_t timesAttribute=SIZE_MAX,valuesAttribute=SIZE_MAX;for(size_t i=0;i<e.attributes.size();++i){const auto&name=graph_.strings.at(e.attributes[i].name);if(name=="times")timesAttribute=i;else if(name=="values")valuesAttribute=i;}if(timesAttribute==SIZE_MAX||valuesAttribute==SIZE_MAX)throw std::runtime_error("missing vector key arrays");
  auto beforeTime=e.attributes[timesAttribute].values.at(index),afterTime=beforeTime;setWireU32(afterTime,uint32_t(tick));auto beforeValue=e.attributes[valuesAttribute].values.at(index),afterValue=beforeValue;if(beforeValue.wire.size()!=12)throw std::runtime_error("invalid vector key payload");
  uint32_t bits;std::memcpy(&bits,&value,4);bits=recoveredX87FloatStoreBitsCandidate(bits);setWireU32(afterValue,bits,component);const bool timeChanged=beforeTime.wire!=afterTime.wire,valueChanged=beforeValue.wire!=afterValue.wire;if(!timeChanged&&!valueChanged)return false;
  auto nextSaved=saved_;std::vector<Edit>edits;edits.reserve(size_t(timeChanged)+size_t(valueChanged));if(timeChanged){if(!savedArrays_.count({layer,timesAttribute}))nextSaved.emplace(std::make_tuple(layer,timesAttribute,index),beforeTime);edits.push_back({layer,timesAttribute,beforeTime,afterTime,index});}if(valueChanged){if(!savedArrays_.count({layer,valuesAttribute}))nextSaved.emplace(std::make_tuple(layer,valuesAttribute,index),beforeValue);edits.push_back({layer,valuesAttribute,beforeValue,afterValue,index});}
  journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);if(timeChanged)e.attributes[timesAttribute].values[index]=std::move(afterTime);if(valueChanged)e.attributes[valuesAttribute].values[index]=std::move(afterValue);return true;
 }
 void setDuration(int32_t ticks){auto&a=duration();Value before=a.values.at(0),after=before;setWireU32(after,uint32_t(ticks));if(before.wire==after.wire)return;
  auto refs=hierarchyRefs(graph_,graph_.elements[root_],"timeFrame",1);AnimationTimeFrame frame(graph_,graph_.elements.at(refs[0]));SessionTimelineFramesCandidate checked(frame.start,ticks,sessionFrameRateCandidate(graph_,root_,false));(void)checked;
  journal_.beginBoundary();journal_.append({frame_,duration_,before,after});a.values[0]=std::move(after);
 }
 void trimClipStart(uint32_t clip,int32_t newStart){
  if(!isTrackClip(clip))throw std::runtime_error("trim requires a child clip on a session track");
  auto frameRefs=hierarchyRefs(graph_,graph_.elements.at(clip),"timeFrame",1);if(frameRefs.size()!=1)throw std::runtime_error("trim clip time frame missing");auto frameId=frameRefs.front();const auto&element=graph_.elements.at(frameId);AnimationTimeFrame frame(graph_,element);int64_t delta=int64_t(newStart)-frame.start;
  if(delta<=0||delta>=frame.duration)throw std::runtime_error("trim start outside clip range");int64_t offset=int64_t(frame.offset)+delta;if(offset>INT32_MAX||offset<INT32_MIN)throw std::runtime_error("trim media offset overflow");
  std::vector<Edit>edits;auto nextSaved=saved_;for(size_t i=0;i<element.attributes.size();++i){const auto&a=element.attributes[i];auto name=graph_.strings.at(a.name);if(name!="start"&&name!="duration"&&name!="offset")continue;auto after=a.values.at(0);auto value=name=="start"?newStart:name=="duration"?int32_t(int64_t(frame.duration)-delta):int32_t(offset);setWireU32(after,uint32_t(value));edits.push_back({frameId,i,a.values[0],std::move(after)});nextSaved.emplace(std::make_tuple(frameId,i,size_t(0)),a.values[0]);}
  // Prepare the entire journal transaction before changing document values.
  auto journalEdits=edits;journal_.appendGroup(std::move(journalEdits));saved_=std::move(nextSaved);for(auto&edit:edits)graph_.elements[edit.element].attributes[edit.attribute].values[0]=std::move(edit.after);
 }
 bool trimClipEnd(uint32_t clip,int32_t newEnd){
  if(!isTrackClip(clip))throw std::runtime_error("trim requires a child clip on a session track");
  auto frameRefs=hierarchyRefs(graph_,graph_.elements.at(clip),"timeFrame",1);if(frameRefs.size()!=1)throw std::runtime_error("trim clip time frame missing");auto frameId=frameRefs.front();const auto&element=graph_.elements.at(frameId);AnimationTimeFrame frame(graph_,element);const int64_t oldEnd=int64_t(frame.start)+frame.duration;
  if(newEnd==oldEnd)return false;if(newEnd<=frame.start||newEnd>oldEnd)throw std::runtime_error("trim end outside clip range");const int32_t nextDuration=int32_t(int64_t(newEnd)-frame.start);
  size_t durationAttribute=SIZE_MAX;for(size_t i=0;i<element.attributes.size();++i)if(graph_.strings.at(element.attributes[i].name)=="duration"){durationAttribute=i;break;}
  if(durationAttribute==SIZE_MAX)throw std::runtime_error("clip duration attribute missing");auto before=element.attributes[durationAttribute].values.at(0),after=before;setWireU32(after,uint32_t(nextDuration));
  auto nextSaved=saved_;nextSaved.emplace(std::make_tuple(frameId,durationAttribute,size_t(0)),before);std::vector<Edit>edits;edits.push_back({frameId,durationAttribute,before,after});journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);graph_.elements.at(frameId).attributes.at(durationAttribute).values[0]=std::move(after);return true;
 }
 bool moveTrackClip(uint32_t clip,int32_t newStart){
  if(!isTrackClip(clip))throw std::runtime_error("move requires a child clip on a session track");
  const auto ownFrameRefs=hierarchyRefs(graph_,graph_.elements.at(clip),"timeFrame",1);if(ownFrameRefs.size()!=1)throw std::runtime_error("move clip time frame missing");
  const uint32_t ownFrameId=ownFrameRefs.front();const auto&ownFrameElement=graph_.elements.at(ownFrameId);const AnimationTimeFrame ownFrame(graph_,ownFrameElement);
  if(ownFrame.duration<=0||!std::isfinite(ownFrame.scale)||ownFrame.scale==0)throw std::runtime_error("move clip has invalid time frame");
  const int64_t proposedEnd=int64_t(newStart)+ownFrame.duration;if(proposedEnd>INT32_MAX||proposedEnd<INT32_MIN)throw std::runtime_error("move clip end overflow");if(newStart==ownFrame.start)return false;
  const auto&root=graph_.elements.at(root_);std::vector<uint32_t>groups=hierarchyRefs(graph_,root,"subClipTrackGroup",1);const auto regular=hierarchyRefs(graph_,root,"trackGroups",15);groups.insert(groups.end(),regular.begin(),regular.end());
  uint32_t owner=UINT32_MAX;size_t startAttribute=SIZE_MAX;
  for(uint32_t group:groups)for(uint32_t track:hierarchyRefs(graph_,graph_.elements.at(group),"tracks",15)){
   const auto&trackElement=graph_.elements.at(track);bool owns=false;for(const auto&child:hierarchyRefs(graph_,trackElement,"children",15))if(child==clip)owns=true;if(!owns)continue;
   if(owner!=UINT32_MAX)throw std::runtime_error("clip is referenced by multiple session tracks");owner=track;
   for(size_t a=0;a<ownFrameElement.attributes.size();++a)if(graph_.strings.at(ownFrameElement.attributes[a].name)=="start")startAttribute=a;
  }
  if(owner==UINT32_MAX||startAttribute==SIZE_MAX)throw std::runtime_error("move clip track/start attribute missing");
  const int64_t newEnd=proposedEnd;
  for(uint32_t sibling:hierarchyRefs(graph_,graph_.elements.at(owner),"children",15)){
   if(sibling==clip)continue;const auto refs=hierarchyRefs(graph_,graph_.elements.at(sibling),"timeFrame",1);if(refs.size()!=1)continue;
   const AnimationTimeFrame other(graph_,graph_.elements.at(refs.front()));if(other.duration<=0)continue;const int64_t otherEnd=int64_t(other.start)+other.duration;
   if(int64_t(newStart)<otherEnd&&int64_t(other.start)<newEnd)throw std::runtime_error("move would overlap another clip on this track");
  }
  auto before=ownFrameElement.attributes.at(startAttribute).values.at(0),after=before;setWireU32(after,uint32_t(newStart));auto nextSaved=saved_;nextSaved.emplace(std::make_tuple(ownFrameId,startAttribute,size_t(0)),before);std::vector<Edit>edits;edits.push_back({ownFrameId,startAttribute,before,after});journal_.appendGroup(std::move(edits));saved_=std::move(nextSaved);graph_.elements.at(ownFrameId).attributes.at(startAttribute).values[0]=std::move(after);return true;
 }
 bool removeTrackClip(uint32_t clip){
  if(!isTrackClip(clip))throw std::runtime_error("delete requires a child clip on a session track");
  const auto&root=graph_.elements.at(root_);std::vector<uint32_t>groups=hierarchyRefs(graph_,root,"subClipTrackGroup",1);const auto regular=hierarchyRefs(graph_,root,"trackGroups",15);groups.insert(groups.end(),regular.begin(),regular.end());
  uint32_t owner=UINT32_MAX;size_t childrenAttribute=SIZE_MAX;
  for(uint32_t group:groups)for(uint32_t track:hierarchyRefs(graph_,graph_.elements.at(group),"tracks",15)){
   const auto&trackElement=graph_.elements.at(track);for(size_t a=0;a<trackElement.attributes.size();++a){const auto&attribute=trackElement.attributes[a];if(graph_.strings.at(attribute.name)!="children")continue;if(attribute.type!=15)throw std::runtime_error("track children attribute is not an element array");for(const auto&value:attribute.values)if(value.reference==clip){if(owner!=UINT32_MAX)throw std::runtime_error("clip is referenced by multiple session tracks");owner=track;childrenAttribute=a;}}
  }
  if(owner==UINT32_MAX)throw std::runtime_error("track clip reference not found");
  auto record=std::make_shared<TrackClipRemovalRecord>();record->track=owner;record->childrenAttribute=childrenAttribute;const auto&children=graph_.elements.at(owner).attributes.at(childrenAttribute);auto found=std::find_if(children.values.begin(),children.values.end(),[&](const Value&value){return value.reference==clip;});if(found==children.values.end())throw std::runtime_error("track clip reference disappeared");record->clipReference=*found;record->before=children;
  auto nextSavedArrays=savedArrays_;nextSavedArrays.try_emplace({owner,childrenAttribute},record->before);
  Edit edit{};edit.element=owner;edit.attribute=childrenAttribute;edit.trackClipRemoval=record;std::vector<Edit>edits;edits.push_back(std::move(edit));journal_.appendGroup(std::move(edits));savedArrays_=std::move(nextSavedArrays);applyTrackClipRemoval(*record);return true;
 }
 bool duplicateTrackClipAt(uint32_t clip,int32_t newStart){
  if(!isTrackClip(clip))throw std::runtime_error("duplicate requires a child clip on a session track");
  if(graph_.elements.size()>UINT32_MAX-2)throw std::runtime_error("DMX element table is full");
  const auto sourceFrameRefs=hierarchyRefs(graph_,graph_.elements.at(clip),"timeFrame",1);if(sourceFrameRefs.size()!=1)throw std::runtime_error("duplicate clip time frame missing");
  const uint32_t sourceFrameId=sourceFrameRefs.front();const AnimationTimeFrame sourceFrame(graph_,graph_.elements.at(sourceFrameId));
  if(sourceFrame.duration<=0||sourceFrame.scale==0||!std::isfinite(sourceFrame.scale))throw std::runtime_error("duplicate clip has invalid time frame");
  const int64_t newEnd=int64_t(newStart)+sourceFrame.duration;if(newEnd>INT32_MAX||newEnd<INT32_MIN)throw std::runtime_error("duplicate clip end overflow");
  const auto&root=graph_.elements.at(root_);std::vector<uint32_t>groups=hierarchyRefs(graph_,root,"subClipTrackGroup",1);const auto regular=hierarchyRefs(graph_,root,"trackGroups",15);groups.insert(groups.end(),regular.begin(),regular.end());
  uint32_t owner=UINT32_MAX;size_t childrenAttribute=SIZE_MAX;
  for(uint32_t group:groups)for(uint32_t track:hierarchyRefs(graph_,graph_.elements.at(group),"tracks",15)){
   const auto&trackElement=graph_.elements.at(track);for(size_t a=0;a<trackElement.attributes.size();++a){const auto&attribute=trackElement.attributes[a];if(graph_.strings.at(attribute.name)!="children")continue;if(attribute.type!=15)throw std::runtime_error("track children attribute is not an element array");if(std::any_of(attribute.values.begin(),attribute.values.end(),[&](const Value&value){return value.reference==clip;})){if(owner!=UINT32_MAX)throw std::runtime_error("clip is referenced by multiple session tracks");owner=track;childrenAttribute=a;}}
  }
  if(owner==UINT32_MAX)throw std::runtime_error("duplicate clip track not found");
  auto&trackElement=graph_.elements.at(owner);const auto*kind=findAttributeForDocumentCandidate(trackElement,"clipType");if(!kind||kind->type!=2||kind->values.size()!=1)throw std::runtime_error("duplicate clip track type missing");
  const int32_t clipType=wireInt(kind->values.front());const std::string expected=clipType==1?"DmeSoundClip":clipType==2?"DmeMaterialOverlayFXClip":clipType==3?"DmeFilmClip":"";
  if(expected.empty()||graph_.strings.at(graph_.elements.at(clip).type)!=expected)throw std::runtime_error("duplicate clip type does not match its track");
  for(uint32_t sibling:hierarchyRefs(graph_,trackElement,"children",15)){if(sibling==clip)continue;const auto refs=hierarchyRefs(graph_,graph_.elements.at(sibling),"timeFrame",1);if(refs.size()!=1)continue;const AnimationTimeFrame other(graph_,graph_.elements.at(refs.front()));const int64_t otherEnd=int64_t(other.start)+other.duration;if(other.duration>0&&int64_t(newStart)<otherEnd&&int64_t(other.start)<newEnd)throw std::runtime_error("duplicate would overlap another clip on this track");}
  auto record=std::make_shared<AddedTrackClipRecord>();record->track=owner;record->childrenAttribute=childrenAttribute;record->firstElement=uint32_t(graph_.elements.size());record->originalStringCount=graph_.strings.size();record->childrenBefore=trackElement.attributes[childrenAttribute];record->childrenAfter=record->childrenBefore;
  try{
   Element frameCopy=graph_.elements.at(sourceFrameId),clipCopy=graph_.elements.at(clip);
   auto intern=[&](const std::string&value){return internString(graph_,value);};
   auto start=std::find_if(frameCopy.attributes.begin(),frameCopy.attributes.end(),[&](const Attribute&attribute){return graph_.strings.at(attribute.name)=="start";});if(start==frameCopy.attributes.end()||start->type!=7||start->values.size()!=1)throw std::runtime_error("duplicate clip start attribute missing");setWireU32(start->values.front(),uint32_t(newStart));
   std::string baseName=graph_.strings.at(graph_.elements.at(clip).name)+" copy";std::set<std::string>names;for(const auto&value:record->childrenBefore.values)if(value.reference<graph_.elements.size())names.insert(graph_.strings.at(graph_.elements[value.reference].name));std::string clipName=baseName;if(names.count(clipName)){for(uint32_t suffix=2;;++suffix){clipName=baseName+" "+std::to_string(suffix);if(!names.count(clipName))break;if(suffix==UINT32_MAX)throw std::runtime_error("clip names exhausted");}}
   clipCopy.name=intern(clipName);auto frameLink=std::find_if(clipCopy.attributes.begin(),clipCopy.attributes.end(),[&](const Attribute&attribute){return graph_.strings.at(attribute.name)=="timeFrame";});if(frameLink==clipCopy.attributes.end()||frameLink->type!=1||frameLink->values.size()!=1)throw std::runtime_error("duplicate clip timeFrame link is invalid");
   std::random_device random;auto uniqueId=[&](Element&element){bool unique=false;for(unsigned attempt=0;attempt<16&&!unique;++attempt){for(auto&byte:element.id)byte=static_cast<char>(random()&0xff);element.id[6]=char((static_cast<unsigned char>(element.id[6])&0x0f)|0x40);element.id[8]=char((static_cast<unsigned char>(element.id[8])&0x3f)|0x80);unique=std::none_of(graph_.elements.begin(),graph_.elements.end(),[&](const Element&other){return other.id==element.id;});}if(!unique)throw std::runtime_error("could not generate unique clip identity");};uniqueId(frameCopy);uniqueId(clipCopy);
   frameLink->values.front().reference=record->firstElement;Value clipRef;clipRef.reference=record->firstElement+1;record->childrenAfter.values.push_back(clipRef);
   record->elements={std::move(frameCopy),std::move(clipCopy)};record->addedStrings.assign(graph_.strings.begin()+record->originalStringCount,graph_.strings.end());
   auto nextSavedArrays=savedArrays_;nextSavedArrays.try_emplace({owner,childrenAttribute},record->childrenBefore);
   graph_.elements.reserve(graph_.elements.size()+2);graph_.elements[owner].attributes[childrenAttribute].values.reserve(record->childrenAfter.values.size());
   Edit edit{};edit.element=owner;edit.attribute=childrenAttribute;edit.addedTrackClip=record;std::vector<Edit>edits;edits.push_back(std::move(edit));journal_.appendGroup(std::move(edits));
   graph_.elements.push_back(record->elements[0]);graph_.elements.push_back(record->elements[1]);graph_.elements[owner].attributes[childrenAttribute]=record->childrenAfter;savedArrays_=std::move(nextSavedArrays);return true;
  }catch(...){graph_.strings.resize(record->originalStringCount);throw;}
 }
 size_t undo(){return journal_.reverse([&](const Edit&e,bool){if(e.addedBookmark)removeAddedBookmark(*e.addedBookmark);else if(e.addedTrackClip)removeAddedTrackClip(*e.addedTrackClip);else if(e.removedDag)restoreDagGraphElement(*e.removedDag);else if(e.keyDeletion)restoreKeyDeletion(*e.keyDeletion);else if(e.trackClipRemoval)restoreTrackClipRemoval(*e.trackClipRemoval);else if(e.elementName)graph_.elements.at(e.element).name=e.nameBefore;else graph_.elements.at(e.element).attributes.at(e.attribute).values.at(e.valueIndex)=e.before;});}
 size_t redo(){return journal_.forward([&](const Edit&e,bool){if(e.addedBookmark)restoreAddedBookmark(*e.addedBookmark);else if(e.addedTrackClip)restoreAddedTrackClip(*e.addedTrackClip);else if(e.removedDag){captureDagDeletion(*e.removedDag);removeDagGraphElement(*e.removedDag);}else if(e.keyDeletion)applyKeyDeletion(*e.keyDeletion);else if(e.trackClipRemoval)applyTrackClipRemoval(*e.trackClipRemoval);else if(e.elementName)graph_.elements.at(e.element).name=e.nameAfter;else graph_.elements.at(e.element).attributes.at(e.attribute).values.at(e.valueIndex)=e.after;});}
 void captureExportSnapshot(const char*path,const std::function<void(Graph&)>&decorate={})const{auto output=graph_;if(decorate)decorate(output);atomicSessionSaveAsCandidate(output,path);}
 bool saveCopy(const char*path,const std::function<void(Graph&)>&decorate={}){if(!path||!*path)return false;saveAs(path,decorate);return true;}
 void saveAs(const char*path,const std::function<void(Graph&)>&decorate={}){auto saved=saved_;for(auto&item:saved)item.second=graph_.elements.at(std::get<0>(item.first)).attributes.at(std::get<1>(item.first)).values.at(std::get<2>(item.first));auto savedArrays=savedArrays_;for(auto&item:savedArrays)item.second=graph_.elements.at(item.first.first).attributes.at(item.first.second);auto savedNames=savedNames_;for(auto&item:savedNames)item.second=graph_.strings.at(graph_.elements.at(item.first).name);auto output=graph_;if(decorate)decorate(output);atomicSessionSaveAsCandidate(output,path);saved_=std::move(saved);savedArrays_=std::move(savedArrays);savedNames_=std::move(savedNames);savedElementCount_=graph_.elements.size();journal_.forEachRecord([&](Edit&edit){if(edit.removedDag){edit.removedDag->savedAttributes.clear();edit.removedDag->savedNames.clear();}if(edit.addedBookmark&&edit.addedBookmark->element<graph_.elements.size()&&graph_.elements[edit.addedBookmark->element].id==edit.addedBookmark->bookmark.id)edit.addedBookmark->stringsCommitted=true;if(edit.addedTrackClip&&edit.addedTrackClip->firstElement+1<graph_.elements.size()&&graph_.elements[edit.addedTrackClip->firstElement].id==edit.addedTrackClip->elements[0].id)edit.addedTrackClip->stringsCommitted=true;});}
};
