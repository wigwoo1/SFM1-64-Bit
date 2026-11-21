#pragma once

#include "source_particle_definition_candidate.h"
#include <array>
#include <optional>
#include <string_view>

// Only operator classes whose mask contribution has been checked are
// admitted. The definition and each child contribute to the collection mask.
inline std::optional<uint64_t> sourceParticleControlPointMaskCandidate(
        const Graph& graph, uint32_t definition) {
    static constexpr std::array<std::string_view, 20> zeroMask{
        "render_animated_sprites", "render_sprite_trail", "alpha fade and decay", "lifespan decay",
        "color fade", "movement basic", "movement lock to control point", "oscillate vector",
        "radius scale", "alpha random", "color random", "lifetime random",
        "position within sphere random", "radius random", "rotation random",
        "rotation yaw flip random", "sequence random", "trail length random",
        "lifetime from sequence", "collision via traces"
    };
    uint64_t mask = 0;
    std::set<uint32_t> active, complete;
    const auto visit = [&](auto&& self, uint32_t id) -> bool {
        if (id >= graph.elements.size() || active.size() >= 64 || active.count(id)) return false;
        if (complete.count(id)) return true;
        active.insert(id);
        const auto& element = graph.elements[id];
        if (graph.strings.at(element.type) != "DmeParticleSystemDefinition") return false;
        for (const auto* group : {"renderers", "operators", "initializers", "emitters", "forces", "constraints"}) {
            const auto* slots = sourceParticleAttributeCandidate(graph, element, group, 15);
            if (!slots) continue;
            for (const auto& slot : slots->values) {
                if (slot.reference >= graph.elements.size()) return false;
                const auto& op = graph.elements[slot.reference];
                const auto* function = sourceParticleAttributeCandidate(graph, op, "functionName", 5);
                if (!function) return false;
                const auto name = vmtAsciiLowerCandidate(function->values.front().text);
                if (name == "emit_continuously") {
                    const auto* scale = sourceParticleAttributeCandidate(
                        graph, op, "emission count scale control point", 2);
                    const auto index = scale ? wireInt(scale->values.front()) : -1;
                    if (index >= 64) return false;
                    if (index >= 0) mask |= uint64_t{1} << index;
                } else if (std::find(zeroMask.begin(), zeroMask.end(), name) == zeroMask.end()) {
                    return false;
                }
            }
        }
        if (const auto* children = sourceParticleAttributeCandidate(graph, element, "children", 15)) {
            for (const auto& entry : children->values) {
                if (entry.reference >= graph.elements.size()) return false;
                const auto& child = graph.elements[entry.reference];
                if (graph.strings.at(child.type) != "DmeParticleChild") return false;
                const auto* link = sourceParticleAttributeCandidate(graph, child, "child", 1);
                if (!link || !self(self, link->values.front().reference)) return false;
            }
        }
        active.erase(id);
        complete.insert(id);
        return true;
    };
    if (!visit(visit, definition)) return {};
    return mask;
}
