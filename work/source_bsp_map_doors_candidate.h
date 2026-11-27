#pragma once

#include "source_bsp_inline_model_name_candidate.h"
#include "source_map_door_runtime_candidate.h"
#include "vmt_keyvalues_candidate.h"

#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <sstream>
#include <string_view>

struct SourceBspMapDoorCandidate {
    size_t model;
    std::string name;
    SourceSlidingDoorSpawnCandidate spawn;
    std::vector<VmtNodeCandidate> outputs;
};

struct SourceBspMapDoorTriggerCandidate {
    size_t model, doorIndex;
    std::array<float, 3> origin;
    SourceMapDoorCloseCandidate close;
};

struct SourceBspMapDoorsCandidate {
    std::vector<SourceBspMapDoorCandidate> doors;
    std::vector<SourceBspMapDoorTriggerCandidate> triggers;
    size_t unsupportedDoors = 0, unsupportedTriggers = 0;
};

namespace sourceBspMapDoorsDetail {
using Fields = std::vector<VmtNodeCandidate>;
using Point = std::array<float, 3>;

inline std::string field(const Fields& fields, const char* key, std::string fallback = {}) {
    const std::string* value = nullptr;
    for (const auto& entry : fields) {
        if (vmtAsciiLowerCandidate(entry.key) != key) continue;
        if (value) throw std::runtime_error("duplicate BSP entity field: " + std::string(key));
        value = &entry.value;
    }
    return value ? *value : fallback;
}

template<class Number>
Number number(std::string_view raw) {
    Number value{};
    if (!raw.empty() && raw.front() == '+') raw.remove_prefix(1);
    if (raw.empty()) throw std::runtime_error("missing BSP entity number");
    const auto result = std::from_chars(raw.data(), raw.data()+raw.size(), value);
    if (result.ec != std::errc{} || result.ptr != raw.data()+raw.size() || !std::isfinite(value)) {
        throw std::runtime_error("invalid BSP entity number");
    }
    return value;
}

inline Point point(const std::string& text) {
    std::istringstream input(text);
    Point value{};
    std::string component;
    for (auto& axis : value) {
        if (!(input >> component)) throw std::runtime_error("short BSP entity vector");
        axis = number<float>(component);
    }
    if (input >> component) throw std::runtime_error("long BSP entity vector");
    return value;
}

inline std::vector<VmtNodeCandidate> outputs(const Fields& entity) {
    std::vector<VmtNodeCandidate> result;
    for (const auto& field : entity)
        if (vmtAsciiLowerCandidate(field.key).rfind("on", 0) == 0) result.push_back(field);
    return result;
}

struct Action { std::string_view target, input; };
inline std::optional<Action> action(std::string_view text) {
    // The engine also accepts ESC as a field separator to allow literal commas.
    const char separator = text.find('\x1b') == std::string_view::npos ? ',' : '\x1b';
    std::array<std::string_view, 5> fields;
    for (size_t i=0; i<fields.size()-1; ++i) {
        const auto end = text.find(separator);
        if (end == std::string_view::npos) return std::nullopt;
        fields[i] = text.substr(0, end);
        text.remove_prefix(end+1);
    }
    fields.back() = text;
    if (fields[0].empty() || fields[1].empty() || !fields[2].empty() ||
        fields[3] != "0" || fields[4] != "-1") return std::nullopt;
    return Action{fields[0], fields[1]};
}

inline std::string target(const std::vector<VmtNodeCandidate>& list, const char* event, const char* input) {
    std::string result;
    for (const auto& entry : list) {
        if (vmtAsciiLowerCandidate(entry.key) != event) continue;
        const auto parsed = action(entry.value);
        if (!parsed || !result.empty() || vmtAsciiLowerCandidate(std::string(parsed->input)) != input) return {};
        result = vmtAsciiLowerCandidate(std::string(parsed->target));
    }
    return result;
}
}

