#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_map_frame_sync_candidate.h"

struct Query {
    uint32_t suspended, hasSync, hasRate, enabled, requested, document, attribute, acceptRates;
    float documentRate, initialRate;
};
struct Event { uint32_t kind; float value; };
struct Reply { uint32_t preference, calls; std::array<Event, 6> events; };
static_assert(sizeof(Query) == 40 && sizeof(Reply) == 56);

struct Host {
    const Query& query;
    bool enabled = query.enabled != 0;
    float rate = query.initialRate;
    Reply reply{};

    void record(uint32_t kind, float value) {
        if (reply.calls == reply.events.size()) throw std::runtime_error("too many frame sync calls");
        reply.events[reply.calls++] = {kind, value};
    }
    bool hasSyncVariable() const { return query.hasSync != 0; }
    bool hasRateVariable() const { return query.hasRate != 0; }
    bool syncEnabled() const { return enabled; }
    float frameRate() const { return rate; }
    std::optional<float> documentRate() const {
        if (query.document && query.attribute) return query.documentRate;
        return std::nullopt;
    }
    void setSyncEnabled(bool value) {
        enabled = value;
        record(1, value ? 1.f : 0.f);
    }
    void setFrameRate(float value) {
        if (query.acceptRates) rate = value;
        record(2, value);
    }
    void notice(SourceMapFrameSyncNotice kind, float value) {
        record(3 + static_cast<uint32_t>(kind), value);
    }
};

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
#include <cstdarg>

template<class T> T& installed(uint32_t address) { return *reinterpret_cast<T*>(address); }
Host* observed = nullptr;
float documentValue = 0.f;
int documentToken, attributeToken;
struct Variable {
    const uintptr_t* vtable;
    std::array<uint8_t, 0x28> prefix{};
    float floatValue = 0.f;
    int intValue = 0;
};
static_assert(offsetof(Variable, floatValue) == 0x2c && offsetof(Variable, intValue) == 0x30);

void __fastcall setSync(Variable* self, void*, int value) {
    self->intValue = value;
    observed->setSyncEnabled(value != 0);
}
void __fastcall setRate(Variable* self, void*, float value) {
    observed->setFrameRate(value);
    self->floatValue = observed->rate;
}
void __cdecl logNotice(const char* format, ...) {
    const auto address = reinterpret_cast<uintptr_t>(format);
    if (address == 0x10ae5228) {
        observed->notice(SourceMapFrameSyncNotice::disabled, 0.f);
        return;
    }
    if (address != 0x10ae5270 && address != 0x10ae5248)
        throw std::runtime_error("unexpected frame sync message");
    va_list args;
    va_start(args, format);
    const auto rate = static_cast<float>(va_arg(args, double));
    va_end(args);
    observed->notice(address == 0x10ae5270 ? SourceMapFrameSyncNotice::enabled
                                          : SourceMapFrameSyncNotice::changedRate, rate);
}
void boundary(unsigned slot, CONTEXT& c) {
    using Observer = PrivateX86BoundaryObserverFixture;
    if (slot == 0) {
        Observer::returnCdecl(c, observed->query.document ? reinterpret_cast<uintptr_t>(&documentToken) : 0);
    } else if (slot == 1) {
        if (c.Ecx != reinterpret_cast<uintptr_t>(&documentToken))
            throw std::runtime_error("unexpected document in frame sync attribute lookup");
        Observer::returnCdecl(c, observed->query.attribute ? reinterpret_cast<uintptr_t>(&attributeToken) : 0);
        c.Esp += 4;
    } else if (slot == 2) {
        if (c.Ecx != reinterpret_cast<uintptr_t>(&attributeToken))
            throw std::runtime_error("unexpected frame sync rate attribute");
        Observer::returnCdecl(c, reinterpret_cast<uintptr_t>(&documentValue));
    } else {
        throw std::runtime_error("unexpected observer slot");
    }
}

Reply run(const Query& query) {
    alignas(4) std::array<uint8_t, 0x6d0> editor{};
    editor[0x69c] = static_cast<uint8_t>(query.suspended);
    editor[0x6c0] = static_cast<uint8_t>(!query.requested);
    Host host{query};
    observed = &host;
    documentValue = query.documentRate;
    const uintptr_t table[]{0, reinterpret_cast<uintptr_t>(setSync), reinterpret_cast<uintptr_t>(setRate)};
    Variable sync{table}, rate{table};
    sync.intValue = query.enabled != 0;
    rate.floatValue = query.initialRate;
    auto* invalid = reinterpret_cast<Variable*>(0x10fd2e60);
    installed<Variable*>(0x10f3239c) = query.hasSync ? &sync : invalid;
    installed<Variable*>(0x10f323a0) = &sync;
    installed<Variable*>(0x10f32394) = query.hasRate ? &rate : invalid;
    installed<Variable*>(0x10f32398) = &rate;
    installed<uint32_t>(0x10f323a4) = 3; // Already looked up both cvars.
    installed<decltype(&logNotice)>(0x10a7e468) = logNotice;
    reinterpret_cast<void (__thiscall*)(void*, bool)>(0x1029fe90)(editor.data(), query.requested != 0);
    host.reply.preference = editor[0x6c0];
    observed = nullptr;
    return host.reply;
}
#else
Reply run(const Query& query) {
    Host host{query};
    host.reply.preference = query.requested != 0;
    sourceMapApplyFrameSync(query.requested != 0, query.suspended != 0, host);
    return host.reply;
}
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("expected query, output and reference paths");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        if (!input || !output) throw std::runtime_error("frame sync fixture I/O");
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[3], 0xfd43f8);
        image.seal();
        PrivateX86BoundaryObserverFixture observer{
            {0x1028bee0, 0x1035dd50, 0x100276d0, 0x10002600}, boundary};
        observer.configure(true);
#endif
        Query query{};
        while (input.read(reinterpret_cast<char*>(&query), sizeof(query))) {
            const auto reply = run(query);
            output.write(reinterpret_cast<const char*>(&reply), sizeof(reply));
        }
        if (!input.eof() || input.gcount() || !output) throw std::runtime_error("incomplete frame sync stream");
#ifndef _WIN64
        observer.configure(false);
        image.verify();
#endif
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
