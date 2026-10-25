#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
// Append-only owned form of datacache's sequence-name RB tree. Equal keys go
// RIGHT on insertion, but lookup returns the first equal node encountered from
// the current root. Rotations therefore matter for duplicate-label selection.
// Keys must already be checked/folded ASCII sequence labels. No native pointers,
// allocator ABI, removal, shared cache lifetime or concurrency is represented.
class SourceSequenceNameCacheCandidate {
    struct Node {std::string key;uint32_t value;int32_t left=-1,right=-1,parent=-1;bool red=true;};
    std::vector<Node>nodes;int32_t root=-1;
    bool red(int32_t i)const{return i>=0&&nodes.at(size_t(i)).red;}
    void left(int32_t x){const auto y=nodes.at(size_t(x)).right;if(y<0)throw std::runtime_error("sequence cache left rotation");
        auto&xn=nodes[size_t(x)];auto&yn=nodes[size_t(y)];xn.right=yn.left;if(yn.left>=0)nodes[size_t(yn.left)].parent=x;yn.parent=xn.parent;
        if(xn.parent<0)root=y;else if(x==nodes[size_t(xn.parent)].left)nodes[size_t(xn.parent)].left=y;else nodes[size_t(xn.parent)].right=y;yn.left=x;xn.parent=y;
    }
    void right(int32_t x){const auto y=nodes.at(size_t(x)).left;if(y<0)throw std::runtime_error("sequence cache right rotation");
        auto&xn=nodes[size_t(x)];auto&yn=nodes[size_t(y)];xn.left=yn.right;if(yn.right>=0)nodes[size_t(yn.right)].parent=x;yn.parent=xn.parent;
        if(xn.parent<0)root=y;else if(x==nodes[size_t(xn.parent)].right)nodes[size_t(xn.parent)].right=y;else nodes[size_t(xn.parent)].left=y;yn.right=x;xn.parent=y;
    }
public:
    std::optional<uint32_t>find(const std::string&key)const{
        int32_t at=root;size_t steps=0;while(at>=0){if(++steps>nodes.size())throw std::runtime_error("sequence cache cycle");const auto&n=nodes.at(size_t(at));if(key<n.key)at=n.left;else if(n.key<key)at=n.right;else return n.value;}return {};
    }
    void insert(std::string key,uint32_t value){
        if(key.empty()||nodes.size()>=4096)throw std::runtime_error("sequence cache key/count");int32_t parent=-1,at=root;size_t steps=0;
        while(at>=0){if(++steps>nodes.size())throw std::runtime_error("sequence cache insertion cycle");parent=at;at=key<nodes[size_t(at)].key?nodes[size_t(at)].left:nodes[size_t(at)].right;}
        const int32_t inserted=int32_t(nodes.size());nodes.push_back(Node{std::move(key),value,-1,-1,parent,true});
        if(parent<0)root=inserted;else if(nodes[size_t(inserted)].key<nodes[size_t(parent)].key)nodes[size_t(parent)].left=inserted;else nodes[size_t(parent)].right=inserted;
        int32_t x=inserted;
        while(x!=root&&red(nodes[size_t(x)].parent)){
            const auto p=nodes[size_t(x)].parent,g=nodes[size_t(p)].parent;if(g<0)throw std::runtime_error("sequence cache red root");
            if(p==nodes[size_t(g)].left){const auto uncle=nodes[size_t(g)].right;
                if(red(uncle)){nodes[size_t(p)].red=false;nodes[size_t(uncle)].red=false;nodes[size_t(g)].red=true;x=g;}
                else {if(x==nodes[size_t(p)].right){x=p;left(x);}const auto p2=nodes[size_t(x)].parent,g2=nodes[size_t(p2)].parent;nodes[size_t(p2)].red=false;nodes[size_t(g2)].red=true;right(g2);}
            }else {const auto uncle=nodes[size_t(g)].left;
                if(red(uncle)){nodes[size_t(p)].red=false;nodes[size_t(uncle)].red=false;nodes[size_t(g)].red=true;x=g;}
                else {if(x==nodes[size_t(p)].left){x=p;right(x);}const auto p2=nodes[size_t(x)].parent,g2=nodes[size_t(p2)].parent;nodes[size_t(p2)].red=false;nodes[size_t(g2)].red=true;left(g2);}
            }
        }nodes[size_t(root)].red=false;
    }
};
