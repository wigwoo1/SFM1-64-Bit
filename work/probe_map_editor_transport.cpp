#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_map_editor_transport_candidate.h"

struct Query {
    int32_t playhead, rangeStart, rangeEnd;
    uint32_t playing, sync, recording, otherMode, rewind, entryPoint;
};
struct Event { uint32_t kind, playing; int32_t value; };
struct Reply { int32_t playhead; float speed; uint32_t playing, count; std::array<Event, 4> events; };
static_assert(sizeof(Query) == 36 && sizeof(Reply) == 64);

void append(Reply& reply, uint32_t kind, bool playing, int32_t value = 0) {
    if (reply.count == reply.events.size()) throw std::runtime_error("transport event overflow");
    reply.events[reply.count++] = {kind, playing, value};
}

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"

template<class T> T& cell(uintptr_t address) { return *reinterpret_cast<T*>(address); }
const Query* current = nullptr;
Reply* reply = nullptr;
uintptr_t editor = 0;
void* __fastcall noFilmElement(void*, void*, int32_t) { return nullptr; }
void __fastcall notifyPlayback(void*, void*) {
    append(*reply, 4, cell<uint8_t>(editor + 0x69c) != 0);
}
void __fastcall notifyPlayhead(void*, void*, int32_t time, uint32_t scrubbing) {
    if (static_cast<uint8_t>(scrubbing) != 0)
        throw std::runtime_error("unexpected ordinary playback notification flag");
    append(*reply, 3, cell<uint8_t>(editor + 0x69c) != 0, time);
}
uint32_t profileBegins = 0, profileEnds = 0;
void __fastcall beginProfile(void*, void*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    ++profileBegins;
}
void __fastcall endProfile(void*, void*) { ++profileEnds; }
void boundary(unsigned slot, CONTEXT& c) {
    using Observer = PrivateX86BoundaryObserverFixture;
    const bool playing = cell<uint8_t>(editor + 0x69c) != 0;
    const auto first = cell<uint32_t>(c.Esp + 4);
    switch (slot) {
    case 0:
        if (c.Ecx != editor) throw std::runtime_error("unexpected sync owner");
        append(*reply, 1, playing, static_cast<uint8_t>(first) != 0);
        Observer::returnCdecl(c);
        c.Esp += 4;
        break;
    case 1:
        if (c.Ecx != editor) throw std::runtime_error("unexpected range owner");
        cell<int32_t>(first) = current->rangeStart;
        cell<int32_t>(cell<uint32_t>(c.Esp + 8)) = current->rangeEnd;
        Observer::returnCdecl(c);
        c.Esp += 8;
        break;
    case 2:
        if (c.Ecx != editor) throw std::runtime_error("unexpected seek owner");
        append(*reply, 2, playing, static_cast<int32_t>(first));
        cell<int32_t>(editor + 0x684) = static_cast<int32_t>(first);
        Observer::returnCdecl(c);
        c.Esp += 20;
        break;
    case 3: throw std::runtime_error("unexpected transport boundary");
    }
}