// Loads the verified zero-angle, parentless toggle doors and the supported
// client-only trigger routes. Door outputs are retained in map order for
// downstream animation and other side effects.
inline SourceBspMapDoorsCandidate readSourceBspMapDoorsCandidate(const std::vector<uint8_t>& bsp) {
    using namespace sourceBspMapDoorsDetail;
    constexpr size_t headerSize = 1036, modelSize = 48;
    if (bsp.size() < headerSize || std::memcmp(bsp.data(), "VBSP", 4)) {
        throw std::runtime_error("VBSP header required for map doors");
    }
    const auto readWord = [&](size_t index) {
        uint32_t word;
        std::memcpy(&word, bsp.data()+index, sizeof(word));
        return word;
    };
    const auto lump = [&](size_t id) {
        const auto base = 8+id*16;
        const size_t offset = readWord(base), size = readWord(base+4);
        if (offset > bsp.size() || size > bsp.size()-offset || (size && offset < headerSize) ||
            readWord(base+8) || readWord(base+12)) {
            throw std::runtime_error("unsupported or invalid VBSP map-door lump");
        }
        return std::pair{offset, size};
    };
    const auto [entityOffset, entitySize] = lump(0);
    const auto [modelOffset, modelBytes] = lump(14);
    if (modelBytes % modelSize) throw std::runtime_error("invalid VBSP map-door model table");
    std::string text(reinterpret_cast<const char*>(bsp.data()+entityOffset), entitySize);
    while (!text.empty() && text.back() == '\0') text.pop_back();
    const auto entities = VmtParserCandidate(text).parseEntityBlocks();
    const auto modelNumber = [&](const Fields& entity) {
        const auto model = sourceBspInlineModelIndexCandidate(field(entity, "model"));
        if (model >= modelBytes/modelSize) throw std::runtime_error("map door inline model outside table");
        return model;
    };
    const auto untransformed = [](const Fields& entity) {
        return field(entity, "parentname").empty() && point(field(entity, "angles", "0 0 0")) == Point{};
    };

    SourceBspMapDoorsCandidate result;
    std::map<std::string, size_t> namedDoors, nameCounts;
    for (const auto& entity : entities) {
        const auto name = field(entity, "targetname");
        if (!name.empty()) ++nameCounts[vmtAsciiLowerCandidate(name)];
        if (vmtAsciiLowerCandidate(field(entity, "classname")) != "func_door") continue;
        const uint32_t flags = number<uint32_t>(field(entity, "spawnflags", "0"));
        constexpr uint32_t toggle = 32, supportedFlags = toggle | 1 | 2048;
        if (!untransformed(entity) || !(flags & toggle) || (flags & ~supportedFlags) ||
            number<int32_t>(field(entity, "forceclosed", "0")) != 0 ||
            number<float>(field(entity, "dmg", "0")) != 0.f) {
            ++result.unsupportedDoors;
            continue;
        }
        const auto model = modelNumber(entity);
        std::array<float, 6> bounds;
        std::memcpy(bounds.data(), bsp.data()+modelOffset+model*modelSize, sizeof(bounds));
        for (size_t axis=0; axis<3; ++axis) {
            if (!(bounds[axis] <= bounds[axis+3])) throw std::runtime_error("invalid BSP door model bounds");
            bounds[axis] -= 1.f;
            bounds[axis+3] += 1.f;
        }
        const auto spawn = sourceSlidingDoorSpawnCandidate(point(field(entity, "origin", "0 0 0")),
            point(field(entity, "movedir", "0 0 0")), bounds, number<float>(field(entity, "lip", "0")),
            number<float>(field(entity, "speed", "0")), number<int32_t>(field(entity, "spawnpos", "0")), flags);
        if (spawn.speed <= 0.f) {
            ++result.unsupportedDoors;
            continue;
        }
        if (!name.empty()) namedDoors.emplace(vmtAsciiLowerCandidate(name), result.doors.size());
        result.doors.push_back({model, name, spawn, outputs(entity)});
    }

    for (const auto& entity : entities) {
        if (vmtAsciiLowerCandidate(field(entity, "classname")) != "trigger_multiple") continue;
        const auto list = outputs(entity);
        const auto open = target(list, "onstarttouch", "open");
        const auto door = namedDoors.find(open);
        const auto name = vmtAsciiLowerCandidate(field(entity, "targetname"));
        const bool direct = list.size() == 2 && target(list, "onendtouchall", "close") == open;
        const bool test = list.size() == 3 && !name.empty() && nameCounts[name] == 1 &&
            target(list, "onendtouch", "touchtest") == name && target(list, "onnottouching", "close") == open;
        if (!untransformed(entity) || door == namedDoors.end() || nameCounts[open] != 1 || (!direct && !test) ||
            !field(entity, "filtername").empty() || number<int32_t>(field(entity, "startdisabled", "0")) ||
            number<uint32_t>(field(entity, "spawnflags", "0")) != 1 ||
            number<float>(field(entity, "wait", "-1")) != 0.f) {
            ++result.unsupportedTriggers;
            continue;
        }
        result.triggers.push_back({modelNumber(entity), door->second, point(field(entity, "origin", "0 0 0")),
            direct ? SourceMapDoorCloseCandidate::endTouchAll : SourceMapDoorCloseCandidate::testAfterEndTouch});
    }
    return result;
}
