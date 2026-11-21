#pragma once

#include "vmt_keyvalues_candidate.h"
#include <algorithm>
#include <map>
#include <set>

// Uses the read-only binary DMX graph reader. PCFs never enter the session
// document or its save path.
inline Graph readSourceParticleConfigCandidate(const std::filesystem::path& path) {
    Reader reader(path);
    if (reader.size > 32u * 1024u * 1024u)
        throw std::runtime_error("PCF exceeds the experimental reader's size limit");
    auto header = reader.cstr();
    auto format = header;
    if (!format.empty() && format.back() == '\n') format.pop_back();
    if (!format.empty() && format.back() == '\r') format.pop_back();
    for (unsigned version = 2; version <= 5; ++version) {
        const auto expected = "<!-- dmx encoding binary " + std::to_string(version) +
                              " format pcf " + std::to_string(version == 2 ? 1 : 2) + " -->";
        if (format == expected)
            return readBinaryDmxGraphBody(reader, std::move(header), version, true);
    }
    throw std::runtime_error("unsupported PCF header");
}

inline const Attribute* sourceParticleAttributeCandidate(
        const Graph& graph, const Element& element, const char* name, uint8_t type) {
    const Attribute* result = nullptr;
    for (const auto& attribute : element.attributes) {
        if (graph.strings.at(attribute.name) != name) continue;
        if (result || attribute.type != type || (type < 15 && attribute.values.size() != 1))
            throw std::runtime_error(std::string("particle attribute schema: ") + name);
        result = &attribute;
    }
    return result;
}

struct SourceParticleConfigCandidate {
    std::filesystem::path path;
    Graph graph;
};

struct SourceParticleDefinitionCandidate {
    std::shared_ptr<const SourceParticleConfigCandidate> config;
    uint32_t element;
};

inline std::filesystem::path findLooseParticleFileCandidate(
        const std::vector<std::filesystem::path>& mounts, const std::filesystem::path& relative) {
    for (const auto& mount : mounts) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(mount / relative, error)) continue;
        const auto root = std::filesystem::canonical(mount);
        const auto found = std::filesystem::canonical(mount / relative);
        const auto within = found.lexically_relative(root);
        if (within.empty() || within.has_root_path() || *within.begin() == "..")
            throw std::runtime_error("particle file resolves outside its content mount");
        return found;
    }
    return {};
}

// Only the general manifest and loose Game mounts are covered here. Map
// manifests, packed files, and engine duplicate-name precedence are separate.
class SourceParticleCatalogCandidate {
    std::map<std::string, SourceParticleDefinitionCandidate> definitions_;
    std::set<std::string> ambiguous_;
    size_t loaded_ = 0, missing_ = 0, unreadable_ = 0;

    static std::filesystem::path particlePath(std::string entry) {
        if (!entry.empty() && entry.front() == '!') entry.erase(0, 1);
        std::replace(entry.begin(), entry.end(), '\\', '/');
        const std::filesystem::path path = std::filesystem::u8path(entry);
        if (path.has_root_path() || path.empty() || entry.find(':') != std::string::npos ||
            vmtAsciiLowerCandidate(path.begin()->string()) != "particles" ||
            vmtAsciiLowerCandidate(path.extension().string()) != ".pcf")
            throw std::runtime_error("invalid PCF manifest path");
        for (const auto& part : path)
            if (part == "..") throw std::runtime_error("parent path in PCF manifest");
        return path.lexically_normal();
    }

    void index(const std::filesystem::path& path) {
        const auto config = std::make_shared<const SourceParticleConfigCandidate>(
            SourceParticleConfigCandidate{path, readSourceParticleConfigCandidate(path)});
        const auto& graph = config->graph;
        if (graph.elements.empty()) throw std::runtime_error("missing PCF root");
        const auto* entries = sourceParticleAttributeCandidate(
            graph, graph.elements.front(), "particleSystemDefinitions", 15);
        if (!entries) throw std::runtime_error("missing PCF definition list");
        std::vector<std::pair<std::string, uint32_t>> candidates;
        for (const auto& entry : entries->values) {
            if (entry.reference >= graph.elements.size())
                throw std::runtime_error("unresolved PCF definition");
            const auto& element = graph.elements[entry.reference];
            if (graph.strings.at(element.type) != "DmeParticleSystemDefinition")
                throw std::runtime_error("invalid PCF definition type");
            const auto* hidden = sourceParticleAttributeCandidate(
                graph, element, "preventNameBasedLookup", 4);
            if (hidden && hidden->values.front().wire.front()) continue;
            auto name = vmtAsciiLowerCandidate(graph.strings.at(element.name));
            if (!name.empty()) candidates.emplace_back(std::move(name), entry.reference);
        }
        for (const auto& candidate : candidates) {
            const auto [found, added] = definitions_.emplace(
                candidate.first, SourceParticleDefinitionCandidate{config, candidate.second});
            if (!added && (found->second.config != config || found->second.element != candidate.second))
                ambiguous_.insert(candidate.first);
        }
    }

public:
    explicit SourceParticleCatalogCandidate(const std::vector<std::filesystem::path>& mounts) {
        const auto manifest = findLooseParticleFileCandidate(mounts, "particles/particles_manifest.txt");
        if (manifest.empty()) return;
        Reader reader(manifest);
        if (reader.size > 1024u * 1024u) throw std::runtime_error("PCF manifest too large");
        std::string text(static_cast<size_t>(reader.size), '\0');
        reader.bytes(text.data(), text.size());
        const auto root = parseVmtCandidate(text);
        if (vmtAsciiLowerCandidate(root.key) != "particles_manifest")
            throw std::runtime_error("unexpected particle manifest root");
        std::set<std::string> declared;
        std::set<std::filesystem::path> loaded;
        for (const auto& entry : vmtValuesCandidate(root, "file")) {
            try {
                const auto relative = particlePath(entry);
                if (!declared.insert(vmtAsciiLowerCandidate(relative.generic_u8string())).second) continue;
                const auto path = findLooseParticleFileCandidate(mounts, relative);
                if (path.empty()) {
                    ++missing_;
                } else if (loaded.insert(path).second) {
                    index(path);
                    ++loaded_;
                }
            } catch (const std::runtime_error&) {
                ++unreadable_;
            }
        }
    }

    const SourceParticleDefinitionCandidate* find(const std::string& name) const {
        const auto key = vmtAsciiLowerCandidate(name);
        if (ambiguous_.count(key)) return nullptr;
        const auto found = definitions_.find(key);
        return found == definitions_.end() ? nullptr : &found->second;
    }
    bool ambiguous(const std::string& name) const { return ambiguous_.count(vmtAsciiLowerCandidate(name)) != 0; }
    size_t loaded() const { return loaded_; }
    size_t missing() const { return missing_; }
    size_t unreadable() const { return unreadable_; }
};