Reply run(const Query& query) {
    alignas(4) std::array<uint8_t, 0x6d0> document{};
    alignas(4) std::array<uint8_t, 0x250> viewport{};
    alignas(4) std::array<uint8_t, 0x4f4> listeners{};
    std::array<uint8_t, 0x202> recorder{};
    std::array<uintptr_t, 0x48 / 4 + 1> modelTable{};
    modelTable[0x48 / 4] = reinterpret_cast<uintptr_t>(noFilmElement);
    const uintptr_t listenerTable[] = {reinterpret_cast<uintptr_t>(notifyPlayback)};
    const uintptr_t listener = reinterpret_cast<uintptr_t>(listenerTable);
    const uintptr_t listenerList[] = {reinterpret_cast<uintptr_t>(&listener)};
    const uintptr_t timeListenerTable[] = {reinterpret_cast<uintptr_t>(notifyPlayhead)};
    const uintptr_t timeListener = reinterpret_cast<uintptr_t>(timeListenerTable);
    const uintptr_t timeListenerList[] = {reinterpret_cast<uintptr_t>(&timeListener)};
    const uintptr_t dataModel = reinterpret_cast<uintptr_t>(modelTable.data());
    editor = reinterpret_cast<uintptr_t>(document.data());
    cell<int32_t>(editor + 0x670) = -1;
    cell<int32_t>(editor + 0x684) = query.playhead;
    cell<uint32_t>(editor + 0x494) = !query.rewind;
    cell<uint8_t>(editor + 0x69c) = static_cast<uint8_t>(query.playing);
    cell<uint8_t>(editor + 0x6c0) = static_cast<uint8_t>(query.sync);
    cell<uint8_t>(editor + 0x228) = static_cast<uint8_t>(query.recording);
    cell<uint8_t>(editor + 0x2a8) = static_cast<uint8_t>(query.otherMode);
    cell<float>(editor + 0x6a0) = 3.f;
    cell<uintptr_t>(reinterpret_cast<uintptr_t>(viewport.data()) + 0x248) = editor;
    const auto tool = reinterpret_cast<uintptr_t>(listeners.data());
    cell<uintptr_t>(tool) = 0x10aeb0f4;
    cell<uintptr_t>(tool + 0x344) = editor;
    cell<uintptr_t>(tool + 0x3c0) = reinterpret_cast<uintptr_t>(listenerList);
    cell<int32_t>(tool + 0x3cc) = 1;
    cell<uintptr_t>(tool + 0x370) = reinterpret_cast<uintptr_t>(timeListenerList);
    cell<int32_t>(tool + 0x37c) = 1;
    cell<uintptr_t>(0x10f32568) = tool;
    cell<uintptr_t>(0x10f2ba04) = reinterpret_cast<uintptr_t>(recorder.data());
    cell<uintptr_t>(0x10e9a51c) = reinterpret_cast<uintptr_t>(&dataModel);
    cell<uintptr_t>(0x10fd4398) = 0; // Fail if this route reaches the engine tool.
    cell<uintptr_t>(0x10a7e460) = reinterpret_cast<uintptr_t>(beginProfile);
    cell<uintptr_t>(0x10a7e48c) = reinterpret_cast<uintptr_t>(endProfile);
    cell<uintptr_t>(0x10a7e45c) = 0;
    profileBegins = profileEnds = 0;
    Reply result{};
    current = &query;
    reply = &result;
    if (query.entryPoint) {
        reinterpret_cast<void (__thiscall*)(void*, const char*)>(0x10209c10)(viewport.data(), "OnPlayPause");
    } else {
        reinterpret_cast<void (__thiscall*)(void*)>(0x10026f70)(listeners.data());
    }
    reply = nullptr;
    current = nullptr;
    result.playhead = cell<int32_t>(editor + 0x684);
    result.speed = cell<float>(editor + 0x6a0);
    result.playing = cell<uint8_t>(editor + 0x69c);
    const auto notifications = static_cast<uint32_t>(std::count_if(
        result.events.begin(), result.events.begin() + result.count,
        [](const Event& event) { return event.kind == 3; }));
    if (profileBegins != notifications || profileEnds != notifications)
        throw std::runtime_error("playhead dispatcher profiling imbalance");
    return result;
}
#else
struct Transport {
    bool playing, frameSyncPreferred, recording, otherPlaybackMode, rewindAtEnd;
    int32_t playhead, rangeStart, rangeEnd;
    float playbackSpeed = 3.f;
    Reply reply{};

    void sync(bool enable) { append(reply, 1, playing, enable); }
    void seek(int32_t position) { append(reply, 2, playing, position); playhead = position; }
    void playheadChanged() { append(reply, 3, playing, playhead); }
    void playbackChanged() { append(reply, 4, playing); }
};

Reply run(const Query& query) {
    Transport state{query.playing != 0, query.sync != 0, query.recording != 0, query.otherMode != 0,
                    query.rewind != 0, query.playhead, query.rangeStart, query.rangeEnd};
    sourceMapToggleEditorPlayback(state, {24, 10000});
    state.reply.playhead = state.playhead;
    state.reply.speed = state.playbackSpeed;
    state.reply.playing = state.playing;
    return state.reply;
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("expected query, output and reference paths");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        if (!input || !output) throw std::runtime_error("transport fixture I/O");
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[3], 0xfd43f8);
        image.seal();
        PrivateX86BoundaryObserverFixture observer{
            {0x1029a030, 0x1029ac20, 0x102b2de0, 0x10002800}, boundary};
        observer.configure(true);
#endif
        Query item{};
        while (input.read(reinterpret_cast<char*>(&item), sizeof(item))) {
            const auto result = run(item);
            output.write(reinterpret_cast<const char*>(&result), sizeof(result));
        }
        if (!input.eof() || input.gcount() || !output) throw std::runtime_error("incomplete transport stream");
#ifndef _WIN64
        observer.configure(false);
        image.verify();
#endif
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
