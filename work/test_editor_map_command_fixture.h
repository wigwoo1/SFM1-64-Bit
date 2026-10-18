#pragma once
#include "editor_map_command_protocol_candidate.h"
#include "map_base_name_candidate.h"
#include <string>
#include <vector>
#include <cassert>
struct EditorMapCommandFixture {
    bool pendingValue=false,valid=true;
    std::vector<unsigned> events;
    std::string command;
    bool pending() {return pendingValue;}
    const char* currentMap() {events.push_back(1);return "maps/fixture";}
    std::string extractMapBaseName(const char* p) {events.push_back(2);char buffer[256];extractMapBaseNameCandidate(p,buffer,sizeof(buffer));return buffer;}
    bool validateMap(const std::string&) {events.push_back(3);return valid;}
    void setPending(bool v) {events.push_back(4);pendingValue=v;}
    std::string formatMapCommand(const std::string& name) {assert(pendingValue);events.push_back(5);return "wait; map "+name+"\n";}
    void dispatchCommand(const std::string& text) {events.push_back(6);command=text;}
    void engineAfterCommand() {events.push_back(7);}
    void refreshEditor() {events.push_back(8);}
};
inline void testEditorMapCommandFixture() {
    EditorMapCommandFixture f;f.pendingValue=true;
    assert(!requestEditorMapCommandCandidate(nullptr,f) && f.events.empty());
    f.pendingValue=false;f.valid=false;
    assert(!requestEditorMapCommandCandidate("bad",f));
    assert((f.events==std::vector<unsigned>{2,3}) && !f.pendingValue && f.command.empty());
    f.events.clear();f.valid=true;
    assert(requestEditorMapCommandCandidate(nullptr,f));
    assert((f.events==std::vector<unsigned>{1,2,3,4,5,6,7,8}));
    assert(f.command=="wait; map fixture\n" && f.pendingValue);
    f.events.clear();assert(!requestEditorMapCommandCandidate("other",f) && f.events.empty());
    EditorMapCommandFixture path;
    assert(requestEditorMapCommandCandidate("C:\\game\\maps\\stage.bsp",path));
    assert(path.command=="wait; map stage\n");
    struct Case {const char* input;const char* expected;};
    for(auto c:{Case{"a.b.c","a.b"},Case{"dir.name/file","file"},Case{".hidden",""},
        Case{"path/",""},Case{"C:stage.bsp","C:stage"},Case{"stage","stage"},Case{nullptr,""}}) {
        char output[256];extractMapBaseNameCandidate(c.input,output,sizeof(output));assert(std::string(output)==c.expected);
    }
    char small[4];extractMapBaseNameCandidate("longname.bsp",small,sizeof(small));assert(std::string(small)=="lon");
}
