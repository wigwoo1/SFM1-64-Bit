#pragma once
// Original cache clears1035E640/680/6C0/700/740: capture next before release,
// release head-to-tail, and clear head only after every release callback.
template<class Node,class Next,class Release>
void clearRecoveredReferenceCacheListCandidate(Node*& head,Next next,Release release){
 for(auto* node=head;node;){auto* following=next(node);release(node);node=following;}head=nullptr;
}
