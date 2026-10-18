#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "reference_owned_dmx_projection.h"

int main() {
    try {
        static_assert(sizeof(void*) == 8, "x64 build required");
        constexpr const char* uuid = "5b5114af-27df-44af-b36d-353fa8538e49";
        Graph source;
        source.header = "<!-- dmx encoding binary 5 format sfm_session 20 -->\n";
        source.strings = {"DmeParticleChild", "cig_burn", "child", "children"};
        Element owner{};
        owner.type = 0;
        owner.name = 1;
        Attribute child{};
        child.name = 2;
        child.type = 1;
        child.values.push_back(Value{});
        child.values.back().reference = 0xfffffffeu;
        child.values.back().text = uuid;
        Attribute children{};
        children.name = 3;
        children.type = 15;
        for (int i = 0; i < 2; ++i) {
            children.values.push_back(Value{});
            children.values.back().reference = 0xfffffffeu;
            children.values.back().text = uuid;
        }
        owner.attributes = {child, children};
        source.elements.push_back(owner);
        validateGraph(source);

        RecoveredOwnedDmxReferenceProjectionCandidate<Graph> projection(
            source,
            [](const std::string&, auto&) -> uint32_t { throw std::runtime_error("unexpected external resolver call"); },
            true);
        if (projection.opaqueExternalHandlesByUuid.size() != 1 ||
            projection.opaqueExternalElementHandles.size() != 1 || projection.scene.size() != 2)
            throw std::runtime_error("opaque external UUID was not interned once");
        const auto externalHandle = projection.opaqueExternalHandlesByUuid.at(uuid);
        if (!projection.scene.resolve(externalHandle) || projection.opaqueExternalUuid(externalHandle) != uuid)
            throw std::runtime_error("opaque external UUID identity lookup");

        auto roundtrip = projection.exportReferences([&](uint32_t handle, const auto&) -> std::string {
            return projection.opaqueExternalUuid(handle);
        });
        validateGraph(roundtrip);
        if (roundtrip.elements.size() != source.elements.size() || roundtrip.elements[0].attributes.size() != 2)
            throw std::runtime_error("external-reference graph shape changed");
        for (const auto& attribute : roundtrip.elements[0].attributes) {
            if (attribute.values.empty()) throw std::runtime_error("external-reference array became empty");
            for (const auto& value : attribute.values) {
                if (value.reference != 0xfffffffeu || value.text != uuid)
                    throw std::runtime_error("external UUID did not survive roundtrip");
            }
        }

        size_t removed = 0;
        projection.scene.cleanup({}, [](auto&, bool) {}, [&](auto&) { ++removed; });
        if (projection.scene.size() || projection.scene.attributes.size() ||
            projection.scene.references.handles.active() || removed != 2)
            throw std::runtime_error("opaque external-reference teardown");
        std::cout << "opaque_external_uuid=preserved duplicate_uuid_values=3 opaque_targets=1 teardown=passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
