#include "graph_bookmark_update_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{int reference=1,log=2,element=3,components=35,type=10;bool missingReference=false,missingLog=false,missingElement=false;unsigned mask=5,profiles=0,frees=0;std::vector<unsigned> bookmarks[35];std::vector<int> changed;
 void beginProfile(){++profiles;}void endProfile(){++profiles;}int* findGraphCurveReference(){return missingReference?nullptr:&reference;}int* logFromReference104cd3e0(int*){return missingLog?nullptr:&log;}
 int componentCountSlot62(int*){return components;}int* resolveLiveHandle5c(){return missingElement?nullptr:&element;}int elementTypeSlot36(int*){return type;}unsigned componentMask70(){return mask;}int sourceCount84(int){return 2;}
 Adapter& bookmarkView(int*){return *this;}int bookmarkCount105079b0(int c){return int(bookmarks[c].size());}unsigned sourceMappedTime(int c,int i){return i?unsigned(c):0x80000000;}unsigned bookmarkTime10517210(int i,int c){return bookmarks[c][i];}
 std::vector<unsigned> allocateTemporaryTimes(int count){std::vector<unsigned> v;v.reserve(count);return v;}void appendTemporaryTime(std::vector<unsigned>& v,unsigned t){v.push_back(t);}
 void replaceBookmarks10517340(int*,int c,const std::vector<unsigned>& v){bookmarks[c]=v;changed.push_back(c);}void releaseTemporaryTimes(std::vector<unsigned>&){++frees;}
};
int main(){Adapter a;graphBookmarkUpdateCandidate(a);assert((a.changed==std::vector<int>{0,2,32,34}) && a.frees==4 && a.profiles==2);a.changed.clear();graphBookmarkUpdateCandidate(a);assert(a.changed.empty() && a.frees==4);Adapter b;b.type=3;graphBookmarkUpdateCandidate(b);assert((b.changed==std::vector<int>{0}));for(int mode=0;mode<3;++mode){Adapter c;c.missingReference=mode==0;c.missingLog=mode==1;c.missingElement=mode==2;graphBookmarkUpdateCandidate(c);assert(c.changed.empty() && c.profiles==2);}std::cout<<"x64 bookmark updater: component masks and rotation, scalar gate, changed/equal times, temporary lifetime and null paths passed\n";}
