#include "source_map_player_pose_candidate.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <cstdint>

struct Query {
    uint32_t clientPresent, hasPose, snapResult;
    SourceMapPlayerViewPose pose;
};
struct Reply {
    uint32_t lookup=0, read=0, snap=0, fov=0;
    SourceMapPlayerViewPose pose{};
};
static_assert(sizeof(Query)==36 && sizeof(Reply)==40);

#ifdef NATIVE_IFM_PLAYER_VIEW
#include <windows.h>
#include "native_private_world_image_fixture.h"

const Query* query=nullptr;
Reply* observed=nullptr;
uint32_t token=0;
void* clientToken() { return query->clientPresent?&token:nullptr; }
void check(void* client) {
    if(client!=clientToken()) throw std::runtime_error("IFM used a different client for the view operation");
}
void* __fastcall localPlayer(void*,void*) {
    ++observed->lookup;
    return clientToken();
}
bool __fastcall readPlayer(void*,void*,SourceMapPlayerVector* eye,SourceMapPlayerVector* angles,void* client) {
    check(client);
    if(++observed->read!=1 || observed->snap || observed->fov) throw std::runtime_error("unexpected IFM view read");
    if(!query->hasPose) return false;
    *eye=query->pose.eye;
    *angles=query->pose.angles;
    return true;
}
bool __fastcall snapPlayer(void*,void*,const SourceMapPlayerVector* eye,const SourceMapPlayerVector* angles,void* client) {
    check(client);
    if(observed->read!=1 || ++observed->snap!=1 || observed->fov) throw std::runtime_error("unexpected IFM view snap");
    observed->pose={*eye,*angles};
    return query->snapResult!=0;
}
bool __fastcall setFov(void*,void*,uint32_t fov,void* client) {
    check(client);
    if(observed->snap!=1 || observed->fov) throw std::runtime_error("unexpected IFM FOV change");
    observed->fov=fov;
    return true;
}
Reply run(const Query& input) {
    std::array<uintptr_t,0x60/4> clientTable{};
    std::array<uintptr_t,5> serverTable{};
    clientTable[0x5c/4]=reinterpret_cast<uintptr_t>(localPlayer);
    serverTable[0x8/4]=reinterpret_cast<uintptr_t>(snapPlayer);
    serverTable[0xc/4]=reinterpret_cast<uintptr_t>(readPlayer);
    serverTable[0x10/4]=reinterpret_cast<uintptr_t>(setFov);
    auto* client=clientTable.data();
    auto* server=serverTable.data();
    *reinterpret_cast<uintptr_t**>(0x10fd43f4)=reinterpret_cast<uintptr_t*>(&client);
    *reinterpret_cast<uintptr_t**>(0x10fd43f0)=reinterpret_cast<uintptr_t*>(&server);
    Reply reply{};
    query=&input;
    observed=&reply;
    reinterpret_cast<void(__thiscall*)(void*)>(0x10276850)(nullptr);
    return reply;
}
#else
Reply run(const Query& input) {
    Reply reply{};
    ++reply.lookup;
    sourceResetMapPlayerView([&]() -> std::optional<SourceMapPlayerViewPose> {
        ++reply.read;
        return input.hasPose?std::optional<SourceMapPlayerViewPose>(input.pose):std::nullopt;
    },[&](const auto& pose){
        ++reply.snap;
        reply.pose=pose;
        return input.snapResult!=0;
    },[&](uint32_t fov){reply.fov=fov;});
    return reply;
}
#endif

int main(int argc,char** argv) {
    try {
        if(argc!=4) throw std::runtime_error("expected query, output and reference paths");
        std::ifstream input(argv[1],std::ios::binary);
        std::ofstream output(argv[2],std::ios::binary);
        if(!input || !output) throw std::runtime_error("IFM view fixture I/O");
#ifdef NATIVE_IFM_PLAYER_VIEW
        NativePrivateWorldImageFixture image(argv[3],0xfd43f8);
        image.seal();
#endif
        Query item{};
        unsigned count=0;
        while(input.read(reinterpret_cast<char*>(&item),sizeof(item))) {
            const auto reply=run(item);
            output.write(reinterpret_cast<const char*>(&reply),sizeof(reply));
            ++count;
        }
        if(!count || !input.eof() || input.gcount()!=0 || !output) throw std::runtime_error("invalid IFM view test stream");
#ifdef NATIVE_IFM_PLAYER_VIEW
        image.verify();
#endif
        std::cout << "IFM view queries=" << count << '\n';
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
