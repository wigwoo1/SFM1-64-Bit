#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include "read_binary_asset_candidate.h"
#include "source_map_door_collision_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"

template<class T> T read(std::istream& input) {
    T value{};
    if (!input.read(reinterpret_cast<char*>(&value), sizeof(value))) throw std::runtime_error("incomplete runtime fixture");
    return value;
}
struct Tick {
    float now, duration;
    uint32_t stepping;
    int32_t credits;
    uint32_t operations;
};
struct Operation { uint32_t type, first, second; };
struct Reply {
    uint32_t touching, pending, collisionMask;
    int32_t credits;
    std::array<uint32_t,32> events{};
    std::array<float,11> motion{};
    uint32_t state;
};
static_assert(sizeof(Tick)==20 && sizeof(Operation)==12 && sizeof(Reply)==192);

int main(int argc, char** argv) {
    try {
        if (argc==3 && std::string_view(argv[1])=="--inspect") {
            const auto scene=readSourceBspMapDoorsCandidate(readBinaryAssetCandidate(argv[2]));
            std::cout << "unsupported " << scene.unsupportedDoors << ' ' << scene.unsupportedTriggers << '\n';
            for(const auto& door:scene.doors) {
                std::cout << "door " << door.model << ' ' << door.outputs.size() << std::hexfloat;
                for(float value:door.spawn.closedOrigin) std::cout << ' ' << value;
                for(float value:door.spawn.openOrigin) std::cout << ' ' << value;
                std::cout << ' ' << door.spawn.speed << '\n';
                for(const auto& out:door.outputs) std::cout << out.key << ' ' << out.value << '\n';
            }
            for(const auto& trigger:scene.triggers) {
                std::cout << "trigger " << trigger.model << ' ' << scene.doors.at(trigger.doorIndex).model
                          << ' ' << static_cast<unsigned>(trigger.close);
                for(float value:trigger.origin) std::cout << ' ' << value;
                std::cout << '\n';
            }
            return 0;
        }
        const bool collisionMode=argc==5 && std::string_view(argv[4])=="--collision";
        if (argc != 4 && !collisionMode) throw std::runtime_error("expected input, output, BSP, and optional --collision");
        const auto bytes=readBinaryAssetCandidate(argv[3]);
        const auto scene=readSourceBspMapDoorsCandidate(bytes);
        std::optional<SourceBspBrushOnlyTraceCandidate> collision;
        std::optional<SourceBspDisplacementCollisionCandidate> displacement;
        std::optional<SourcePreparedWorldRayCandidate> prepared;
        if(collisionMode) {
            collision.emplace(bytes,std::vector<uint32_t>(32768));
            displacement.emplace(bytes,0);
            prepared.emplace(*collision,displacement->meshes);
        }
        std::ifstream input(argv[1],std::ios::binary);
        std::ofstream output(argv[2],std::ios::binary);
        if(!input || !output) throw std::runtime_error("runtime fixture files unavailable");
        using Input=SourceMapTriggerInputCandidate;
        constexpr uint32_t trigger=0x10002;
        const auto cases=read<uint32_t>(input);
        for(uint32_t i=0;i<cases;++i) {
            const auto model=read<uint32_t>(input);
            const auto route=std::find_if(scene.triggers.begin(),scene.triggers.end(),
                [model](const auto& entry){return entry.model==model;});
            if(route==scene.triggers.end())throw std::runtime_error("fixture trigger is absent from BSP routes");
            SourceMapDoorRuntimeCandidate map;
            map.addActor(0x10000,true);
            map.addActor(0x10001,true);
            map.addActor(0x10005,false);
            const auto doorId=map.addDoor(scene.doors.at(route->doorIndex).spawn);
            map.addTrigger(trigger,doorId,route->close);
            std::optional<SourceMapDoorBrushCandidate> brush;
            std::vector<const SourceMapDoorBrushCandidate*> targets;
            if(collisionMode) {
                brush.emplace(*route,trigger,*collision,*prepared);
                targets.push_back(&*brush);
            }
            const auto ticks=read<uint32_t>(input);
            for(uint32_t tick=0;tick<ticks;++tick) {
                const auto step=read<Tick>(input);
                map.advanceUnobstructed(step.duration);
                Reply reply{};
                for(uint32_t n=0;n<step.operations;++n) {
                    const auto op=read<Operation>(input);
                    if(op.type==0) map.beginScan(0x10000+op.first,[](uint32_t){});
                    else if(op.type==1) map.reportContact(0x10000+op.first,0x10000+op.second,step.now);
                    else if(op.type==2) map.finishScan(0x10000+op.first,step.now);
                    else if(op.type==3 && collisionMode && op.first<32) {
                        using Point=SourceTriggerPointCandidate;
                        const auto origin=read<Point>(input);
                        const auto previous=read<Point>(input);
                        const auto bounds=read<SourceTriggerBoundsCandidate>(input);
                        const auto hull=sourceSolidTriggerSweepCandidate(origin,
                            op.second? std::optional<Point>{previous}:std::nullopt,2,true,bounds,{});
                        const auto contacts=sourceScanSolidForMapDoors(map,0x10000+op.first,hull,
                            targets,step.now,[](uint32_t){});
                        if(!contacts.empty()) reply.collisionMask |= uint32_t(1)<<op.first;
                    }
                    else throw std::runtime_error("invalid runtime operation");
                }
                map.servicePendingScans(step.now);
                reply.credits=step.credits;
                size_t delivered=0;
                map.service(step.now,step.stepping!=0,reply.credits,[&](const auto& event){
                    reply.events.at(delivered++)=1+static_cast<uint32_t>(event.input);
                    if(event.input==Input::touchTest && event.target!=trigger) throw std::runtime_error("wrong TouchTest target");
                    if(event.input!=Input::touchTest && event.target!=doorId) throw std::runtime_error("wrong door target");
                });
                reply.touching=static_cast<uint32_t>(map.touchCount(trigger));
                reply.pending=static_cast<uint32_t>(map.pendingInputs());
                const auto& door=map.door(doorId);
                for(size_t axis=0;axis<3;++axis) {
                    reply.motion[axis]=door.origin()[axis];
                    reply.motion[axis+3]=door.destination()[axis];
                    reply.motion[axis+6]=door.velocity()[axis];
                }
                reply.motion[9]=door.localTime();
                reply.motion[10]=door.moveDoneTime();
                reply.state=static_cast<uint32_t>(door.state());
                output.write(reinterpret_cast<const char*>(&reply),sizeof(reply));
            }
        }
        if(input.peek()!=EOF || !output) throw std::runtime_error("invalid runtime fixture stream");
        return 0;
    } catch(const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
