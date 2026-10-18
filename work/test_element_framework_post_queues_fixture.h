#pragma once
#include "element_framework_post_queues_candidate.h"
#include <vector>
#include <cassert>
struct FrameworkPostQueuesFixture {
    struct Element {unsigned id;};
    Element elements[4]{{1},{2},{3},{4}};
    std::vector<unsigned> posts{1,2,99},callbacks{10},resets{1},events;
    bool mutate=true;
    auto postCount() {return static_cast<std::int32_t>(posts.size());}
    auto* postData() {return posts.data();}
    void clearPostCount() {posts.clear();}
    auto callbackCount() {return static_cast<std::int32_t>(callbacks.size());}
    auto* callbackData() {return callbacks.data();}
    auto resetCount() {return static_cast<std::int32_t>(resets.size());}
    auto* resetData() {return resets.data();}
    void clearResetCount() {resets.clear();}
    Element* resolve(unsigned h) {return h>=1 && h<=4?&elements[h-1]:nullptr;}
    void postUpdate(Element* e) {
        events.push_back(e->id);
        if(mutate && e->id==1) {
            posts[1]=3;posts.push_back(4);
            callbacks.push_back(20);resets.push_back(2);
        }
    }
    void invoke(unsigned callback) {
        events.push_back(callback);
        if(mutate && callback==10) {
            callbacks[1]=30;callbacks.push_back(40);resets.push_back(3);
        }
    }
    void reset(Element* e) {
        events.push_back(100+e->id);
        if(mutate && e->id==1) {resets[1]=4;resets.push_back(2);}
    }
};
inline void testElementFrameworkPostQueuesFixture() {
    FrameworkPostQueuesFixture f;
    processElementFrameworkPostQueuesCandidate(f);
    assert((f.events==std::vector<unsigned>{1,3,10,30,101,104,103}));
    assert(f.posts.empty() && f.resets.empty());
    assert((f.callbacks==std::vector<unsigned>{10,30,40}));
    f.events.clear();f.mutate=false;
    processElementFrameworkPostQueuesCandidate(f);
    assert((f.events==std::vector<unsigned>{10,30,40}));
}
