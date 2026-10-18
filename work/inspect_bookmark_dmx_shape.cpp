#define NOMINMAX
#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include <iostream>
#include <map>
#include <set>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: inspect_bookmark_dmx_shape SESSION.dmx");
        const auto graph = loadGraph(argv[1]);
        std::map<std::string, size_t> typeCounts;
        for (const auto& element : graph.elements) ++typeCounts[graph.strings.at(element.type)];
        for (const auto& [type, count] : typeCounts)
            if (type.find("Bookmark") != std::string::npos || type.find("TimeSelection") != std::string::npos)
                std::cout << "type " << type << " count=" << count << '\n';
        std::vector<uint8_t> isBookmark(graph.elements.size()), isBookmarkSet(graph.elements.size());
        for (size_t i = 0; i < graph.elements.size(); ++i) {
            isBookmark[i] = graph.strings.at(graph.elements[i].type) == "DmeBookmark";
            isBookmarkSet[i] = graph.strings.at(graph.elements[i].type) == "DmeBookmarkSet";
        }
        size_t populatedSets = 0, bookmarkOwners = 0;
        for (size_t i = 0; i < graph.elements.size(); ++i) {
            const auto& element = graph.elements[i];
            const auto& type = graph.strings.at(element.type);
            if (type == "DmeFilmClip" && graph.strings.at(element.name) == "mtt_soldier") {
                std::cout << "benchmark_clip element=" << i << " name=mtt_soldier\n";
                for (const auto& attribute : element.attributes) {
                    const auto& attrName = graph.strings.at(attribute.name);
                    if (attrName == "timeFrame" && attribute.type == 1 && !attribute.values.empty()) {
                        const uint32_t frame = attribute.values.front().reference;
                        std::cout << "  timeFrame_ref=" << frame;
                        if (frame < graph.elements.size()) {
                            const auto& target = graph.elements[frame];
                            std::cout << " type=" << graph.strings.at(target.type) << "\n";
                            for (const auto& field : target.attributes)
                                if ((field.type == 7 || field.type == 3) && field.values.size() == 1 && field.values[0].wire.size() == 4)
                                    std::cout << "    " << graph.strings.at(field.name) << " type=" << unsigned(field.type)
                                              << " value=" << (field.type == 7 ? double(wireInt(field.values[0])) : double(wireFloat(field.values[0]))) << '\n';
                        } else std::cout << " invalid\n";
                    }
                }
            }
            if (type == "DmeBookmarkSet") {
                for (const auto& attribute : element.attributes) {
                    if (graph.strings.at(attribute.name) == "bookmarks" && !attribute.values.empty()) {
                        ++populatedSets;
                        std::cout << "populated_set element=" << i << " refs=" << attribute.values.size() << '\n';
                        if (i == 208966) for (size_t slot = 0; slot < attribute.values.size(); ++slot)
                            std::cout << "  bookmark_slot=" << slot << " ref=" << attribute.values[slot].reference << '\n';
                    }
                }
            }
            for (const auto& attribute : element.attributes) {
                if (attribute.type != 1 && attribute.type != 15) continue;
                for (const auto& value : attribute.values) {
                    if (value.reference >= isBookmark.size() || !isBookmark[value.reference]) continue;
                    ++bookmarkOwners;
                    std::cout << "bookmark_reference owner=" << i << " type=" << type << " name="
                              << graph.strings.at(element.name) << " attribute=" << graph.strings.at(attribute.name)
                              << " target=" << value.reference << '\n';
                }
                for (const auto& value : attribute.values) {
                    if (value.reference >= isBookmarkSet.size() || !isBookmarkSet[value.reference]) continue;
                    std::cout << "bookmark_set_reference owner=" << i << " type=" << type << " name="
                              << graph.strings.at(element.name) << " attribute=" << graph.strings.at(attribute.name)
                              << " target=" << value.reference << '\n';
                }
            }
        }
        std::cout << "populated_bookmark_sets=" << populatedSets << " bookmark_reference_edges=" << bookmarkOwners << '\n';
        size_t shown = 0, shownSets = 0;
        for (size_t i = 0; i < graph.elements.size(); ++i) {
            const auto& element = graph.elements[i];
            const auto& type = graph.strings.at(element.type);
            const auto& name = graph.strings.at(element.name);
            const bool bookmark = type == "DmeBookmark" || name.find("bookmark") != std::string::npos || name.find("Bookmark") != std::string::npos;
            const bool set = type == "DmeBookmarkSet";
            const bool timeSelection = type == "DmeTimeSelection";
            if ((!bookmark && !set && !timeSelection) || (set && shownSets >= 2) || (bookmark && shown >= 32)) continue;
            if (set) ++shownSets;
            ++shown;
            std::cout << "element " << i << " type=" << type << " name=" << name << '\n';
            for (const auto& attribute : element.attributes) {
                const auto& attrName = graph.strings.at(attribute.name);
                std::cout << "  " << attrName << " wire_type=" << unsigned(attribute.type)
                          << " count=" << attribute.values.size();
                if (attribute.type == 5 && !attribute.values.empty())
                    std::cout << " string=" << attribute.values.front().text.substr(0, 96);
                else if ((attribute.type == 1 || attribute.type == 15) && !attribute.values.empty()) {
                    const auto reference = attribute.values.front().reference;
                    std::cout << " ref=" << reference;
                    if (reference < graph.elements.size())
                        std::cout << " target=" << graph.strings.at(graph.elements[reference].type)
                                  << '/' << graph.strings.at(graph.elements[reference].name);
                } else if (attribute.type == 3 && attribute.values.size() == 1 && attribute.values.front().wire.size() == 4)
                    std::cout << " float=" << wireFloat(attribute.values.front());
                else if (attribute.type == 2 && attribute.values.size() == 1 && attribute.values.front().wire.size() == 4)
                    std::cout << " int=" << wireInt(attribute.values.front());
                else if (attribute.type == 7 && attribute.values.size() == 1 && attribute.values.front().wire.size() == 4)
                    std::cout << " raw_ticks=" << wireInt(attribute.values.front());
                std::cout << '\n';
            }
        }
        std::cout << "bookmark_named_elements_shown=" << shown << " total_elements=" << graph.elements.size() << '\n';
        auto findAttribute = [&](uint32_t element, const char* name) -> const Attribute* {
            if (element >= graph.elements.size()) return nullptr;
            for (const auto& attribute : graph.elements[element].attributes)
                if (graph.strings.at(attribute.name) == name) return &attribute;
            return nullptr;
        };
        auto readTime = [&](uint32_t element, const char* name) -> int32_t {
            const auto* attribute = findAttribute(element, name);
            if (!attribute || attribute->type != 7 || attribute->values.size() != 1 || attribute->values[0].wire.size() != 4)
                throw std::runtime_error(std::string("invalid bookmark time field: ") + name);
            return wireInt(attribute->values[0]);
        };
        uint32_t benchmarkClip = 0xffffffffu;
        for (uint32_t i = 0; i < graph.elements.size(); ++i)
            if (graph.strings.at(graph.elements[i].type) == "DmeFilmClip" && graph.strings.at(graph.elements[i].name) == "mtt_soldier") {
                if (benchmarkClip != 0xffffffffu) throw std::runtime_error("duplicate benchmark film clip name");
                benchmarkClip = i;
            }
        if (benchmarkClip == 0xffffffffu) throw std::runtime_error("benchmark film clip missing");
        const auto* frameAttribute = findAttribute(benchmarkClip, "timeFrame");
        const auto* setsAttribute = findAttribute(benchmarkClip, "bookmarkSets");
        if (!frameAttribute || frameAttribute->type != 1 || frameAttribute->values.size() != 1 ||
            !setsAttribute || (setsAttribute->type != 1 && setsAttribute->type != 15))
            throw std::runtime_error("benchmark clip time frame/bookmark set links invalid");
        const uint32_t timeFrame = frameAttribute->values[0].reference;
        const int32_t clipStart = readTime(timeFrame, "start");
        const int32_t clipDuration = readTime(timeFrame, "duration");
        std::set<int32_t> timestamps;
        std::set<std::array<char, 16>> bookmarkIds;
        size_t bookmarkTotal = 0;
        int32_t minTime = INT32_MAX, maxTime = INT32_MIN;
        for (const auto& setValue : setsAttribute->values) {
            if (setValue.reference >= graph.elements.size() || graph.strings.at(graph.elements[setValue.reference].type) != "DmeBookmarkSet")
                throw std::runtime_error("invalid benchmark bookmark set reference");
            const auto* bookmarks = findAttribute(setValue.reference, "bookmarks");
            if (!bookmarks || bookmarks->type != 15) throw std::runtime_error("bookmark set array missing or invalid");
            for (const auto& bookmarkValue : bookmarks->values) {
                if (bookmarkValue.reference >= graph.elements.size() || graph.strings.at(graph.elements[bookmarkValue.reference].type) != "DmeBookmark")
                    throw std::runtime_error("invalid bookmark reference in benchmark set");
                const auto& bookmark = graph.elements[bookmarkValue.reference];
                const int32_t time = readTime(bookmarkValue.reference, "time");
                const int32_t duration = readTime(bookmarkValue.reference, "duration");
                const auto* note = findAttribute(bookmarkValue.reference, "note");
                if (!note || note->type != 5 || note->values.size() != 1) throw std::runtime_error("bookmark note field missing or invalid");
                if (duration != 0 || int64_t(time) < clipStart || int64_t(time) > int64_t(clipStart) + clipDuration)
                    throw std::runtime_error("benchmark bookmark falls outside zero-duration clip marker bounds");
                if (!timestamps.insert(time).second) throw std::runtime_error("duplicate bookmark timestamp in benchmark set");
                if (!bookmarkIds.insert(bookmark.id).second) throw std::runtime_error("duplicate bookmark identity in benchmark set");
                minTime = std::min(minTime, time); maxTime = std::max(maxTime, time); ++bookmarkTotal;
            }
        }
        if (!bookmarkTotal) throw std::runtime_error("benchmark bookmark set is empty");
        std::cout << "bookmark_shape_verified=1 clip=" << benchmarkClip << " sets=1 bookmarks=" << bookmarkTotal
                  << " clip_start_ticks=" << clipStart << " clip_duration_ticks=" << clipDuration
                  << " min_bookmark_tick=" << minTime << " max_bookmark_tick=" << maxTime
                  << " zero_duration=1 unique_times=1 unique_ids=1 graph_modified=no\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
