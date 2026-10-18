#pragma once
inline int sourceAsciiNameCompareCandidate(const char* left,const char* right) {
    for (;;) {
        unsigned a=static_cast<unsigned char>(*left++);
        unsigned b=static_cast<unsigned char>(*right++);
        if (a==b) {if (!a) return 0; continue;}
        if (!b) return static_cast<int>(a);
        if (a>='A' && a<='Z') a+=32;
        if (b>='A' && b<='Z') b+=32;
        if (a!=b) return static_cast<int>(a)-static_cast<int>(b);
    }
}
// Missing original name sentinel adapted to nullptr by graph caller.
inline int controlGroupNameComparatorCandidate(const char* first,const char* second) {
    return sourceAsciiNameCompareCandidate(second?second:"",first?first:"");
}
inline int controlGroupForwardNameComparatorCandidate(const char* first,const char* second) {
    return sourceAsciiNameCompareCandidate(first?first:"",second?second:"");
}
inline int controlGroupSortNameComparatorCandidate(const char* first,const char* second,bool forward) {
    return forward ? controlGroupForwardNameComparatorCandidate(first,second)
                   : controlGroupNameComparatorCandidate(first,second);
}
