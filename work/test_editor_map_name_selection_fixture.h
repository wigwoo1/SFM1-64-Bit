#pragma once
#include "editor_map_name_selection_candidate.h"
#include <vector>
#include <string>
#include <cassert>
struct EditorMapNameSelectionFixture {
    struct Object {const char* name;};
    Object primary{"maps/project.bsp"},secondary{"maps/secondary.bsp"};
    bool primaryPresent=true,fallbackPresent=true;
    unsigned resolutions=0,fallbacks=0;
    std::vector<unsigned> events;
    Object* resolvePrimary() {++resolutions;return primaryPresent?&primary:nullptr;}
    const char* mapName(Object* object) {assert(object);return object->name;}
    Object* fallbackObject() {++fallbacks;return fallbackPresent?&secondary:nullptr;}
    const char* emptyName() {return "";}
    const char* engineMap() {events.push_back(1);return "maps/engine.bsp";}
    const char* projectMap() {events.push_back(2);return projectMapNameCandidate(*this);}
};
inline void testEditorMapNameSelectionFixture() {
    EditorMapNameSelectionFixture f;char result[256];
    selectEditorMapNameCandidate("maps/supplied.bsp",result,sizeof(result),f);
    assert(std::string(result)=="supplied" && (f.events==std::vector<unsigned>{1}) && f.resolutions==0);
    f.events.clear();selectEditorMapNameCandidate(nullptr,result,sizeof(result),f);
    assert(std::string(result)=="project" && (f.events==std::vector<unsigned>{1,2}) && f.resolutions==2 && f.fallbacks==0);
    f.primary.name="";selectEditorMapNameCandidate("",result,sizeof(result),f);
    assert(std::string(result)=="secondary" && f.fallbacks==1);
    f.primaryPresent=false;selectEditorMapNameCandidate(nullptr,result,sizeof(result),f);
    assert(std::string(result)=="engine" && f.fallbacks==1); // Missing primary skips fallback object.
    f.primaryPresent=true;f.secondary.name=nullptr;
    selectEditorMapNameCandidate(nullptr,result,sizeof(result),f);assert(std::string(result)=="engine");
    f.fallbackPresent=false;
    selectEditorMapNameCandidate(nullptr,result,sizeof(result),f);assert(std::string(result)=="engine");
}
